// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/metal/clay_headless_renderer_metal.h"

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/iosurface_image_backing.h"
#include "clay/gfx/shared_image/mtl_image_representation.h"
#include "clay/shell/platform/headless/clay_headless_engine.h"

static_assert(__has_feature(objc_arc), "ARC must be enabled.");

namespace clay {
ClayHeadlessRendererMetal::ClayHeadlessRendererMetal(ClayHeadlessEngine* engine,
                                                     const ClayHardwareRendererConfig& config)
    : ClayHeadlessRenderer(engine), enable_partial_repaint_(!config.disable_partial_repaint) {
  device_.reset(MTLCreateSystemDefaultDevice());
  FML_CHECK(device_ != nil);
  command_queue_.reset([device_ newCommandQueue]);
  FML_CHECK(command_queue_ != nil);

  ClaySharedImageRepresentationConfig image_config{};
  image_config.struct_size = sizeof(ClaySharedImageRepresentationConfig);
  image_config.type = kClaySharedImageRepresentationTypeMetal;
  image_config.metal_config.struct_size = sizeof(ClaySharedImageMetalRepresentationConfig);
  image_config.metal_config.device = (__bridge ClayMetalDeviceHandle)device_.get();
  image_config.metal_config.command_queue =
      (__bridge ClayMetalCommandQueueHandle)command_queue_.get();
  surface_accessor_ = ClayCreateSharedImageSinkAccessor(config.sink_ref, &image_config);
}

ClayHeadlessRendererMetal::~ClayHeadlessRendererMetal() = default;

EmbedderSurfaceMetalDelegate* ClayHeadlessRendererMetal::GetMetalRendererDelegate() { return this; }

GPUMTLDeviceHandle ClayHeadlessRendererMetal::GetMTLDevice() const {
  return (__bridge GPUMTLDeviceHandle)device_.get();
}
GPUMTLCommandQueueHandle ClayHeadlessRendererMetal::GetMTLCommandQueue() const {
  return (__bridge GPUMTLCommandQueueHandle)command_queue_.get();
}
GPUMTLTextureInfo ClayHeadlessRendererMetal::GetMTLTexture(const skity::Vec2& frame_size) const {
  ClaySize size{.width = static_cast<uint32_t>(frame_size.x),
                .height = static_cast<uint32_t>(frame_size.y)};
  ClaySharedImageWriteResult result;
  ClaySharedImageRef shared_image;
  uint32_t buffer_age = 0;  // partial repaint is handled in gpu_surface_metal_skia.mm

  GPUMTLTextureInfo res;
  memset(&res, 0, sizeof(clay::GPUMTLTextureInfo));
  if (!surface_accessor_ || !ClaySharedImageSinkBeginWrite(surface_accessor_, &size, &shared_image,
                                                           &result, &buffer_age)) {
    FML_LOG(ERROR) << "failed to get fbo";
    return res;
  }
  // Metal coordinate is Y-flipped
  ClayTransformation transformation{1, 0, 0, 0, -1, 1, 0, 0, 1};
  ClaySharedImageSetTransformation(shared_image, &transformation);
  FML_DCHECK(result.type == kClaySharedImageRepresentationTypeMetal);

  res.texture = result.metal_texture.texture;
  res.texture_id = reinterpret_cast<int64_t>(result.metal_texture.texture);
  res.destruction_callback = result.metal_texture.destruction_callback;
  res.destruction_context = result.metal_texture.user_data;
  return res;
}
bool ClayHeadlessRendererMetal::PresentTexture(GPUMTLTextureInfo texture) const {
  if (!surface_accessor_) {
    return false;
  }
  return ClaySharedImageSinkEndWrite(surface_accessor_);
}
bool ClayHeadlessRendererMetal::EnablePartialRepaint() const { return enable_partial_repaint_; }

void ClayHeadlessRendererMetal::CleanupGPUResources() {
  if (surface_accessor_) {
    ClayDestroySharedImageSinkAccessor(surface_accessor_);
    surface_accessor_ = nullptr;
  }
}

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateMetal(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config) {
  return std::make_unique<ClayHeadlessRendererMetal>(engine, config);
}
}  // namespace clay
