// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

#include "build/build_config.h"
#include "clay/common/service/service_manager.h"
#include "clay/common/thread_host.h"
#include "clay/flow/frame_timings.h"
#include "clay/flow/layers/transform_layer.h"
#include "clay/flow/surface.h"
#include "clay/public/clay.h"
#include "clay/shell/common/rasterizer.h"
#include "clay/shell/common/services/platform_const_service.h"
#include "clay/shell/common/services/rasterizer_service.h"
#if defined(OS_HARMONY)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <native_image/native_image.h>
#endif
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

#if defined(OS_HARMONY)
TEST(ClaySharedImageTest, NativeImageAcceptsUnspecifiedDimensions) {
  for (const ClaySize size :
       {ClaySize{0, 0}, ClaySize{0, 16}, ClaySize{16, 0}}) {
    auto image = ClayCreateSharedImage(
        kClaySharedImageBackingTypeNativeImage,
        kClaySharedImageBackingPixelFormatNative8888, &size);
    ASSERT_NE(image, nullptr);
    ClaySize actual{};
    ClaySharedImageGetSize(image, &actual);
    EXPECT_EQ(actual.width, 256u);
    EXPECT_EQ(actual.height, 256u);
    ClayReleaseSharedImage(image);

    auto* native = OH_NativeImage_Create(0, GL_TEXTURE_EXTERNAL_OES);
    ASSERT_NE(native, nullptr);
    image = ClayCreateSharedImageFromHandle(
        kClaySharedImageBackingTypeNativeImage,
        kClaySharedImageBackingPixelFormatNative8888, &size, native);
    if (!image) OH_NativeImage_Destroy(&native);
    ASSERT_NE(image, nullptr);
    ClaySharedImageGetSize(image, &actual);
    EXPECT_EQ(actual.width, 256u);
    EXPECT_EQ(actual.height, 256u);
    ClayReleaseSharedImage(image);
  }
}
#endif

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

#ifdef ENABLE_SKITY
class TestSkitySurface final : public skity::GPUSurface {
 public:
  TestSkitySurface(uint32_t width, uint32_t height)
      : bitmap_(width, height, skity::AlphaType::kPremul_AlphaType),
        canvas_(skity::Canvas::MakeSoftwareCanvas(&bitmap_)) {}

  uint32_t GetWidth() const override { return bitmap_.Width(); }
  uint32_t GetHeight() const override { return bitmap_.Height(); }
  float ContentScale() const override { return 1.f; }
  skity::Canvas* LockCanvas(bool clear) override {
    if (clear) {
      canvas_->Clear(skity::Color_TRANSPARENT);
    }
    return canvas_.get();
  }
  void Flush() override { canvas_->Flush(); }
  std::shared_ptr<skity::Pixmap> ReadPixels(const skity::Rect& rect) override {
    return bitmap_.GetPixmap();
  }
  const skity::Bitmap& GetBitmap() const { return bitmap_; }

 private:
  skity::Bitmap bitmap_;
  std::unique_ptr<skity::Canvas> canvas_;
};
#endif

class TestRasterSurface final : public Surface {
 public:
  using AcquireCallback =
      std::function<std::unique_ptr<SurfaceFrame>(const skity::Vec2&)>;

  explicit TestRasterSurface(AcquireCallback on_acquire)
      : on_acquire_(std::move(on_acquire)) {}

  bool IsValid() override { return true; }
  std::unique_ptr<SurfaceFrame> AcquireFrame(const skity::Vec2& size) override {
    return on_acquire_(size);
  }
  skity::Matrix GetRootTransformation() const override {
    skity::Matrix identity;
    identity.Reset();
    return identity;
  }
  clay::GrContext* GetContext() override { return nullptr; }
  clay::GrContextPtr GetContextPtr() override { return nullptr; }
  std::unique_ptr<GLContextResult> MakeRenderContextCurrent() override {
    return std::make_unique<GLContextDefaultResult>(true);
  }
  bool ClearRenderContext() override { return true; }
  bool AllowsDrawingWhenGpuDisabled() const override { return true; }

 private:
  AcquireCallback on_acquire_;
};

TEST(RasterizerCanvasBackgroundTest, ClearsSurfaceAndMarksFullDamageOnUpdate) {
  ThreadHost thread_host("lynx.clay.rasterizer.bg.test.",
                         ThreadHost::Type::Platform | ThreadHost::Type::RASTER |
                             ThreadHost::Type::IO | ThreadHost::Type::UI);
  TaskRunners task_runners("test", thread_host.platform_thread->GetTaskRunner(),
                           thread_host.raster_thread->GetTaskRunner(),
                           thread_host.ui_thread->GetTaskRunner(),
                           thread_host.io_thread->GetTaskRunner());

  task_runners.GetRasterTaskRunner()->PostSyncTask([&] {
    auto service_manager = ServiceManager::Create(
        {task_runners.GetPlatformTaskRunner(), task_runners.GetUITaskRunner(),
         task_runners.GetRasterTaskRunner(), task_runners.GetIOTaskRunner()});
    Settings settings;
    service_manager->RegisterService<PlatformConstService>(
        std::make_shared<PlatformConstService>(settings, true, nullptr));

    auto rasterizer = std::make_unique<Rasterizer>(service_manager);
    service_manager->Attach<Owner::kRaster>(
        RasterServiceContext{.rasterizer = rasterizer.get()});
    EXPECT_FALSE(rasterizer->GetBackgroundColor().has_value());

#ifdef ENABLE_SKITY
    std::shared_ptr<TestSkitySurface> latest_surface;
#else
    sk_sp<SkSurface> latest_surface;
#endif
    std::optional<skity::Rect> latest_frame_damage;
    std::optional<skity::Rect> latest_buffer_damage;

    rasterizer->Setup(std::make_unique<TestRasterSurface>(
        [&](const skity::Vec2& size) -> std::unique_ptr<SurfaceFrame> {
          SurfaceFrame::FramebufferInfo fb_info;
          fb_info.supports_partial_repaint = true;
          fb_info.existing_damage = skity::Rect::MakeXYWH(0, 0, 1, 1);
#ifdef ENABLE_SKITY
          latest_surface = std::make_shared<TestSkitySurface>(
              static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y));
#else
          latest_surface = SkSurface::MakeRaster(SkImageInfo::Make(
              static_cast<int>(size.x), static_cast<int>(size.y),
              kRGBA_8888_SkColorType, kPremul_SkAlphaType));
#endif
          return std::make_unique<SurfaceFrame>(
              latest_surface, fb_info,
              [](SurfaceFrame&, clay::GrCanvas*) { return true; },
              [&](const SurfaceFrame::SubmitInfo& info) {
                latest_frame_damage = info.frame_damage;
                latest_buffer_damage = info.buffer_damage;
                return true;
              },
              size);
        }));

    auto make_recorder = []() {
      auto recorder = std::make_unique<FrameTimingsRecorder>();
      const auto now = fml::TimePoint::Now();
      recorder->RecordVsync(now, now + fml::TimeDelta::FromMilliseconds(16));
      recorder->RecordBuildStart(now);
      recorder->RecordBuildEnd(now);
      return recorder;
    };

    auto layer_tree = std::make_shared<LayerTree>(skity::Vec2{40, 40}, 1.0f);
    skity::Matrix identity;
    identity.Reset();
    layer_tree->set_root_layer(std::make_shared<TransformLayer>(identity));

    auto expect_rgba_at_center = [&](uint8_t r, uint8_t g, uint8_t b,
                                     uint8_t a) {
#ifdef ENABLE_SKITY
      auto pixmap = latest_surface->GetBitmap().GetPixmap();
      ASSERT_NE(pixmap, nullptr);
      const uint8_t* px = pixmap->Addr8(20, 20);
#else
      SkPixmap pixmap;
      ASSERT_TRUE(latest_surface->peekPixels(&pixmap));
      const auto* px = reinterpret_cast<const uint8_t*>(pixmap.addr32(20, 20));
#endif
      EXPECT_EQ(px[0], r);
      EXPECT_EQ(px[1], g);
      EXPECT_EQ(px[2], b);
      EXPECT_EQ(px[3], a);
    };

    // Unconfigured canvas clears to transparent.
    EXPECT_EQ(rasterizer->Draw(layer_tree, make_recorder()),
              RasterStatus::kSuccess);
    expect_rgba_at_center(0x00, 0x00, 0x00, 0x00);

    // Updating canvas background color forces full-frame damage on
    // partial-repaint surfaces and clears to the configured ARGB color.
    rasterizer->SetBackgroundColor(0xFF123456u);
    ASSERT_TRUE(rasterizer->GetBackgroundColor().has_value());
    EXPECT_EQ(*rasterizer->GetBackgroundColor(), 0xFF123456u);
    EXPECT_EQ(rasterizer->DrawLastLayerTree(), RasterStatus::kSuccess);
    ASSERT_TRUE(latest_frame_damage.has_value());
    EXPECT_EQ(*latest_frame_damage, skity::Rect::MakeXYWH(0, 0, 40, 40));
    ASSERT_TRUE(latest_buffer_damage.has_value());
    EXPECT_EQ(*latest_buffer_damage, skity::Rect::MakeXYWH(0, 0, 40, 40));
    expect_rgba_at_center(0x12, 0x34, 0x56, 0xFF);

    // Subsequent static redraw without changing background color only repaints
    // existing_damage.
    EXPECT_EQ(rasterizer->DrawLastLayerTree(), RasterStatus::kSuccess);
    ASSERT_TRUE(latest_buffer_damage.has_value());
    EXPECT_EQ(*latest_buffer_damage, skity::Rect::MakeXYWH(0, 0, 1, 1));

    // ScreenshotLastLayerTree composites over the configured background color.
    auto screenshot = rasterizer->ScreenshotLastLayerTree(
        ScreenshotData::ScreenshotType::UncompressedImage, false,
        *rasterizer->GetBackgroundColor());
    ASSERT_NE(screenshot.data, nullptr);
    ASSERT_GE(DATA_GET_SIZE(screenshot.data), 4u);
    const auto* shot_bytes =
        static_cast<const uint8_t*>(DATA_GET_DATA(screenshot.data));
    EXPECT_EQ(shot_bytes[0], 0x12);
    EXPECT_EQ(shot_bytes[1], 0x34);
    EXPECT_EQ(shot_bytes[2], 0x56);
    EXPECT_EQ(shot_bytes[3], 0xFF);

    // Transparent screenshot preserves zero alpha instead of forcing opaque.
    auto transparent_shot = rasterizer->ScreenshotLastLayerTree(
        ScreenshotData::ScreenshotType::UncompressedImage, false,
        Color::kTransparent().Value());
    ASSERT_NE(transparent_shot.data, nullptr);
    ASSERT_GE(DATA_GET_SIZE(transparent_shot.data), 4u);
    const auto* transparent_bytes =
        static_cast<const uint8_t*>(DATA_GET_DATA(transparent_shot.data));
    EXPECT_EQ(transparent_bytes[0], 0x00);
    EXPECT_EQ(transparent_bytes[1], 0x00);
    EXPECT_EQ(transparent_bytes[2], 0x00);
    EXPECT_EQ(transparent_bytes[3], 0x00);

    // CleanForRecycle retains the configured host canvas background color.
    rasterizer->CleanForRecycle();
    ASSERT_TRUE(rasterizer->GetBackgroundColor().has_value());
    EXPECT_EQ(*rasterizer->GetBackgroundColor(), 0xFF123456u);

    service_manager->Detach<Owner::kRaster>();
  });
}

}  // namespace
}  // namespace testing
}  // namespace clay
