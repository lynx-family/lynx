// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <CoreGraphics/CoreGraphics.h>
#import <IOSurface/IOSurface.h>

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterSurfaceManager.h"

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterSurface.h"

#include <array>
#include <tuple>

#include "clay/common/graphics/shared_image_external_texture.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/shell/platform/embedder/embedder_surface_metal.h"
#include "clay/shell/platform/headless/metal/clay_headless_renderer_metal.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#ifdef ENABLE_SKITY
#include "clay/gfx/skity/skity_image.h"
#include "skity/codec/codec.hpp"
#endif

#ifdef CLAY_TEST_EMBEDDER_API
#include "clay/lynx_adaptor/native_platform_view.h"
#include "platform/embedder/public/capi/lynx_native_view_capi.h"
#endif

namespace clay {
namespace {
using Config = std::tuple<ClayColorSpace, ClaySharedImageSinkBufferMode, bool>;

#ifdef CLAY_TEST_EMBEDDER_API
class NativeViewSource {
 public:
  explicit NativeViewSource(ClaySharedImageSinkBufferMode mode) : mode_(mode) {
    view_ = lynx_native_view_create(this);
    lynx_native_view_bind_is_surface_enabled(view_, [](auto*, void*) { return true; });
    lynx_native_view_bind_surface_buffer_mode(view_, [](auto*, void* data) {
      return static_cast<lynx_surface_buffer_mode_t>(static_cast<NativeViewSource*>(data)->mode_);
    });
    sink_ = reinterpret_cast<NativePlatformView*>(view_)->GetSharedImageSink();
  }
  ~NativeViewSource() {
    ClaySharedImageSinkSetFrameAvailableCallback(sink_, nullptr, nullptr);
    lynx_native_view_release(view_);
  }
  bool SetSpace(ClayColorSpace space) {
    return lynx_native_view_set_surface_color_space(
        view_, space == kClayColorSpaceDisplayP3 ? kLynxColorSpaceDisplayP3 : kLynxColorSpaceSRGB);
  }
  lynx_native_view_t* view() const { return view_; }
  ClaySharedImageSinkRef sink() const { return sink_; }

 private:
  ClaySharedImageSinkBufferMode mode_;
  lynx_native_view_t* view_ = nullptr;
  ClaySharedImageSinkRef sink_ = nullptr;
};
#endif

class DesktopColorSpaceTest : public ::testing::TestWithParam<Config> {
 protected:
  void SetUp() override {
    const auto [space, mode, partial] = GetParam();
    sink_ = ClayCreateSharedImageSinkWithColorSpace(mode, kClaySharedImageBackingTypeIOSurface,
                                                    kClaySharedImageBackingPixelFormatNative8888,
                                                    space);
    ASSERT_NE(sink_, nullptr);
    ClaySharedImageSinkSetFrameAvailableCallback(
        sink_, [](void*) {}, nullptr);
    ClayHardwareRendererConfig config = {};
    config.struct_size = sizeof(config);
    config.sink_ref = sink_;
    config.disable_partial_repaint = !partial;
    renderer_ = std::make_unique<ClayHeadlessRendererMetal>(nullptr, config);
    embedder_ = std::make_unique<EmbedderSurfaceMetal>(renderer_.get());
    surface_ = static_cast<OutputSurface*>(embedder_.get())->CreateGPUSurface(nullptr);
    ASSERT_NE(surface_, nullptr);
    ASSERT_TRUE(surface_->IsValid());
  }
  void TearDown() override {
    surface_.reset();
    embedder_.reset();
    if (renderer_) renderer_->CleanupGPUResources();
    renderer_.reset();
    ClaySharedImageSinkSetFrameAvailableCallback(sink_, nullptr, nullptr);
    ClayReleaseSharedImageSink(sink_);
  }
  ClaySharedImageRef Front() {
    ClaySharedImageRef image = nullptr;
    if (!ClaySharedImageSinkUpdateFront(sink_, nullptr, &image)) return nullptr;
    auto fence = ClaySharedImageGetFenceSync(image);
    EXPECT_TRUE(ClayFenceSyncClientWait(fence));
    ClayDestroyFenceSync(fence);
    return image;
  }
  void ExpectPixel(ClaySharedImageRef image, int x, int y, std::array<int, 4> rgba) {
    ASSERT_NE(image, nullptr);
    EXPECT_EQ(ClaySharedImageGetColorSpace(image), std::get<0>(GetParam()));
    ClaySharedImageNativeHandle handle = nullptr;
    ClaySharedImageGetBacking(image, nullptr, nullptr, &handle);
    auto io_surface = static_cast<IOSurfaceRef>(handle);
    auto name = IOSurfaceCopyValue(io_surface, CFSTR("IOSurfaceColorSpace"));
    ASSERT_NE(name, nullptr);
    EXPECT_TRUE(CFEqual(name, std::get<0>(GetParam()) == kClayColorSpaceDisplayP3
                                  ? kCGColorSpaceDisplayP3
                                  : kCGColorSpaceSRGB));
    CFRelease(name);
    ASSERT_EQ(IOSurfaceLock(io_surface, kIOSurfaceLockReadOnly, nullptr), kIOReturnSuccess);
    auto* pixel = static_cast<const uint8_t*>(IOSurfaceGetBaseAddress(io_surface)) +
                  y * IOSurfaceGetBytesPerRow(io_surface) + x * 4;
    EXPECT_NEAR(pixel[2], rgba[0], 2);
    EXPECT_NEAR(pixel[1], rgba[1], 2);
    EXPECT_NEAR(pixel[0], rgba[2], 2);
    EXPECT_NEAR(pixel[3], rgba[3], 1);
    EXPECT_EQ(IOSurfaceUnlock(io_surface, kIOSurfaceLockReadOnly, nullptr), kIOReturnSuccess);
  }
  void DrawRed(int size) {
    auto frame = surface_->AcquireFrame({size, size});
    ASSERT_NE(frame, nullptr);
    auto* canvas = frame->GetCanvas();
    ASSERT_NE(canvas, nullptr);
#ifdef ENABLE_SKITY
    canvas->Clear(0);
    skity::Paint paint;
    paint.SetColor(0xffff0000);
    canvas->DrawPaint(paint);
#else
    canvas->clear(0);
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    canvas->drawPaint(paint);
#endif
    frame->Prepare(std::nullopt);
    ASSERT_TRUE(frame->Submit());
  }
  ClaySharedImageSinkRef sink_ = nullptr;
  std::unique_ptr<ClayHeadlessRendererMetal> renderer_;
  std::unique_ptr<EmbedderSurfaceMetal> embedder_;
  std::unique_ptr<Surface> surface_;
};

TEST_P(DesktopColorSpaceTest, BufferRotationAndResize) {
  const auto red = std::get<0>(GetParam()) == kClayColorSpaceDisplayP3
                       ? std::array<int, 4>{234, 51, 35, 255}
                       : std::array<int, 4>{255, 0, 0, 255};
  for (int i = 0; i < 8; ++i) {
    int size = i < 4 ? 16 : 24;
    DrawRed(size);
    ExpectPixel(Front(), 8, 8, red);
  }
}

TEST_P(DesktopColorSpaceTest, PartialUpdatePreservesPixelsAcrossBuffers) {
  const int buffers = std::get<1>(GetParam()) == kClaySharedImageSinkBufferModeTripleBuffer ? 3 : 2;
  // Queue every backing before reading so triple buffering actually allocates
  // three distinct images instead of alternating between two reusable images.
  for (int i = 0; i < buffers; ++i) DrawRed(16);
  for (int i = 0; i < buffers; ++i) ASSERT_NE(Front(), nullptr);
  const bool p3 = std::get<0>(GetParam()) == kClayColorSpaceDisplayP3;
  for (int i = 0; i < 12; ++i) {
    auto frame = surface_->AcquireFrame({16, 16});
    ASSERT_NE(frame, nullptr);
    auto damage = skity::Rect::MakeLTRB(4, 4, 12, 12);
    frame->Prepare(damage);
    SurfaceFrame::SubmitInfo info;
    info.frame_damage = damage;
    info.buffer_damage = damage;
    frame->set_submit_info(info);
    auto* canvas = frame->GetCanvas();
    // With partial repaint disabled the renderer uses MSAA and the caller
    // must repaint the full target; previous resolve contents are not loaded.
    if (!std::get<2>(GetParam())) {
#ifdef ENABLE_SKITY
      skity::Paint background;
      background.SetColor(0xffff0000);
      canvas->DrawPaint(background);
#else
      SkPaint background;
      background.setColor(SK_ColorRED);
      canvas->drawPaint(background);
#endif
    }
#ifdef ENABLE_SKITY
    canvas->Save();
    canvas->ClipRect(damage);
    skity::Paint paint;
    paint.SetColor(i % 2 ? 0xff0000ff : 0xff00ff00);
    canvas->DrawPaint(paint);
    canvas->Restore();
#else
    canvas->save();
    canvas->clipRect(SkRect::MakeLTRB(4, 4, 12, 12));
    SkPaint paint;
    paint.setColor(i % 2 ? SK_ColorBLUE : SK_ColorGREEN);
    canvas->drawPaint(paint);
    canvas->restore();
#endif
    ASSERT_TRUE(frame->Submit());
    auto image = Front();
    ExpectPixel(image, 2, 2,
                p3 ? std::array<int, 4>{234, 51, 35, 255} : std::array<int, 4>{255, 0, 0, 255});
    ExpectPixel(
        image, 8, 8,
        i % 2 ? (p3 ? std::array<int, 4>{0, 0, 245, 255} : std::array<int, 4>{0, 0, 255, 255})
              : (p3 ? std::array<int, 4>{117, 251, 76, 255} : std::array<int, 4>{0, 255, 0, 255}));
  }
}

TEST_P(DesktopColorSpaceTest, NativeViewExternalTextureConvertsSourceToOutput) {
  auto source = ClayCreateSharedImageSinkWithColorSpace(
      kClaySharedImageSinkBufferModeDoubleBuffer, kClaySharedImageBackingTypeIOSurface,
      kClaySharedImageBackingPixelFormatNative8888, kClayColorSpaceDisplayP3);
  ASSERT_NE(source, nullptr);
  auto external = std::make_unique<SharedImageExternalTexture>(
      fml::Ref(reinterpret_cast<SharedImageSink*>(source)));
  external->SetFrameAvailableCallback([&] { external->MarkNewFrameAvailable(); });
  ClaySize size = {16, 16};
  ClaySharedImageRef backing = nullptr;
  ASSERT_TRUE(ClaySharedImageSinkAcquireBack(source, &size, &backing, nullptr));
  ClaySharedImageNativeHandle handle = nullptr;
  ClaySharedImageGetBacking(backing, nullptr, nullptr, &handle);
  auto io_surface = static_cast<IOSurfaceRef>(handle);
  ASSERT_EQ(IOSurfaceLock(io_surface, 0, nullptr), kIOReturnSuccess);
  for (int y = 0; y < 16; ++y) {
    auto* row = static_cast<uint8_t*>(IOSurfaceGetBaseAddress(io_surface)) +
                y * IOSurfaceGetBytesPerRow(io_surface);
    for (int x = 0; x < 16; ++x) {
      row[x * 4] = 32;
      row[x * 4 + 1] = 64;
      row[x * 4 + 2] = 128;
      row[x * 4 + 3] = 255;
    }
  }
  ASSERT_EQ(IOSurfaceUnlock(io_surface, 0, nullptr), kIOReturnSuccess);
  ASSERT_TRUE(ClaySharedImageSinkSwapBack(source, nullptr));
  auto frame = surface_->AcquireFrame({16, 16});
  ASSERT_NE(frame, nullptr);
  frame->Prepare(std::nullopt);
  DrawableImage::PaintContext context{.canvas = frame->GetCanvas(),
                                      .gr_context = surface_->GetContext()};
  external->Paint(context, skity::Rect::MakeWH(16, 16), false, GrSamplingOptions(),
                  DrawableImage::FitMode::kScaleToFill);
  ASSERT_TRUE(frame->Submit());
  // ColorSync is the independent reference for the source-to-target gamut conversion.
  auto p3 = CGColorSpaceCreateWithName(kCGColorSpaceDisplayP3);
  auto destination = CGColorSpaceCreateWithName(std::get<0>(GetParam()) == kClayColorSpaceDisplayP3
                                                    ? kCGColorSpaceDisplayP3
                                                    : kCGColorSpaceSRGB);
  CGFloat components[] = {128.0 / 255, 64.0 / 255, 32.0 / 255, 1};
  auto color = CGColorCreate(p3, components);
  auto converted = CGColorCreateCopyByMatchingToColorSpace(
      destination, kCGRenderingIntentRelativeColorimetric, color, nullptr);
  ASSERT_NE(converted, nullptr);
  const auto* expected = CGColorGetComponents(converted);
  ExpectPixel(Front(), 8, 8,
              {static_cast<int>(expected[0] * 255 + 0.5), static_cast<int>(expected[1] * 255 + 0.5),
               static_cast<int>(expected[2] * 255 + 0.5), 255});
  CGColorRelease(converted);
  CGColorRelease(color);
  CGColorSpaceRelease(destination);
  CGColorSpaceRelease(p3);
  external->OnGrContextDestroyed();
  external.reset();
  ClayReleaseSharedImageSink(source);
}

#ifdef CLAY_TEST_EMBEDDER_API
TEST_P(DesktopColorSpaceTest, NativeViewDynamicallySwitchesSourceEncoding) {
  NativeViewSource source(std::get<1>(GetParam()));
  auto external = std::make_unique<SharedImageExternalTexture>(
      fml::Ref(reinterpret_cast<SharedImageSink*>(source.sink())));
  external->SetFrameAvailableCallback([&] { external->MarkNewFrameAvailable(); });
  for (int i = 0; i < 18; ++i) {
    auto space = (i / 3) % 2 ? kClayColorSpaceDisplayP3 : kClayColorSpaceSRGB;
    ASSERT_TRUE(source.SetSpace(space));
    EXPECT_EQ(reinterpret_cast<NativePlatformView*>(source.view())->GetSharedImageSink(),
              source.sink());
    int size = i < 9 ? 16 : 24;
    auto io_surface =
        reinterpret_cast<IOSurfaceRef>(lynx_native_view_acquire_surface(source.view(), size, size));
    ASSERT_NE(io_surface, nullptr);
    auto name = IOSurfaceCopyValue(io_surface, CFSTR("IOSurfaceColorSpace"));
    ASSERT_NE(name, nullptr);
    EXPECT_TRUE(CFEqual(
        name, space == kClayColorSpaceDisplayP3 ? kCGColorSpaceDisplayP3 : kCGColorSpaceSRGB));
    CFRelease(name);
    ASSERT_EQ(IOSurfaceLock(io_surface, 0, nullptr), kIOReturnSuccess);
    int alpha = i % 2 ? 128 : 255;
    for (int y = 0; y < size; ++y) {
      auto* row = static_cast<uint8_t*>(IOSurfaceGetBaseAddress(io_surface)) +
                  y * IOSurfaceGetBytesPerRow(io_surface);
      for (int x = 0; x < size; ++x) {
        row[x * 4] = alpha == 255 ? 32 : 16;
        row[x * 4 + 1] = alpha == 255 ? 64 : 32;
        row[x * 4 + 2] = alpha == 255 ? 128 : 64;
        row[x * 4 + 3] = alpha;
      }
    }
    ASSERT_EQ(IOSurfaceUnlock(io_surface, 0, nullptr), kIOReturnSuccess);
    ASSERT_TRUE(lynx_native_view_swap_back(source.view()));
    auto frame = surface_->AcquireFrame({16, 16});
    ASSERT_NE(frame, nullptr);
    frame->Prepare(std::nullopt);
    auto* canvas = frame->GetCanvas();
#ifdef ENABLE_SKITY
    canvas->Clear(0);
#else
    canvas->clear(0);
#endif
    DrawableImage::PaintContext context{.canvas = canvas, .gr_context = surface_->GetContext()};
    external->Paint(context, skity::Rect::MakeWH(16, 16), false, GrSamplingOptions(),
                    DrawableImage::FitMode::kScaleToFill);
    ASSERT_TRUE(frame->Submit());
    auto src = CGColorSpaceCreateWithName(space == kClayColorSpaceDisplayP3 ? kCGColorSpaceDisplayP3
                                                                            : kCGColorSpaceSRGB);
    auto dst = CGColorSpaceCreateWithName(std::get<0>(GetParam()) == kClayColorSpaceDisplayP3
                                              ? kCGColorSpaceDisplayP3
                                              : kCGColorSpaceSRGB);
    CGFloat components[] = {128.0 / 255, 64.0 / 255, 32.0 / 255, 1};
    auto color = CGColorCreate(src, components);
    auto converted = CGColorCreateCopyByMatchingToColorSpace(
        dst, kCGRenderingIntentRelativeColorimetric, color, nullptr);
    ASSERT_NE(converted, nullptr);
    const auto* expected = CGColorGetComponents(converted);
    ExpectPixel(
        Front(), 8, 8,
        {static_cast<int>(expected[0] * alpha + .5), static_cast<int>(expected[1] * alpha + .5),
         static_cast<int>(expected[2] * alpha + .5), alpha});
    CGColorRelease(converted);
    CGColorRelease(color);
    CGColorSpaceRelease(dst);
    CGColorSpaceRelease(src);
  }
  external->OnGrContextDestroyed();
}

TEST_P(DesktopColorSpaceTest, MetalProducerSwitchResetsDamageAndTargetEncoding) {
  NativeViewSource source(std::get<1>(GetParam()));
  ClaySharedImageSinkSetFrameAvailableCallback(
      source.sink(), [](void*) {}, nullptr);
  ClayHardwareRendererConfig config{};
  config.struct_size = sizeof(config);
  config.sink_ref = source.sink();
  config.disable_partial_repaint = false;
  auto producer = std::make_unique<ClayHeadlessRendererMetal>(nullptr, config);
  auto embedder = std::make_unique<EmbedderSurfaceMetal>(producer.get());
  auto surface = static_cast<OutputSurface*>(embedder.get())->CreateGPUSurface(nullptr);
  ASSERT_TRUE(surface && surface->IsValid());
  for (int i = 0; i < 12; ++i) {
    bool p3 = (i / 3) % 2;
    ASSERT_TRUE(source.SetSpace(p3 ? kClayColorSpaceDisplayP3 : kClayColorSpaceSRGB));
    if (i == 2) {
      // A request can return to the original space before any intervening frame
      // is drawn. Its new buffer still has no valid damage history.
      ASSERT_TRUE(source.SetSpace(kClayColorSpaceDisplayP3));
      ASSERT_TRUE(source.SetSpace(kClayColorSpaceSRGB));
    }
    auto frame = surface->AcquireFrame({16, 16});
    ASSERT_NE(frame, nullptr);
    EXPECT_EQ(frame->framebuffer_info().color_space_changed, i > 0 && i % 3 == 0);
    if (i % 3 == 0 || i == 2) EXPECT_FALSE(frame->framebuffer_info().existing_damage.has_value());
    auto* canvas = frame->GetCanvas();
#ifdef ENABLE_SKITY
    canvas->Clear(0);
    skity::Paint paint;
    paint.SetColor(0xffff0000);
    canvas->DrawPaint(paint);
#else
    canvas->clear(0);
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    canvas->drawPaint(paint);
#endif
    frame->Prepare(frame->framebuffer_info().existing_damage.has_value()
                       ? std::optional<skity::Rect>(skity::Rect::MakeWH(4, 4))
                       : std::nullopt);
    ASSERT_TRUE(frame->Submit());
    ClaySharedImageRef front = nullptr;
    ASSERT_TRUE(ClaySharedImageSinkUpdateFront(source.sink(), nullptr, &front));
    auto fence = ClaySharedImageGetFenceSync(front);
    ASSERT_TRUE(ClayFenceSyncClientWait(fence));
    ClayDestroyFenceSync(fence);
    EXPECT_EQ(ClaySharedImageGetColorSpace(front),
              p3 ? kClayColorSpaceDisplayP3 : kClayColorSpaceSRGB);
    ClaySharedImageNativeHandle handle = nullptr;
    ClaySharedImageGetBacking(front, nullptr, nullptr, &handle);
    auto io = static_cast<IOSurfaceRef>(handle);
    ASSERT_EQ(IOSurfaceLock(io, kIOSurfaceLockReadOnly, nullptr), kIOReturnSuccess);
    auto* pixel = static_cast<const uint8_t*>(IOSurfaceGetBaseAddress(io)) +
                  8 * IOSurfaceGetBytesPerRow(io) + 8 * 4;
    EXPECT_NEAR(pixel[2], p3 ? 234 : 255, 2);
    EXPECT_NEAR(pixel[1], p3 ? 51 : 0, 2);
    EXPECT_NEAR(pixel[0], p3 ? 35 : 0, 2);
    EXPECT_EQ(IOSurfaceUnlock(io, kIOSurfaceLockReadOnly, nullptr), kIOReturnSuccess);
  }
  surface.reset();
  embedder.reset();
  producer->CleanupGPUResources();
}
#endif

INSTANTIATE_TEST_SUITE_P(
    Metal, DesktopColorSpaceTest,
    ::testing::Combine(::testing::Values(kClayColorSpaceSRGB, kClayColorSpaceDisplayP3),
                       ::testing::Values(kClaySharedImageSinkBufferModeDoubleBuffer,
                                         kClaySharedImageSinkBufferModeTripleBuffer),
                       ::testing::Bool()));

TEST(DesktopColorSpaceAPI, DefaultsAndValidation) {
  ClaySize size = {16, 16};
  auto image = ClayCreateSharedImage(kClaySharedImageBackingTypeIOSurface,
                                     kClaySharedImageBackingPixelFormatNative8888, &size);
  ASSERT_NE(image, nullptr);
  EXPECT_EQ(ClaySharedImageGetColorSpace(image), kClayColorSpaceSRGB);
  ClayReleaseSharedImage(image);
  EXPECT_EQ(ClayCreateSharedImageWithColorSpace(kClaySharedImageBackingTypeIOSurface,
                                                kClaySharedImageBackingPixelFormatNative8888, &size,
                                                static_cast<ClayColorSpace>(99)),
            nullptr);
}
TEST(DesktopColorSpaceAPI, ImportsExistingIOSurfaceWithUnspecifiedSize) {
  ClaySize extent = {16, 16};
  auto image = ClayCreateSharedImage(kClaySharedImageBackingTypeIOSurface,
                                     kClaySharedImageBackingPixelFormatNative8888, &extent);
  ASSERT_NE(image, nullptr);
  ClaySharedImageNativeHandle handle = nullptr;
  ClaySharedImageGetBacking(image, nullptr, nullptr, &handle);
  ASSERT_NE(handle, nullptr);
  ClaySize unspecified = {};
  auto imported = ClayCreateSharedImageFromHandle(kClaySharedImageBackingTypeIOSurface,
                                                  kClaySharedImageBackingPixelFormatNative8888,
                                                  &unspecified, handle);
  ClayReleaseSharedImage(image);
  ASSERT_NE(imported, nullptr);
  ClaySharedImageNativeHandle retained = nullptr;
  ClaySharedImageGetBacking(imported, nullptr, nullptr, &retained);
  EXPECT_EQ(retained, handle);
  ClayReleaseSharedImage(imported);
}
}  // namespace
}  // namespace clay

namespace clay {
TEST(DesktopColorSpaceWindow, IOSurfaceAndRenderTargetAgree) {
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  ASSERT_NE(device, nil);
  for (auto space : {kClayColorSpaceSRGB, kClayColorSpaceDisplayP3}) {
    FlutterSurface* surface = [[FlutterSurface alloc] initWithSize:CGSizeMake(16, 16)
                                                            device:device
                                                        colorSpace:space];
    auto info = surface.asGPUMTLTextureInfo;
    EXPECT_EQ(info.color_space, space);
    CFTypeRef name = IOSurfaceCopyValue(surface.ioSurface, CFSTR("IOSurfaceColorSpace"));
    ASSERT_NE(name, nullptr);
    EXPECT_TRUE(CFEqual(
        name, space == kClayColorSpaceDisplayP3 ? kCGColorSpaceDisplayP3 : kCGColorSpaceSRGB));
    CFRelease(name);
    info.destruction_callback(info.destruction_context);
  }
}
}  // namespace clay

#ifdef CLAY_TEST_EMBEDDER_API
namespace clay {
TEST(DesktopColorSpaceNativeViewAPI, SingleBufferChangesAfterReaderReleasesFront) {
  NativeViewSource source(kClaySharedImageSinkBufferModeSingleBuffer);
  ClaySharedImageSinkSetFrameAvailableCallback(
      source.sink(), [](void*) {}, nullptr);
  auto* sink = reinterpret_cast<SharedImageSink*>(source.sink());
  ASSERT_NE(lynx_native_view_acquire_surface(source.view(), 16, 16), nullptr);
  EXPECT_FALSE(source.SetSpace(kClayColorSpaceDisplayP3));
  ASSERT_TRUE(lynx_native_view_swap_back(source.view()));
  auto previous = sink->UpdateFront(nullptr);
  ASSERT_TRUE(previous);
  ASSERT_TRUE(source.SetSpace(kClayColorSpaceDisplayP3));
  EXPECT_EQ(previous->GetColorSpace(), kClayColorSpaceSRGB);
  sink->ReleaseFront(nullptr);
  ASSERT_NE(lynx_native_view_acquire_surface(source.view(), 16, 16), nullptr);
  ASSERT_TRUE(lynx_native_view_swap_back(source.view()));
  auto next = sink->UpdateFront(nullptr);
  ASSERT_TRUE(next);
  EXPECT_NE(next, previous);
  EXPECT_EQ(next->GetColorSpace(), kClayColorSpaceDisplayP3);
}

TEST(DesktopColorSpaceNativeViewAPI, QueuedFramesKeepTheirOriginalEncoding) {
  NativeViewSource source(kClaySharedImageSinkBufferModeTripleBuffer);
  ClaySharedImageSinkSetFrameAvailableCallback(
      source.sink(), [](void*) {}, nullptr);
  auto* sink = reinterpret_cast<SharedImageSink*>(source.sink());
  ASSERT_TRUE(source.SetSpace(kClayColorSpaceSRGB));
  ASSERT_NE(lynx_native_view_acquire_surface(source.view(), 16, 16), nullptr);
  ASSERT_TRUE(lynx_native_view_swap_back(source.view()));
  ASSERT_TRUE(source.SetSpace(kClayColorSpaceDisplayP3));
  ASSERT_NE(lynx_native_view_acquire_surface(source.view(), 16, 16), nullptr);
  ASSERT_TRUE(lynx_native_view_swap_back(source.view()));

  auto srgb = sink->UpdateFront(nullptr);
  ASSERT_TRUE(srgb);
  EXPECT_EQ(srgb->GetColorSpace(), kClayColorSpaceSRGB);
  auto p3 = sink->UpdateFront(nullptr);
  ASSERT_TRUE(p3);
  EXPECT_EQ(p3->GetColorSpace(), kClayColorSpaceDisplayP3);
  EXPECT_EQ(srgb->GetColorSpace(), kClayColorSpaceSRGB);

  ASSERT_TRUE(source.SetSpace(kClayColorSpaceSRGB));
  auto next = lynx_native_view_acquire_surface(source.view(), 16, 16);
  ASSERT_NE(next, nullptr);
  EXPECT_NE(reinterpret_cast<void*>(next), srgb->GetGFXHandle());
  EXPECT_NE(reinterpret_cast<void*>(next), p3->GetGFXHandle());
  ASSERT_TRUE(lynx_native_view_swap_back(source.view()));
  auto changed = sink->UpdateFront(nullptr);
  ASSERT_TRUE(changed);
  EXPECT_EQ(changed->GetColorSpace(), kClayColorSpaceSRGB);
}

TEST(DesktopColorSpaceNativeViewAPI, ImportedHandlesKeepTheirDeclaredSpace) {
  NativeViewSource source(kClaySharedImageSinkBufferModeDoubleBuffer);
  ClaySharedImageSinkSetFrameAvailableCallback(
      source.sink(), [](void*) {}, nullptr);
  auto* sink = reinterpret_cast<SharedImageSink*>(source.sink());
  ClaySize size{16, 16};
  for (auto space : {kClayColorSpaceSRGB, kClayColorSpaceDisplayP3, kClayColorSpaceSRGB}) {
    ASSERT_TRUE(source.SetSpace(space));
    auto image = ClayCreateSharedImageWithColorSpace(kClaySharedImageBackingTypeIOSurface,
                                                     kClaySharedImageBackingPixelFormatNative8888,
                                                     &size, space);
    ASSERT_NE(image, nullptr);
    ClaySharedImageNativeHandle handle = nullptr;
    ClaySharedImageGetBacking(image, nullptr, nullptr, &handle);
    ASSERT_TRUE(lynx_native_view_present_surface(source.view(), 16, 16, nullptr,
                                                 reinterpret_cast<lynx_surface_handle_t*>(handle)));
    auto front = sink->UpdateFront(nullptr);
    ASSERT_TRUE(front);
    EXPECT_EQ(front->GetColorSpace(), space);
    EXPECT_EQ(front->GetGFXHandle(), handle);
    ClayReleaseSharedImage(image);
  }
}
}  // namespace clay

TEST(DesktopColorSpaceNativeViewAPI, SourceSpaceRejectsChangesDuringWriteAndMatchesIOSurface) {
  for (auto space : {kLynxColorSpaceSRGB, kLynxColorSpaceDisplayP3}) {
    auto view = lynx_native_view_create(nullptr);
    ASSERT_NE(view, nullptr);
    lynx_native_view_bind_is_surface_enabled(view, [](lynx_native_view_t*, void*) { return true; });
    EXPECT_EQ(lynx_native_view_get_surface_color_space(view), kLynxColorSpaceSRGB);
    EXPECT_TRUE(lynx_native_view_set_surface_color_space(view, space));
    auto handle = lynx_native_view_acquire_surface(view, 16, 16);
    ASSERT_NE(handle, nullptr);
    auto name =
        IOSurfaceCopyValue(reinterpret_cast<IOSurfaceRef>(handle), CFSTR("IOSurfaceColorSpace"));
    ASSERT_NE(name, nullptr);
    EXPECT_TRUE(CFEqual(
        name, space == kLynxColorSpaceDisplayP3 ? kCGColorSpaceDisplayP3 : kCGColorSpaceSRGB));
    CFRelease(name);
    auto other = space == kLynxColorSpaceSRGB ? kLynxColorSpaceDisplayP3 : kLynxColorSpaceSRGB;
    EXPECT_FALSE(lynx_native_view_set_surface_color_space(view, other));
    EXPECT_EQ(lynx_native_view_get_surface_color_space(view), space);
    lynx_native_view_release(view);
  }
}
#endif

@interface P3TestSurfaceDelegate : NSObject <FlutterSurfaceManagerDelegate>
@end
@implementation P3TestSurfaceDelegate
- (void)onPresent:(CGSize)frameSize withBlock:(dispatch_block_t)block {
  block();
}
@end

namespace clay {
namespace {
class WindowMetalDelegate final : public EmbedderSurfaceMetalDelegate {
 public:
  WindowMetalDelegate(id<MTLDevice> device, id<MTLCommandQueue> queue,
                      FlutterSurfaceManager* manager, ClayColorSpace space)
      : device_(device), queue_(queue), manager_(manager), space_(space) {}
  GPUMTLDeviceHandle GetMTLDevice() const override { return (__bridge void*)device_; }
  GPUMTLCommandQueueHandle GetMTLCommandQueue() const override { return (__bridge void*)queue_; }
  GPUMTLTextureInfo GetMTLTexture(const skity::Vec2& size) const override {
    return
        [manager_ surfaceForSize:CGSizeMake(size.x, size.y) colorSpace:space_].asGPUMTLTextureInfo;
  }
  bool PresentTexture(GPUMTLTextureInfo texture) const override {
    auto info = [[FlutterSurfacePresentInfo alloc] init];
    info.surface = [FlutterSurface fromGPUMTLTextureInfo:&texture];
    [manager_ present:@[ info ] notify:nil];
    return true;
  }
  bool EnablePartialRepaint() const override { return true; }

 private:
  id<MTLDevice> device_;
  id<MTLCommandQueue> queue_;
  FlutterSurfaceManager* manager_;
  ClayColorSpace space_;
};
}  // namespace

TEST(DesktopColorSpaceWindow, PresentsTaggedSurfaceThroughCoreAnimation) {
  [NSApplication sharedApplication];
  auto window = [[NSWindow alloc] initWithContentRect:NSMakeRect(100, 100, 160, 160)
                                            styleMask:NSWindowStyleMaskTitled
                                              backing:NSBackingStoreBuffered
                                                defer:NO];
  window.releasedWhenClosed = NO;
  window.title = @"Clay Display P3 validation";
  window.contentView.wantsLayer = YES;
  [window orderFront:nil];
  id<MTLDevice> device = MTLCreateSystemDefaultDevice();
  ASSERT_NE(device, nil);
  id<MTLCommandQueue> queue = [device newCommandQueue];
  auto observer = [[P3TestSurfaceDelegate alloc] init];
  auto manager = [[FlutterSurfaceManager alloc] initWithDevice:device
                                                  commandQueue:queue
                                                         layer:window.contentView.layer
                                                      delegate:observer];
  for (auto space : {kClayColorSpaceSRGB, kClayColorSpaceDisplayP3}) {
    WindowMetalDelegate delegate(device, queue, manager, space);
    EmbedderSurfaceMetal embedder(&delegate);
    auto surface = static_cast<OutputSurface*>(&embedder)->CreateGPUSurface(nullptr);
    ASSERT_NE(surface, nullptr);
    auto frame = surface->AcquireFrame({160, 160});
    ASSERT_NE(frame, nullptr);
    frame->Prepare(std::nullopt);
#ifdef ENABLE_SKITY
    skity::Paint paint;
    paint.SetColor(0xffff0000);
    frame->GetCanvas()->DrawPaint(paint);
#else
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    frame->GetCanvas()->drawPaint(paint);
#endif
    ASSERT_TRUE(frame->Submit());
    auto fence = [queue commandBuffer];
    [fence commit];
    [fence waitUntilCompleted];
    ASSERT_EQ(manager.frontSurfaces.count, 1u);
    auto front = manager.frontSurfaces.firstObject;
    EXPECT_EQ(front.colorSpace, space);
    ASSERT_EQ(manager.layers.count, 1u);
    EXPECT_EQ(manager.layers.firstObject.contents, (__bridge id)front.ioSurface);
    ASSERT_EQ(IOSurfaceLock(front.ioSurface, kIOSurfaceLockReadOnly, nullptr), kIOReturnSuccess);
    auto* pixel = static_cast<const uint8_t*>(IOSurfaceGetBaseAddress(front.ioSurface));
    EXPECT_NEAR(pixel[2], space == kClayColorSpaceDisplayP3 ? 234 : 255, 2);
    EXPECT_EQ(IOSurfaceUnlock(front.ioSurface, kIOSurfaceLockReadOnly, nullptr), kIOReturnSuccess);
    [CATransaction flush];
    [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.05]];
  }
  [window close];
}
}  // namespace clay

#ifdef ENABLE_SKITY
TEST(DesktopColorSpaceCodec, PngPreservesP3PrimariesAndResizing) {
  auto pixels = std::make_shared<skity::Pixmap>(16, 16, skity::AlphaType::kUnpremul_AlphaType,
                                                skity::ColorType::kRGBA);
  pixels->SetColorInfo(pixels->GetAlphaType(), pixels->GetColorType(),
                       skity::ColorSpace::DisplayP3());
  for (int y = 0; y < 16; ++y) {
    for (int x = 0; x < 16; ++x) {
      auto* p = pixels->WritableAddr8(x, y);
      p[0] = y < 8 ? 255 : 0;
      p[1] = 0;
      p[2] = y < 8 ? 0 : 255;
      p[3] = y < 8 ? 255 : 128;
    }
  }
  auto encoded = skity::Codec::MakePngCodec()->Encode(pixels.get());
  ASSERT_NE(encoded, nullptr);
  auto codec = skity::Codec::MakeFromData(encoded);
  ASSERT_NE(codec, nullptr);
  codec->SetData(encoded);
  auto decoded = codec->Decode();
  ASSERT_NE(decoded, nullptr);
  ASSERT_TRUE(decoded->GetColorSpace());
  for (int i = 0; i < 9; ++i) {
    EXPECT_NEAR(decoded->GetColorSpace()->gamut_xyz[i],
                skity::ColorSpace::DisplayP3()->gamut_xyz[i], 0.001);
  }
  EXPECT_EQ(decoded->Addr8(0, 0)[0], 255);
  EXPECT_EQ(decoded->Addr8(0, 15)[2], 255);
  EXPECT_EQ(decoded->Addr8(0, 15)[3], 128);
  auto image = skity::Image::MakeImage(decoded);
  auto scaled = std::make_shared<skity::Pixmap>(8, 8);
  scaled->SetColorInfo(scaled->GetAlphaType(), scaled->GetColorType(), decoded->GetColorSpace());
  ASSERT_TRUE(image->ScalePixels(scaled, nullptr, skity::SamplingOptions()));
  EXPECT_EQ(scaled->GetColorSpace(), decoded->GetColorSpace());
  EXPECT_EQ(scaled->Addr8(4, 0)[0], 255);
  EXPECT_EQ(scaled->Addr8(4, 7)[2], 255);
}
#endif
