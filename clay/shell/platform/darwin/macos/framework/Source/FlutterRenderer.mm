// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterRenderer.h"

#include "clay/fml/logging.h"

#ifndef ENABLE_SKITY
#include "clay/shell/platform/darwin/graphics/FlutterDarwinContextMetalSkia.h"
#else
#include "clay/shell/platform/darwin/graphics/FlutterDarwinContextMetalSkity.h"
#endif

#import "clay/shell/platform/darwin/macos/framework/Source/FlutterEngine_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterViewController_Internal.h"
#import "clay/shell/platform/darwin/macos/framework/Source/FlutterViewEngineProvider.h"

namespace clay {
class EmbedderSurfaceMetalDelegateImpl : public EmbedderSurfaceMetalDelegate {
 public:
  explicit EmbedderSurfaceMetalDelegateImpl(void* flutter_renderer);
  GPUMTLDeviceHandle GetMTLDevice() const override;
  GPUMTLCommandQueueHandle GetMTLCommandQueue() const override;
  GPUMTLTextureInfo GetMTLTexture(const skity::Vec2& frame_size) const override;
  bool PresentTexture(GPUMTLTextureInfo texture) const override;
  bool EnablePartialRepaint() const override;

 private:
  void* flutter_renderer_;
};
}  // namespace clay

#pragma mark - FlutterRenderer implementation

@implementation FlutterRenderer {
  FlutterViewEngineProvider* _viewProvider;
#ifndef ENABLE_SKITY
  FlutterDarwinContextMetalSkia* _darwinMetalContext;
#else
  FlutterDarwinContextMetalSkity* _darwinMetalContext;
#endif
  clay::EmbedderSurfaceMetalDelegateImpl* _embedderSurfaceMetalDelegate;
}

- (instancetype)initWithFlutterEngine:(nonnull FlutterEngine*)flutterEngine {
  self = [super init];
  if (self) {
    _viewProvider = [[FlutterViewEngineProvider alloc] initWithEngine:flutterEngine];
    _device = MTLCreateSystemDefaultDevice();
    if (!_device) {
      FML_LOG(ERROR) << "Could not acquire Metal device.";
      return nil;
    }

    _commandQueue = [_device newCommandQueue];
    if (!_commandQueue) {
      FML_LOG(ERROR) << "Could not create Metal command queue.";
      return nil;
    }

#ifndef ENABLE_SKITY
    _darwinMetalContext = [[FlutterDarwinContextMetalSkia alloc] initWithMTLDevice:_device
                                                                      commandQueue:_commandQueue];
#else
    _darwinMetalContext = [[FlutterDarwinContextMetalSkity alloc] initWithMTLDevice:_device
                                                                       commandQueue:_commandQueue];
#endif

    _embedderSurfaceMetalDelegate =
        new clay::EmbedderSurfaceMetalDelegateImpl((__bridge void*)self);
  }
  return self;
}

- (void)dealloc {
  if (_embedderSurfaceMetalDelegate) {
    delete _embedderSurfaceMetalDelegate;
  }
}

- (FlutterViewEngineProvider*)viewProvider {
  return _viewProvider;
}

- (id<MTLDevice>)mtlDevice {
  return _device;
}

- (id<MTLCommandQueue>)mtlCommandQueue {
  return _commandQueue;
}

- (fml::RefPtr<clay::EmbedderSurfaceMetal>)createEmbedderSurfaceMetal {
  return fml::MakeRefCounted<clay::EmbedderSurfaceMetal>(_embedderSurfaceMetalDelegate);
}

@end

namespace clay {

EmbedderSurfaceMetalDelegateImpl::EmbedderSurfaceMetalDelegateImpl(void* flutter_renderer)
    : flutter_renderer_(flutter_renderer) {}

GPUMTLDeviceHandle EmbedderSurfaceMetalDelegateImpl::GetMTLDevice() const {
  FlutterRenderer* flutter_renderer = (__bridge FlutterRenderer*)flutter_renderer_;
  return (__bridge GPUMTLDeviceHandle)[flutter_renderer mtlDevice];
}

GPUMTLCommandQueueHandle EmbedderSurfaceMetalDelegateImpl::GetMTLCommandQueue() const {
  FlutterRenderer* flutter_renderer = (__bridge FlutterRenderer*)flutter_renderer_;
  return (__bridge GPUMTLCommandQueueHandle)[flutter_renderer mtlCommandQueue];
}

GPUMTLTextureInfo EmbedderSurfaceMetalDelegateImpl::GetMTLTexture(
    const skity::Vec2& frame_size) const {
  FlutterRenderer* flutter_renderer = (__bridge FlutterRenderer*)flutter_renderer_;
  CGSize size = CGSizeMake(frame_size.x, frame_size.y);
  FlutterView* view = [[flutter_renderer viewProvider] getView:kFlutterDefaultViewId];
  if (view == nil) {
    return {};
  }
  return [view.surfaceManager surfaceForSize:size].asGPUMTLTextureInfo;
}

bool EmbedderSurfaceMetalDelegateImpl::PresentTexture(GPUMTLTextureInfo texture) const {
  FlutterRenderer* flutter_renderer = (__bridge FlutterRenderer*)flutter_renderer_;
  FlutterView* view = [[flutter_renderer viewProvider] getView:kFlutterDefaultViewId];
  if (view == nil) {
    return NO;
  }
  FlutterSurface* surface = [FlutterSurface fromGPUMTLTextureInfo:&texture];
  if (surface == nil) {
    return NO;
  }
  FlutterSurfacePresentInfo* info = [[FlutterSurfacePresentInfo alloc] init];
  info.surface = surface;
  [view.surfaceManager present:@[ info ] notify:nil];
  return YES;
}

bool EmbedderSurfaceMetalDelegateImpl::EnablePartialRepaint() const { return YES; }

}  // namespace clay
