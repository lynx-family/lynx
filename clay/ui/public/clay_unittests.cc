// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <limits>
#include <vector>

#include "clay/public/clay.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

#ifdef ENABLE_SKITY
#include "skity/codec/codec.hpp"
#include "skity/io/data.hpp"
#include "skity/io/pixmap.hpp"
#else
#include "third_party/skia/include/codec/SkCodec.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#endif

namespace clay {
namespace testing {
namespace {

class ClayEncodeBitmapTest : public ::testing::TestWithParam<ClayImageFormat> {
 protected:
  void SetUp() override {
    bitmap_.struct_size = sizeof(bitmap_);
    bitmap_.width = 2;
    bitmap_.height = 3;
    bitmap_.pixels.ptr = pixels_.data();
    bitmap_.pixels.size = pixels_.size();
    bitmap_.pixels.user_data = &input_releases_;
    bitmap_.pixels.destruction_callback = [](const void*, void* user_data) {
      ++*static_cast<int*>(user_data);
    };
    Fill({32, 64, 96, 255});
  }

  void TearDown() override {
    ReleaseOutput();
    EXPECT_EQ(input_releases_, 0);
  }

  void ReleaseOutput() {
    if (output_.destruction_callback) {
      output_.destruction_callback(output_.ptr, output_.user_data);
      output_ = {};
    }
  }

  void Fill(const std::array<uint8_t, 4>& color) {
    for (size_t i = 0; i < pixels_.size(); ++i) {
      pixels_[i] = color[i % color.size()];
    }
  }

  void DecodeOutput(std::vector<uint8_t>* pixels) {
    ASSERT_NE(output_.ptr, nullptr);
    ASSERT_GT(output_.size, 0u);
    ASSERT_NE(output_.destruction_callback, nullptr);
    ASSERT_NE(output_.user_data, nullptr);
#ifdef ENABLE_SKITY
    auto data = skity::Data::MakeWithCopy(output_.ptr, output_.size);
    auto codec = skity::Codec::MakeFromData(data);
    ASSERT_NE(codec, nullptr);
    codec->SetData(data);
    auto pixmap = codec->Decode();
    ASSERT_NE(pixmap, nullptr);
    EXPECT_EQ(pixmap->Width(), bitmap_.width);
    EXPECT_EQ(pixmap->Height(), bitmap_.height);
    ASSERT_EQ(pixmap->GetColorType(), skity::ColorType::kRGBA);
    ASSERT_EQ(pixmap->GetAlphaType(), skity::AlphaType::kUnpremul_AlphaType);
    for (uint32_t y = 0; y < pixmap->Height(); ++y) {
      const auto* row = pixmap->Addr8(0, y);
      pixels->insert(pixels->end(), row, row + pixmap->Width() * 4);
    }
#else
    auto data = SkData::MakeWithCopy(output_.ptr, output_.size);
    auto codec = SkCodec::MakeFromData(data);
    ASSERT_NE(codec, nullptr);
    EXPECT_EQ(codec->getInfo().width(), static_cast<int>(bitmap_.width));
    EXPECT_EQ(codec->getInfo().height(), static_cast<int>(bitmap_.height));
    auto info = codec->getInfo()
                    .makeColorType(kRGBA_8888_SkColorType)
                    .makeAlphaType(kUnpremul_SkAlphaType);
    pixels->resize(info.computeMinByteSize());
    ASSERT_EQ(codec->getPixels(info, pixels->data(), info.minRowBytes()),
              SkCodec::kSuccess);
#endif
  }

  std::array<uint8_t, 24> pixels_{};
  ClayBitmap bitmap_{};
  ClayDataHolder output_{};
  int input_releases_ = 0;
};

TEST_P(ClayEncodeBitmapTest, EncodesRequestedFormatAndPreservesPixels) {
  const auto original = pixels_;
  ASSERT_TRUE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, &output_));
  EXPECT_EQ(pixels_, original);
  const auto* bytes = static_cast<const uint8_t*>(output_.ptr);
  if (GetParam() == kClayImageFormatPNG) {
    const std::array<uint8_t, 8> signature = {137, 80, 78, 71, 13, 10, 26, 10};
    ASSERT_GE(output_.size, signature.size());
    EXPECT_EQ(std::vector<uint8_t>(bytes, bytes + signature.size()),
              std::vector<uint8_t>(signature.begin(), signature.end()));
  } else {
    ASSERT_GE(output_.size, 2u);
    EXPECT_EQ(bytes[0], 0xff);
    EXPECT_EQ(bytes[1], 0xd8);
  }

  // The encoded output must not depend on the caller's pixel buffer.
  pixels_.fill(0);
  std::vector<uint8_t> decoded;
  ASSERT_NO_FATAL_FAILURE(DecodeOutput(&decoded));
  ASSERT_EQ(decoded.size(), original.size());
  for (size_t i = 0; i < decoded.size(); ++i) {
    EXPECT_NEAR(decoded[i], original[i],
                GetParam() == kClayImageFormatPNG ? 0 : 2);
  }
  ReleaseOutput();
  EXPECT_EQ(input_releases_, 0);
}

TEST_P(ClayEncodeBitmapTest, HandlesPremultipliedAlpha) {
  Fill({128, 0, 0, 128});
  const auto original = pixels_;
  ASSERT_TRUE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, &output_));
  EXPECT_EQ(pixels_, original);
  std::vector<uint8_t> decoded;
  ASSERT_NO_FATAL_FAILURE(DecodeOutput(&decoded));
  ASSERT_EQ(decoded.size(), pixels_.size());
  // PNG stores straight alpha; JPEG composites the premultiplied RGB on black.
  const std::array<uint8_t, 4> expected =
      GetParam() == kClayImageFormatPNG
          ? std::array<uint8_t, 4>{255, 0, 0, 128}
          : std::array<uint8_t, 4>{128, 0, 0, 255};
  for (size_t i = 0; i < decoded.size(); ++i) {
    EXPECT_NEAR(decoded[i], expected[i % 4],
                GetParam() == kClayImageFormatPNG ? 0 : 2);
  }
}

TEST_P(ClayEncodeBitmapTest, RejectsNullArguments) {
  EXPECT_FALSE(ClayEncodeBitmap(nullptr, GetParam(), 1.0f, &output_));
  EXPECT_FALSE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, nullptr));
  bitmap_.pixels.ptr = nullptr;
  EXPECT_FALSE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, &output_));
}

TEST_P(ClayEncodeBitmapTest, RejectsEmptyDimensions) {
  bitmap_.width = 0;
  EXPECT_FALSE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, &output_));
  bitmap_.width = 2;
  bitmap_.height = 0;
  EXPECT_FALSE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, &output_));
}

TEST_P(ClayEncodeBitmapTest, RejectsTruncatedPixels) {
  --bitmap_.pixels.size;
  EXPECT_FALSE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, &output_));
  bitmap_.pixels.size = 0;
  EXPECT_FALSE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, &output_));
}

TEST_P(ClayEncodeBitmapTest, RejectsOversizedDimensionsWithoutOverflow) {
  bitmap_.width = std::numeric_limits<uint32_t>::max();
  bitmap_.height = std::numeric_limits<uint32_t>::max();
  EXPECT_FALSE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, &output_));
}

TEST_P(ClayEncodeBitmapTest, FailurePreservesExistingOutput) {
  ASSERT_TRUE(ClayEncodeBitmap(&bitmap_, GetParam(), 1.0f, &output_));
  const auto original = output_;
  EXPECT_FALSE(ClayEncodeBitmap(&bitmap_, static_cast<ClayImageFormat>(-1),
                                1.0f, &output_));
  EXPECT_EQ(output_.ptr, original.ptr);
  EXPECT_EQ(output_.size, original.size);
  EXPECT_EQ(output_.user_data, original.user_data);
  EXPECT_EQ(output_.destruction_callback, original.destruction_callback);
}

INSTANTIATE_TEST_SUITE_P(
    ImageFormats, ClayEncodeBitmapTest,
    ::testing::Values(kClayImageFormatPNG, kClayImageFormatJPEG),
    [](const ::testing::TestParamInfo<ClayImageFormat>& info) {
      return info.param == kClayImageFormatPNG ? "PNG" : "JPEG";
    });

}  // namespace
}  // namespace testing
}  // namespace clay
