// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/platform_overlay_service_mac.h"
#include "clay/common/service/service_manager.h"
#import "clay/shell/platform/darwin/macos/framework/Source/ClayOverlayView.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterEngine_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterSurfaceManager.h"

#import <Cocoa/Cocoa.h>
#import <IOSurface/IOSurface.h>

#include "clay/fml/logging.h"
#include "clay/shell/platform/darwin/macos/framework/Source/overlay_view_controller_service.h"
#include "clay/shell/platform/embedder/embedder_surface_metal.h"

namespace clay {
namespace {

void RunOnMainThreadSync(dispatch_block_t block) {
  if ([NSThread isMainThread]) {
    block();
    return;
  }
  dispatch_sync(dispatch_get_main_queue(), block);
}

}  // namespace

PlatformOverlayMac::PlatformOverlayMac(id<MTLDevice> device, id<MTLCommandQueue> command_queue,
                                       ClayOverlayView* overlay_view, CALayer* layer)
    : device_(device),
      command_queue_(command_queue),
      overlay_view_(overlay_view),
      layer_(layer),
      back_buffer_cache_([[FlutterBackBufferCache alloc] init]),
      front_surface_(nil) {
  // Upcast to base inside the class where private inheritance is accessible.
  EmbedderSurfaceMetalDelegate* delegate = this;
  output_surface_ = fml::MakeRefCounted<EmbedderSurfaceMetal>(delegate);
}

PlatformOverlayMac::~PlatformOverlayMac() {
  // CALayer removal must be on the main thread.
  CALayer* layer = layer_;
  if ([NSThread isMainThread]) {
    [layer removeFromSuperlayer];
    return;
  }
  dispatch_async(dispatch_get_main_queue(), ^{
    [layer removeFromSuperlayer];
  });
}

CGRect PlatformOverlayMac::DisplayOverlaySurface(int x, int y, int width, int height) {
  __block CGRect frame = CGRectZero;
  RunOnMainThreadSync(^{
    CGFloat scale = layer_.superlayer ? layer_.superlayer.contentsScale : GetContentsScale();
    if (scale <= 0.0) {
      scale = 1.0;
    }
    layer_.contentsScale = scale;
    if (overlay_view_) {
      frame = [overlay_view_ viewRectFromDevicePixelRect:CGRectMake(x, y, width, height)
                                           contentsScale:scale];
    } else {
      frame = CGRectMake(x / scale, y / scale, width / scale, height / scale);
    }
    layer_.frame = frame;
    layer_.hidden = NO;
  });
  return frame;
}

void PlatformOverlayMac::BringToFront() const {
  RunOnMainThreadSync(^{
    if (layer_.superlayer) {
      [layer_.superlayer addSublayer:layer_];
    }
  });
}

void PlatformOverlayMac::RemoveFromParent() const {
  RunOnMainThreadSync(^{
    layer_.hidden = YES;
  });
}

CGFloat PlatformOverlayMac::GetContentsScale() const { return layer_.contentsScale; }

fml::RefPtr<OutputSurface> PlatformOverlayMac::GetOutputSurface() const { return output_surface_; }

GPUMTLDeviceHandle PlatformOverlayMac::GetMTLDevice() const {
  return (__bridge GPUMTLDeviceHandle)device_;
}

GPUMTLCommandQueueHandle PlatformOverlayMac::GetMTLCommandQueue() const {
  return (__bridge GPUMTLCommandQueueHandle)command_queue_;
}

GPUMTLTextureInfo PlatformOverlayMac::GetMTLTexture(const skity::Vec2& frame_size) const {
  CGSize size = CGSizeMake(frame_size.x, frame_size.y);
  FlutterSurface* surface = [back_buffer_cache_ removeSurfaceForSize:size];
  if (!surface) {
    surface = [[FlutterSurface alloc] initWithSize:size device:device_];
  }
  return [surface asGPUMTLTextureInfo];
}

bool PlatformOverlayMac::PresentTexture(GPUMTLTextureInfo texture) const {
  FML_DCHECK([NSThread isMainThread]);
  FlutterSurface* surface = [FlutterSurface fromGPUMTLTextureInfo:&texture];
  if (!surface) {
    return false;
  }
  if (front_surface_ && front_surface_ != surface) {
    [back_buffer_cache_ replaceSurfaces:@[ front_surface_ ]];
  }
  front_surface_ = surface;
  layer_.contents = (__bridge id)surface.ioSurface;
  return true;
}

bool PlatformOverlayMac::EnablePartialRepaint() const { return false; }

std::vector<std::shared_ptr<PlatformOverlay>> PlatformOverlayServiceMac::CreatePlatformOverlay(
    size_t num) {
  // CALayer creation and parenting must happen on the main thread.
  __block std::vector<CALayer*> layers(num);
  __block ClayOverlayView* overlay_view = nil;
  __block CALayer* parent = nil;
  __block CGFloat scale = 1.0;
  RunOnMainThreadSync(^{
    overlay_view = overlay_view_controller_service_->GetOverlayView();
    parent = overlay_view.layer;
    scale = overlay_view.window.backingScaleFactor;
    for (size_t i = 0; i < num; i++) {
      CALayer* layer = [CALayer layer];
      layer.opaque = NO;
      layer.hidden = YES;
      layer.contentsGravity = kCAGravityTopLeft;
      layer.contentsScale = scale;
      layer.frame = parent.bounds;
      layer.autoresizingMask = kCALayerWidthSizable | kCALayerHeightSizable;
      [parent addSublayer:layer];
      layers[i] = layer;
    }
  });

  std::vector<std::shared_ptr<PlatformOverlay>> result;
  result.reserve(num);
  for (size_t i = 0; i < num; i++) {
    result.push_back(std::make_shared<PlatformOverlayMac>(
        overlay_view_controller_service_->GetEngine().renderer.device,
        overlay_view_controller_service_->GetEngine().renderer.commandQueue, overlay_view,
        layers[i]));
  }
  return result;
}

std::shared_ptr<PlatformOverlayService> PlatformOverlayService::Create() {
  return std::make_shared<PlatformOverlayServiceMac>();
}

void PlatformOverlayServiceMac::OnInit(clay::ServiceManager& service_manager,
                                       const clay::PlatformServiceContext& ctx) {
  overlay_view_controller_service_ = service_manager.GetService<OverlayViewControllerService>();
}

}  // namespace clay
