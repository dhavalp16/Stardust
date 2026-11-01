#pragma once
#include <string>
#include <vector>

namespace io {

// An 8-bit RGBA image produced from a Radiance HDR file: 4 bytes per pixel,
// rows from top to bottom.
struct HdrImage {
  int width = 0;
  int height = 0;
  std::vector<unsigned char> rgba;
};

// Loads a Radiance .hdr file (RGBE pixels, flat or run-length encoded) and
// tone-maps it to 8-bit colour as it goes, one row at a time. raylib's own
// loader first expands the whole file to 32-bit floats per channel (about
// 600 MB for the 50-megapixel sky), which is why this project has its own.
//
// Returns false and fills `error` if the file is missing or malformed.
bool LoadHdr(const std::string &path, HdrImage &out, std::string *error = nullptr);

// The tone curve applied to one channel. An RGBE pixel stores a mantissa
// (0-255) per channel and one shared exponent; the real value is
// mantissa * 2^(exponent - 136). That value is brightened (exposure 10),
// squeezed into 0..1 by Reinhard's curve x / (x + 1), shaped by a power of
// 2/2.2, and stored as 0-255.
//
// LoadHdr does not call this per pixel: there are only 256 x 256 possible
// (mantissa, exponent) pairs, so it fills a lookup table with this function
// once and then just reads the table. Exposed so the tests can check that the
// table matches it exactly.
unsigned char ToneMapChannel(unsigned char mantissa, unsigned char exponent);

} // namespace io
