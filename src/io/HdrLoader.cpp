#include "io/HdrLoader.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>

namespace io {

unsigned char ToneMapChannel(unsigned char mantissa, unsigned char exponent) {
  // The same operations, in the same order and precision, as the original
  // per-pixel code, so the lookup table reproduces it exactly.
  float v = 0.0f;
  if (exponent != 0) v = mantissa * std::ldexp(1.0f, static_cast<int>(exponent) - 128 - 8);
  const float exposure = 10.0f;
  v *= exposure;
  v = v / (v + 1.0f); // Reinhard tone mapping: 0..infinity -> 0..1
  // Gamma 1/2.2 followed by a contrast power of 2.0, combined: (v^(1/2.2))^2 = v^(2/2.2).
  v = std::pow(v, 2.0f / 2.2f);
  return static_cast<unsigned char>(v * 255.0f);
}

namespace {

bool Fail(std::string *error, const std::string &message) {
  if (error) *error = message;
  return false;
}

// Closes the file automatically when it goes out of scope, whichever way the
// function returns. ("RAII": the resource is released by a destructor.)
struct FileCloser {
  void operator()(std::FILE *f) const { std::fclose(f); }
};
using FilePtr = std::unique_ptr<std::FILE, FileCloser>;

// Walks through the file's bytes, which are all in memory. Reading the whole
// file with one fread and then stepping through an array is much faster than
// asking the C library for one byte at a time (see docs/MEASUREMENTS.md).
struct ByteReader {
  const unsigned char *pos;
  const unsigned char *end;

  bool AtEnd() const { return pos >= end; }
  size_t Remaining() const { return static_cast<size_t>(end - pos); }

  // Copies one text line (up to '\n') into `line`. False at end of file.
  bool ReadLine(std::string &line) {
    if (AtEnd()) return false;
    line.clear();
    while (pos < end && *pos != '\n') line.push_back(static_cast<char>(*pos++));
    if (pos < end) pos++; // skip the '\n'
    if (!line.empty() && line.back() == '\r') line.pop_back();
    return true;
  }
};

// Reads one row of RGBE pixels into `row` (4 bytes per pixel).
bool ReadScanline(ByteReader &in, int width, unsigned char *row) {
  if (in.Remaining() < 4) return false;
  const unsigned char *header = in.pos;

  bool isRunLength = width >= 8 && width <= 0x7fff && header[0] == 2 && header[1] == 2;
  if (!isRunLength) {
    // Flat: plain RGBE bytes, 4 per pixel.
    size_t bytes = static_cast<size_t>(width) * 4;
    if (in.Remaining() < bytes) return false;
    std::memcpy(row, in.pos, bytes);
    in.pos += bytes;
    return true;
  }

  // "New-style" run-length encoding: the row is stored one channel at a time
  // (all reds, then all greens, ...). Each chunk starts with a count byte:
  //   count > 128: the next byte repeats (count - 128) times;
  //   otherwise:   the next `count` bytes are copied as they are.
  if (((header[2] << 8) | header[3]) != width) return false;
  in.pos += 4;
  for (int channel = 0; channel < 4; channel++) {
    int x = 0;
    while (x < width) {
      if (in.AtEnd()) return false;
      int code = *in.pos++;
      if (code > 128) {
        int count = code - 128;
        if (in.AtEnd() || x + count > width) return false;
        unsigned char value = *in.pos++;
        for (int i = 0; i < count; i++) row[(x++) * 4 + channel] = value;
      } else {
        int count = code;
        if (count == 0 || x + count > width || in.Remaining() < static_cast<size_t>(count))
          return false;
        for (int i = 0; i < count; i++) row[(x++) * 4 + channel] = *in.pos++;
      }
    }
  }
  return true;
}

} // namespace

bool LoadHdr(const std::string &path, HdrImage &out, std::string *error) {
  // Read the whole file into memory.
  std::vector<unsigned char> bytes;
  {
    FilePtr file(std::fopen(path.c_str(), "rb"));
    if (!file) return Fail(error, "cannot open " + path);
    std::fseek(file.get(), 0, SEEK_END);
    long size = std::ftell(file.get());
    std::fseek(file.get(), 0, SEEK_SET);
    if (size <= 0) return Fail(error, "file is empty");
    bytes.resize(static_cast<size_t>(size));
    if (std::fread(bytes.data(), 1, bytes.size(), file.get()) != bytes.size())
      return Fail(error, "could not read " + path);
  }
  ByteReader in{bytes.data(), bytes.data() + bytes.size()};

  // Header: text lines, the first starting "#?" (#?RADIANCE or #?RGBE), ended
  // by an empty line.
  std::string line;
  if (!in.ReadLine(line) || line.compare(0, 2, "#?") != 0)
    return Fail(error, "not a Radiance HDR file (no #? signature)");
  while (in.ReadLine(line) && !line.empty()) {
    if (line.rfind("FORMAT=", 0) == 0 && line != "FORMAT=32-bit_rle_rgbe")
      return Fail(error, "unsupported pixel format (only RGBE is supported)");
  }

  // Resolution line, normally "-Y <height> +X <width>" (rows top to bottom).
  // The other orientations are accepted but not flipped, as before.
  int width = 0, height = 0;
  if (!in.ReadLine(line)) return Fail(error, "missing resolution line");
  const char *res = line.c_str();
  if (std::sscanf(res, "-Y %d +X %d", &height, &width) != 2 &&
      std::sscanf(res, "+Y %d +X %d", &height, &width) != 2 &&
      std::sscanf(res, "+X %d -Y %d", &width, &height) != 2 &&
      std::sscanf(res, "+X %d +Y %d", &width, &height) != 2)
    return Fail(error, "unreadable resolution line");
  if (width <= 0 || height <= 0 || width > 65536 || height > 65536)
    return Fail(error, "image size out of range");

  // The tone curve for every possible (exponent, mantissa) pair: 64 KB,
  // computed once. The 50-megapixel sky would otherwise need 150 million
  // pow() calls.
  static const std::array<unsigned char, 256 * 256> toneTable = [] {
    std::array<unsigned char, 256 * 256> table{};
    for (int e = 0; e < 256; e++)
      for (int m = 0; m < 256; m++)
        table[static_cast<size_t>(e * 256 + m)] =
            ToneMapChannel(static_cast<unsigned char>(m), static_cast<unsigned char>(e));
    return table;
  }();

  out.width = width;
  out.height = height;
  out.rgba.assign(static_cast<size_t>(width) * height * 4, 0);
  std::vector<unsigned char> row(static_cast<size_t>(width) * 4);

  for (int y = 0; y < height; y++) {
    if (!ReadScanline(in, width, row.data())) return Fail(error, "file ends early or is corrupt");
    unsigned char *dst = out.rgba.data() + static_cast<size_t>(y) * width * 4;
    for (int x = 0; x < width; x++) {
      const unsigned char *rgbe = &row[static_cast<size_t>(x) * 4];
      const unsigned char *curve = &toneTable[static_cast<size_t>(rgbe[3]) * 256];
      dst[x * 4 + 0] = curve[rgbe[0]];
      dst[x * 4 + 1] = curve[rgbe[1]];
      dst[x * 4 + 2] = curve[rgbe[2]];
      dst[x * 4 + 3] = 255;
    }
  }
  return true;
}

} // namespace io
