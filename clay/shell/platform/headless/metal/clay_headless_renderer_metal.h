// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_METAL_CLAY_HEADLESS_RENDERER_METAL_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_METAL_CLAY_HEADLESS_RENDERER_METAL_H_

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/fml/platform/darwin/scoped_nsobject.h"
#include "clay/shell/platform/headless/clay_headless_renderer.h"

namespace clay {
class IOSurfaceImageBacking;
}

namespace clay {

class ClayHeadlessRendererMetal : public ClayHeadlessRenderer,
                                  public EmbedderSurfaceMetalDelegate {
 public:
  ClayHeadlessRendererMetal(ClayHeadlessEngine* engine,
                            const ClayHardwareRendererConfig& config);
  ~ClayHeadlessRendererMetal() override;

  EmbedderSurfaceMetalDelegate* GetMetalRendererDelegate() override;

  GPUMTLDeviceHandle GetMTLDevice() const override;
  GPUMTLCommandQueueHandle GetMTLCommandQueue() const override;
  GPUMTLTextureInfo GetMTLTexture(const skity::Vec2& frame_size) const override;
  bool PresentTexture(GPUMTLTextureInfo texture) const override;
  bool EnablePartialRepaint() const override;

  void CleanupGPUResources() override;

 private:
  ClaySharedImageSinkAccessorRef surface_accessor_ = nullptr;
  bool enable_partial_repaint_ = false;
  fml::scoped_nsprotocol<id<MTLDevice>> device_;
  fml::scoped_nsprotocol<id<MTLCommandQueue>> command_queue_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_METAL_CLAY_HEADLESS_RENDERER_METAL_H_
