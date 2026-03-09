// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_DARWIN_MACOS_PLATFORM_OVERLAY_SERVICE_MAC_H_
#define CLAY_SHELL_PLATFORM_DARWIN_MACOS_PLATFORM_OVERLAY_SERVICE_MAC_H_

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <memory>
#include <vector>

#include "clay/shell/common/services/compositor/platform_overlay_service.h"
#include "clay/shell/platform/darwin/macos/framework/Source/FlutterSurface.h"
#include "clay/shell/platform/darwin/macos/framework/Source/overlay_view_controller_service.h"
#include "clay/shell/platform/embedder/embedder_surface_metal.h"

@class ClayOverlayView;
@class FlutterBackBufferCache;

namespace clay {

// One Metal IOSurface-backed overlay layer rendered above native views.
// Each instance owns a CALayer sublayer on the Lynx overlay view's layer.
class PlatformOverlayMac final : public PlatformOverlay, private EmbedderSurfaceMetalDelegate {
 public:
  PlatformOverlayMac(id<MTLDevice> device, id<MTLCommandQueue> command_queue,
                     ClayOverlayView* overlay_view, CALayer* layer);
  ~PlatformOverlayMac() override;

  // Update the sublayer frame (called from platform/main thread).
  CGRect DisplayOverlaySurface(int x, int y, int width, int height);

  void BringToFront() const;
  void RemoveFromParent() const;

  // Returns the contentsScale set on this overlay's CALayer.
  CGFloat GetContentsScale() const;

  // PlatformOverlay
  fml::RefPtr<OutputSurface> GetOutputSurface() const override;
  void OnSurfaceUpdated() override {}

 private:
  // EmbedderSurfaceMetalDelegate
  GPUMTLDeviceHandle GetMTLDevice() const override;
  GPUMTLCommandQueueHandle GetMTLCommandQueue() const override;
  GPUMTLTextureInfo GetMTLTexture(const skity::Vec2& frame_size) const override;
  bool PresentTexture(GPUMTLTextureInfo texture) const override;
  bool EnablePartialRepaint() const override;

  __strong id<MTLDevice> device_;
  __strong id<MTLCommandQueue> command_queue_;
  __weak ClayOverlayView* overlay_view_;
  __strong CALayer* layer_;
  __strong FlutterBackBufferCache* back_buffer_cache_;
  mutable __strong FlutterSurface* front_surface_;
  fml::RefPtr<EmbedderSurfaceMetal> output_surface_;
};

// Creates PlatformOverlayMac instances, adding CALayer sublayers to the
// provided overlay_layer (LynxOverlayView.layer).
class PlatformOverlayServiceMac final : public PlatformOverlayService {
 public:
  PlatformOverlayServiceMac() = default;
  std::vector<std::shared_ptr<PlatformOverlay>> CreatePlatformOverlay(size_t num) override;
  void OnInit(clay::ServiceManager& service_manager,
              const clay::PlatformServiceContext& ctx) override;

 private:
  clay::Puppet<clay::Owner::kPlatform, OverlayViewControllerService>
      overlay_view_controller_service_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_DARWIN_MACOS_PLATFORM_OVERLAY_SERVICE_MAC_H_
