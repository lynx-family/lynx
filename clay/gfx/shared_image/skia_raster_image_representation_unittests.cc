// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <cstring>
#include <memory>

#include "clay/common/graphics/shared_image_external_bitmap.h"
#include "clay/gfx/color_space.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/iosurface_image_backing.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/gfx/shared_image/skia_raster_image_representation.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkSurface.h"

namespace clay {
namespace {

class PixelBacking final : public SharedImageBacking {
 public:
  explicit PixelBacking(PixelFormat format = PixelFormat::kNative8888,
                        ClayColorSpace color_space = kClayColorSpaceSRGB)
      : SharedImageBacking(format, {1, 2}, color_space) {}
  BackingType GetType() const override { return BackingType::kMocking; }
  GraphicsMemoryHandle GetGFXHandle() const override { return nullptr; }
  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig*) override {
    return nullptr;
  }
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrDirectContext*) override {
    return fml::MakeRefCounted<SkiaRasterImageRepresentation>(fml::Ref(this));
  }
  bool ReadbackToMemory(SharedImageReadbackPixmap* pixmaps,
                        uint32_t planes) override {
    ++reads;
    if (fail_read || planes != 1) return false;
    for (int y = 0; y < 2; ++y) {
      std::memcpy(pixmaps->writable_addr(0, y), pixels.data() + y * 4, 4);
    }
    return true;
  }
  std::array<uint8_t, 8> pixels{255, 0, 0, 255, 0, 0, 255, 255};
  int reads = 0;
  bool fail_read = false;
};

TEST(SkiaRasterImage, FlipsBgraAndOwnsSnapshotAfterBackingChanges) {
  auto backing = fml::MakeRefCounted<PixelBacking>();
  auto representation = backing->CreateSkiaRepresentation(nullptr);
  auto first = representation->GetSkImage();
  ASSERT_TRUE(first);
  SkPixmap pixels;
  ASSERT_TRUE(first->peekPixels(&pixels));
  EXPECT_EQ(pixels.getColor(0, 0), SK_ColorRED);
  EXPECT_EQ(pixels.getColor(0, 1), SK_ColorBLUE);
  backing->pixels.fill(0);
  auto second = representation->GetSkImage();
  ASSERT_TRUE(second);
  EXPECT_EQ(pixels.getColor(0, 0), SK_ColorRED);
  ASSERT_TRUE(second->peekPixels(&pixels));
  EXPECT_EQ(pixels.getColor(0, 0), SK_ColorTRANSPARENT);
}

TEST(SkiaRasterImage, PreservesRgbaAndDisplayP3) {
  auto backing = fml::MakeRefCounted<PixelBacking>(
      SharedImageBacking::PixelFormat::kRGBA8888, kClayColorSpaceDisplayP3);
  auto image = backing->CreateSkiaRepresentation(nullptr)->GetSkImage();
  ASSERT_TRUE(image);
  EXPECT_TRUE(SkColorSpace::Equals(
      image->colorSpace(), ToSkColorSpace(kClayColorSpaceDisplayP3).get()));
  SkPixmap pixels;
  ASSERT_TRUE(image->peekPixels(&pixels));
  EXPECT_EQ(pixels.colorType(), kRGBA_8888_SkColorType);
  EXPECT_EQ(pixels.getColor(0, 0), SK_ColorBLUE);
  EXPECT_EQ(pixels.getColor(0, 1), SK_ColorRED);
}

TEST(SkiaRasterImage, DoesNotReadOrLoseFailedFence) {
  auto backing = fml::MakeRefCounted<PixelBacking>();
  auto representation = backing->CreateSkiaRepresentation(nullptr);
  representation->ConsumeFence(std::make_unique<FailedFenceSync>());
  EXPECT_FALSE(representation->GetSkImage());
  EXPECT_EQ(backing->reads, 0);
  EXPECT_FALSE(representation->EndRead());
  auto fence = representation->ProduceFence();
  ASSERT_TRUE(fence);
  EXPECT_FALSE(fence->ClientWait());
}

TEST(SkiaRasterImage, DoesNotPublishFailedReadback) {
  auto backing = fml::MakeRefCounted<PixelBacking>();
  backing->fail_read = true;
  EXPECT_FALSE(backing->CreateSkiaRepresentation(nullptr)->GetSkImage());
}

TEST(SkiaRasterImage, IOSurfaceSupportsSoftwareContext) {
  auto backing = fml::MakeRefCounted<IOSurfaceImageBacking>(
      SharedImageBacking::PixelFormat::kNative8888, skity::Vec2(3, 2),
      std::nullopt, kClayColorSpaceDisplayP3);
  auto representation = backing->CreateSkiaRepresentation(nullptr);
  ASSERT_TRUE(representation);
  auto image = representation->GetSkImage();
  ASSERT_TRUE(image);
  EXPECT_EQ(image->width(), 3);
  EXPECT_EQ(image->height(), 2);
  EXPECT_TRUE(SkColorSpace::Equals(
      image->colorSpace(), ToSkColorSpace(kClayColorSpaceDisplayP3).get()));
}

class RasterBitmapTest
    : public ::testing::TestWithParam<SharedImageSink::BufferMode> {};

TEST_P(RasterBitmapTest, FrozenBitmapDrainsBuffersAndKeepsItsSnapshot) {
  auto sink = fml::MakeRefCounted<SharedImageSinkManaged>(
      GetParam(), [](skity::Vec2, std::optional<GraphicsMemoryHandle>) {
        return fml::MakeRefCounted<PixelBacking>();
      });
  SharedImageExternalBitmap drawable(sink);
  drawable.SetFrameAvailableCallback([&] { drawable.MarkNewFrameAvailable(); });
  auto surface = SkSurface::MakeRasterN32Premul(1, 2);
  DrawableImage::PaintContext context;
  context.canvas = surface->getCanvas();
  auto paint = [&](bool freeze) {
    drawable.Paint(context, skity::Rect::MakeWH(1, 2), freeze,
                   GrSamplingOptions(), DrawableImage::FitMode::kClipToBounds);
    SkPixmap pixels;
    EXPECT_TRUE(surface->peekPixels(&pixels));
    return pixels.getColor(0, 0);
  };
  auto publish = [&](std::array<uint8_t, 8> pixels,
                     skity::Matrix transform = {}) {
    auto [backing, age, status] = sink->TryAcquireBack({1, 2});
    ASSERT_TRUE(backing);
    static_cast<PixelBacking*>(backing.get())->pixels = pixels;
    backing->SetTransformation(transform);
    ASSERT_TRUE(sink->SwapBack(nullptr));
  };
  publish({0, 0, 255, 255, 0, 0, 255, 255});
  EXPECT_EQ(paint(false), SK_ColorRED);
  for (int frame = 0; frame < 6; ++frame) {
    publish({255, 0, 0, 255, 255, 0, 0, 255});
    EXPECT_EQ(paint(true), SK_ColorRED);
  }
  EXPECT_EQ(paint(false), SK_ColorBLUE);
  publish({0, 255, 0, 255, 0, 255, 0, 255});
  EXPECT_EQ(paint(false), SK_ColorGREEN);

  // A frozen image also keeps the UV transform from its own frame.
  const std::array<uint8_t, 8> split{255, 0, 0, 255, 0, 0, 255, 255};
  publish(split);
  EXPECT_EQ(paint(false), SK_ColorRED);
  publish(split,
          skity::Matrix::Translate(0.f, 1.f) * skity::Matrix::Scale(1.f, -1.f));
  EXPECT_EQ(paint(true), SK_ColorRED);
  EXPECT_EQ(paint(false), SK_ColorBLUE);
  drawable.OnDrawableImageUnregistered();
}

INSTANTIATE_TEST_SUITE_P(BufferModes, RasterBitmapTest,
                         ::testing::Values(SharedImageSink::kSingleBuffer,
                                           SharedImageSink::kDoubleBuffer));

}  // namespace
}  // namespace clay
