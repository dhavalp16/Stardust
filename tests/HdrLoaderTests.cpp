// Tests for the Radiance HDR decoder. Each test writes a small .hdr file to the
// temp folder, loads it and checks the pixels.
#include "io/HdrLoader.h"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace {

std::string TempPath(const char *name) {
  return (std::filesystem::temp_directory_path() / name).string();
}

void WriteFile(const std::string &path, const std::string &header,
               const std::vector<unsigned char> &pixels) {
  std::FILE *f = std::fopen(path.c_str(), "wb");
  ASSERT_NE(f, nullptr);
  std::fwrite(header.data(), 1, header.size(), f);
  std::fwrite(pixels.data(), 1, pixels.size(), f);
  std::fclose(f);
}

std::string Header(int width, int height) {
  return "#?RADIANCE\nFORMAT=32-bit_rle_rgbe\n\n-Y " + std::to_string(height) + " +X " +
         std::to_string(width) + "\n";
}

} // namespace

TEST(HdrLoader, ToneCurveMatchesTheFormula) {
  // mantissa 128, exponent 129: value = 128 * 2^(129 - 136) = 1.0
  // exposure 10 -> 10; Reinhard 10 / 11; power 2/2.2; times 255.
  double v = std::pow(10.0 / 11.0, 2.0 / 2.2) * 255.0;
  EXPECT_EQ(io::ToneMapChannel(128, 129), static_cast<unsigned char>(v));
  EXPECT_EQ(io::ToneMapChannel(200, 0), 0); // exponent 0 means black
}

TEST(HdrLoader, EveryMantissaAndExponentDecodesThroughTheTable) {
  // A 256 x 256 flat image whose pixel (x, y) has mantissa x and exponent y
  // covers all 65536 combinations, so this proves the lookup table gives
  // exactly what the tone curve gives.
  std::vector<unsigned char> pixels;
  for (int y = 0; y < 256; y++)
    for (int x = 0; x < 256; x++) {
      auto m = static_cast<unsigned char>(x), e = static_cast<unsigned char>(y);
      pixels.insert(pixels.end(), {m, m, m, e});
    }
  std::string path = TempPath("stardust_test_all.hdr");
  WriteFile(path, Header(256, 256), pixels);

  io::HdrImage image;
  ASSERT_TRUE(io::LoadHdr(path, image));
  ASSERT_EQ(image.width, 256);
  ASSERT_EQ(image.height, 256);
  for (int y = 0; y < 256; y++)
    for (int x = 0; x < 256; x++) {
      size_t i = static_cast<size_t>(y * 256 + x) * 4;
      unsigned char expected =
          io::ToneMapChannel(static_cast<unsigned char>(x), static_cast<unsigned char>(y));
      ASSERT_EQ(image.rgba[i], expected) << "x=" << x << " y=" << y;
      ASSERT_EQ(image.rgba[i + 3], 255);
    }
  std::filesystem::remove(path);
}

TEST(HdrLoader, DecodesRunLengthEncodedRows) {
  // One row, 8 pixels wide. Row header 2, 2, 0, 8, then each channel:
  //   R: a run of 8 copies of 100           -> (128 + 8), 100
  //   G: 8 literal bytes 0..7               -> 8, 0, 1, 2, 3, 4, 5, 6, 7
  //   B: run of 4 x 50 then 4 literals      -> (128 + 4), 50, 4, 9, 9, 9, 9
  //   E: a run of 8 copies of 129           -> (128 + 8), 129
  std::vector<unsigned char> data = {2, 2, 0, 8, 136, 100, 8, 0, 1, 2, 3, 4, 5, 6, 7,
                                     132, 50, 4, 9, 9, 9, 9, 136, 129};
  std::string path = TempPath("stardust_test_rle.hdr");
  WriteFile(path, Header(8, 1), data);

  io::HdrImage image;
  std::string error;
  ASSERT_TRUE(io::LoadHdr(path, image, &error)) << error;
  for (int x = 0; x < 8; x++) {
    EXPECT_EQ(image.rgba[x * 4 + 0], io::ToneMapChannel(100, 129));
    EXPECT_EQ(image.rgba[x * 4 + 1], io::ToneMapChannel(static_cast<unsigned char>(x), 129));
    EXPECT_EQ(image.rgba[x * 4 + 2], io::ToneMapChannel(x < 4 ? 50 : 9, 129));
  }
  std::filesystem::remove(path);
}

TEST(HdrLoader, MissingFileFailsWithAMessage) {
  io::HdrImage image;
  std::string error;
  EXPECT_FALSE(io::LoadHdr(TempPath("stardust_does_not_exist.hdr"), image, &error));
  EXPECT_NE(error.find("cannot open"), std::string::npos);
}

TEST(HdrLoader, RejectsFilesWithoutTheSignature) {
  std::string path = TempPath("stardust_test_bad.hdr");
  WriteFile(path, "P6\n2 2\n255\n", {0, 0, 0, 0});
  io::HdrImage image;
  std::string error;
  EXPECT_FALSE(io::LoadHdr(path, image, &error));
  EXPECT_NE(error.find("signature"), std::string::npos);
  std::filesystem::remove(path);
}

TEST(HdrLoader, RejectsTruncatedFiles) {
  std::string path = TempPath("stardust_test_short.hdr");
  WriteFile(path, Header(4, 4), {1, 1, 1, 128, 2, 2}); // 16 pixels promised, 1.5 given
  io::HdrImage image;
  std::string error;
  EXPECT_FALSE(io::LoadHdr(path, image, &error));
  EXPECT_NE(error.find("ends early"), std::string::npos);
  std::filesystem::remove(path);
}
