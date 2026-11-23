// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/shared_image_sink.h"
#include "clay/lynx_adaptor/native_platform_view.h"
#include "platform/embedder/lynx_view_builder_priv.h"
#include "platform/embedder/native_view/lynx_texture_view.h"
#include "platform/embedder/public/lynx_view.h"

#ifdef _WIN32
#include <windows.h>

#include <cstdint>
#elif defined(__APPLE__)
#include <IOSurface/IOSurface.h>
#endif

// cspell:ignore setsurfacecolorspace
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {

using lynx::pub::LynxValue;

TEST(LynxViewBuilderColorSpaceTest, SelectsDesktopWindowOutput) {
  lynx::pub::LynxView::Builder builder;
  auto* config = builder.Impl();
  EXPECT_EQ(config->color_space, kLynxColorSpaceSRGB);
  EXPECT_EQ(config->native_view_creators.count("x-texture-view"), 1u);
  builder.SetColorSpace(kLynxColorSpaceDisplayP3);
  EXPECT_EQ(config->color_space, kLynxColorSpaceDisplayP3);
  EXPECT_FALSE(lynx_view_builder_set_color_space(nullptr, kLynxColorSpaceSRGB));
  EXPECT_TRUE(lynx_view_builder_set_color_space(config, kLynxColorSpaceSRGB));
  EXPECT_EQ(config->color_space, kLynxColorSpaceSRGB);
}

class TextureViewColorSpaceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    view_ = new LynxTextureView(nullptr);
    sink_ref_ = reinterpret_cast<NativePlatformView*>(view_->native_view())
                    ->GetSharedImageSink();
    ASSERT_NE(sink_ref_, nullptr);
    ClaySharedImageSinkSetFrameAvailableCallback(
        sink_ref_, [](void*) {}, nullptr);
    sink_ = reinterpret_cast<SharedImageSink*>(sink_ref_);
  }

  void TearDown() override {
    if (sink_ref_) {
      ClaySharedImageSinkSetFrameAvailableCallback(sink_ref_, nullptr, nullptr);
    }
    if (view_) lynx_native_view_release(view_->native_view());
  }

  int SetSpace(const LynxValue& value) {
    LynxValue params(LynxValue::kCreateAsMapTag);
    params.SetProperty("colorSpace", value);
    int result = LynxTextureView::kUnknown;
    view_->OnMethodInvoked(
        "setsurfacecolorspace", params, [&](int code, LynxValue&& data) {
          result = code;
          if (code != LynxTextureView::kInvalidParameter) {
            EXPECT_EQ(data.Bool(), code == LynxTextureView::kSuccess);
          }
        });
    return result;
  }

  LynxTextureView* view_ = nullptr;
  ClaySharedImageSinkRef sink_ref_ = nullptr;
  SharedImageSink* sink_ = nullptr;
};

TEST_F(TextureViewColorSpaceTest, ValidatesSourceSpaceAfterSinkCreation) {
  EXPECT_EQ(view_->GetSurfaceColorSpace(), kLynxColorSpaceSRGB);
  EXPECT_EQ(SetSpace(LynxValue("display-p3")), LynxTextureView::kSuccess);
  EXPECT_EQ(view_->GetSurfaceColorSpace(), kLynxColorSpaceDisplayP3);
  EXPECT_EQ(SetSpace(LynxValue("adobe-rgb")),
            LynxTextureView::kInvalidParameter);
  EXPECT_EQ(SetSpace(LynxValue(1)), LynxTextureView::kInvalidParameter);
  EXPECT_EQ(view_->GetSurfaceColorSpace(), kLynxColorSpaceDisplayP3);
  EXPECT_EQ(SetSpace(LynxValue("srgb")), LynxTextureView::kSuccess);
  EXPECT_EQ(view_->GetSurfaceColorSpace(), kLynxColorSpaceSRGB);
}

TEST_F(TextureViewColorSpaceTest, RejectsEncodingChangeDuringAnAcquiredFrame) {
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  EXPECT_EQ(SetSpace(LynxValue("display-p3")),
            LynxTextureView::kInvalidStateError);
  EXPECT_EQ(view_->GetSurfaceColorSpace(), kLynxColorSpaceSRGB);
  EXPECT_EQ(SetSpace(LynxValue("srgb")), LynxTextureView::kSuccess);
  bool swapped = false;
  view_->OnMethodInvoked("swapbuffer", LynxValue(LynxValue::kCreateAsMapTag),
                         [&](int code, LynxValue&& data) {
                           EXPECT_EQ(code, LynxTextureView::kSuccess);
                           swapped = data.Bool();
                         });
  ASSERT_TRUE(swapped);
  EXPECT_EQ(SetSpace(LynxValue("display-p3")), LynxTextureView::kSuccess);
  auto previous = sink_->UpdateFront(nullptr);
  ASSERT_TRUE(previous);
  EXPECT_EQ(previous->GetColorSpace(), kClayColorSpaceSRGB);
}

TEST_F(TextureViewColorSpaceTest,
       SwitchesBothDirectionsWithoutRelabelingQueuedFrames) {
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  ASSERT_TRUE(view_->SwapBack());
  ASSERT_EQ(SetSpace(LynxValue("display-p3")), LynxTextureView::kSuccess);
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  ASSERT_TRUE(view_->SwapBack());
  auto original = sink_->UpdateFront(nullptr);
  ASSERT_TRUE(original);
  EXPECT_EQ(original->GetColorSpace(), kClayColorSpaceSRGB);
  auto p3 = sink_->UpdateFront(nullptr);
  ASSERT_TRUE(p3);
  EXPECT_EQ(p3->GetColorSpace(), kClayColorSpaceDisplayP3);
  ASSERT_EQ(SetSpace(LynxValue("srgb")), LynxTextureView::kSuccess);
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  ASSERT_TRUE(view_->SwapBack());
  auto restored = sink_->UpdateFront(nullptr);
  ASSERT_TRUE(restored);
  EXPECT_NE(restored, original);
  EXPECT_EQ(restored->GetColorSpace(), kClayColorSpaceSRGB);
  EXPECT_EQ(p3->GetColorSpace(), kClayColorSpaceDisplayP3);
  EXPECT_EQ(reinterpret_cast<NativePlatformView*>(view_->native_view())
                ->GetSharedImageSink(),
            sink_ref_);
}

TEST_F(TextureViewColorSpaceTest, FailedP3FactoryDoesNotBlockSwitchingBack) {
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  ASSERT_TRUE(view_->SwapBack());
  auto original = sink_->UpdateFront(nullptr);
  ASSERT_TRUE(original);
  sink_->ReleaseFront(nullptr);
  ASSERT_EQ(SetSpace(LynxValue("display-p3")), LynxTextureView::kSuccess);
  auto* managed = static_cast<SharedImageSinkManaged*>(sink_);
  ASSERT_TRUE(managed->UpdateSharedImageFactory(
      [](auto, auto) { return fml::RefPtr<SharedImageBacking>(); }));
  EXPECT_EQ(view_->AcquireSurface(16, 16), nullptr);
  EXPECT_EQ(SetSpace(LynxValue("srgb")), LynxTextureView::kSuccess);
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  ASSERT_TRUE(view_->SwapBack());
  EXPECT_EQ(original->GetColorSpace(), kClayColorSpaceSRGB);
  EXPECT_EQ(SetSpace(LynxValue("display-p3")), LynxTextureView::kSuccess);
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  ASSERT_TRUE(view_->SwapBack());
  auto recovered = sink_->UpdateFrontToLatest(nullptr);
  ASSERT_TRUE(recovered);
  EXPECT_EQ(recovered->GetColorSpace(), kClayColorSpaceDisplayP3);
}

TEST(SharedImageColorSpaceTest, RejectsMissingSize) {
  EXPECT_EQ(ClayCreateSharedImage(kClaySharedImageBackingTypeIOSurface,
                                  kClaySharedImageBackingPixelFormatNative8888,
                                  nullptr),
            nullptr);
  EXPECT_EQ(ClayCreateSharedImageFromHandle(
                kClaySharedImageBackingTypeIOSurface,
                kClaySharedImageBackingPixelFormatNative8888, nullptr, nullptr),
            nullptr);
}

#ifdef _WIN32
TEST(SharedImageColorSpaceTest, RejectsInvalidD3DTextureDimensions) {
  for (auto space : {kClayColorSpaceSRGB, kClayColorSpaceDisplayP3}) {
    EXPECT_EQ(ClayCreateSharedImageWithColorSpace(
                  kClaySharedImageBackingTypeD3DTexture,
                  kClaySharedImageBackingPixelFormatNative8888, nullptr, space),
              nullptr);
    for (const ClaySize size :
         {ClaySize{0, 0}, ClaySize{0, 16}, ClaySize{16, 0}}) {
      EXPECT_EQ(ClayCreateSharedImageWithColorSpace(
                    kClaySharedImageBackingTypeD3DTexture,
                    kClaySharedImageBackingPixelFormatNative8888, &size, space),
                nullptr);
      if (size.width || size.height) {
        EXPECT_EQ(ClayCreateSharedImageFromHandleWithColorSpace(
                      kClaySharedImageBackingTypeD3DTexture,
                      kClaySharedImageBackingPixelFormatNative8888, &size,
                      reinterpret_cast<ClaySharedImageNativeHandle>(1), space),
                  nullptr);
      }
    }
  }
}

#endif

TEST_F(TextureViewColorSpaceTest, ImportsNativeTexturesWithDeclaredColorSpace) {
  const ClaySize size{16, 16};
  for (const char* space : {"srgb", "display-p3"}) {
    ASSERT_EQ(SetSpace(LynxValue(space)), LynxTextureView::kSuccess);
    const auto color = view_->GetSurfaceColorSpace() == kLynxColorSpaceDisplayP3
                           ? kClayColorSpaceDisplayP3
                           : kClayColorSpaceSRGB;
    auto type = reinterpret_cast<NativePlatformView*>(view_->native_view())
                    ->backing_type();
    auto image = ClayCreateSharedImageWithColorSpace(
        type, kClaySharedImageBackingPixelFormatNative8888, &size, color);
    ASSERT_NE(image, nullptr);
    auto* source = reinterpret_cast<SharedImageBacking*>(image);
    LynxValue params(LynxValue::kCreateAsMapTag);
#ifdef _WIN32
    params.SetProperty("processid",
                       LynxValue(static_cast<int32_t>(GetCurrentProcessId())));
    params.SetProperty(
        "surface", LynxValue(static_cast<double>(
                       reinterpret_cast<uintptr_t>(source->GetGFXHandle()))));
#elif defined(__APPLE__)
    auto surface = reinterpret_cast<IOSurfaceRef>(source->GetGFXHandle());
    params.SetProperty("surface", LynxValue(IOSurfaceGetID(surface)));
#endif
    bool invoked = false;
    view_->OnMethodInvoked("update", params, [&](int code, LynxValue&& data) {
      invoked = true;
      EXPECT_EQ(code, LynxTextureView::kSuccess);
      EXPECT_TRUE(data.Bool());
    });
    EXPECT_TRUE(invoked);
    ClayReleaseSharedImage(image);
    auto imported = sink_->UpdateFront(nullptr);
    ASSERT_TRUE(imported);
    EXPECT_EQ(imported->GetSize(), skity::Vec2(16, 16));
    EXPECT_EQ(imported->GetColorSpace(), color);
    sink_->ReleaseFront(nullptr);
  }
}

#ifdef _WIN32
TEST(SharedImageColorSpaceTest, RejectsWindowsHandlesWithoutATexture) {
  HANDLE event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  ASSERT_NE(event, nullptr);
  const ClaySize size{16, 16};
  for (auto space : {kClayColorSpaceSRGB, kClayColorSpaceDisplayP3}) {
    auto image = ClayCreateSharedImageFromHandleWithColorSpace(
        kClaySharedImageBackingTypeD3DTexture,
        kClaySharedImageBackingPixelFormatNative8888, &size, event, space);
    EXPECT_EQ(image, nullptr);
    ClayReleaseSharedImage(image);
  }
  CloseHandle(event);
}

TEST_F(TextureViewColorSpaceTest, FailedWindowsP3AllocationCanRetryAndSwitch) {
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  ASSERT_TRUE(view_->SwapBack());
  auto original = sink_->UpdateFront(nullptr);
  ASSERT_TRUE(original);
  sink_->ReleaseFront(nullptr);
  ASSERT_EQ(SetSpace(LynxValue("display-p3")), LynxTextureView::kSuccess);
  // D3D rejects this extent before allocating GPU memory.
  const int invalid_width = 1 << 20;
  EXPECT_EQ(view_->AcquireSurface(invalid_width, 16), nullptr);
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  ASSERT_TRUE(view_->SwapBack());
  auto p3 = sink_->UpdateFront(nullptr);
  ASSERT_TRUE(p3);
  EXPECT_EQ(p3->GetColorSpace(), kClayColorSpaceDisplayP3);
  EXPECT_EQ(original->GetColorSpace(), kClayColorSpaceSRGB);
  sink_->ReleaseFront(nullptr);
  EXPECT_EQ(view_->AcquireSurface(invalid_width, 16), nullptr);
  EXPECT_EQ(SetSpace(LynxValue("srgb")), LynxTextureView::kSuccess);
  ASSERT_NE(view_->AcquireSurface(16, 16), nullptr);
  ASSERT_TRUE(view_->SwapBack());
  auto recovered = sink_->UpdateFront(nullptr);
  ASSERT_TRUE(recovered);
  EXPECT_EQ(recovered->GetColorSpace(), kClayColorSpaceSRGB);
  EXPECT_EQ(p3->GetColorSpace(), kClayColorSpaceDisplayP3);
}
#endif

}  // namespace
}  // namespace clay
