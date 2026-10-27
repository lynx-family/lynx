// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/display/texture_compositor.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "base/trace/native/trace_event.h"
#include "clay/shell/common/display/raster_output_surface.h"
#include "clay/shell/common/output_surface.h"
#include "skity/gpu/gpu_context_mtl.h"

static_assert(__has_feature(objc_arc), "ARC must be enabled.");

namespace clay::display {
namespace {
class MetalTextureCompositor final : public TextureCompositor {
 public:
  struct alignas(16) QuadParams {
    float clip_rect[4];
    float uv_rect[4];
  };
  static_assert(sizeof(QuadParams) == 32);
  struct Quad {
    id<MTLTexture> texture;
    QuadParams params;
  };

  explicit MetalTextureCompositor(GrContextPtr context)
      : context_(std::move(context)),
        device(skity::MTLContextGetDevice(context_.get())),
        queue(skity::MTLContextGetCommandQueue(context_.get())) {}

  bool PreparePipeline() {
    if (pipeline_attempted) return pipeline != nil;
    pipeline_attempted = true;
    static constexpr char source[] = R"(
#include <metal_stdlib>
using namespace metal;
struct DisplayVertex {
  float4 position [[position]];
  float2 uv;
};
vertex DisplayVertex display_vertex(uint index [[vertex_id]],
                                    constant float4* rects [[buffer(0)]]) {
  float2 corner = float2(index & 1, index >> 1);
  return {float4(mix(rects[0].xy, rects[0].zw, corner), 0.0f, 1.0f),
          mix(rects[1].xy, rects[1].zw, corner)};
}
fragment float4 display_fragment(DisplayVertex input [[stage_in]],
                                 texture2d<float> image [[texture(0)]]) {
  constexpr sampler nearest(coord::normalized, address::clamp_to_edge, filter::nearest);
  return image.sample(nearest, input.uv, level(0.0f));
}
)";
    id<MTLLibrary> library = [device newLibraryWithSource:[NSString stringWithUTF8String:source]
                                                  options:nil
                                                    error:nullptr];
    if (!library) return false;
    auto descriptor = [[MTLRenderPipelineDescriptor alloc] init];
    descriptor.vertexFunction = [library newFunctionWithName:@"display_vertex"];
    descriptor.fragmentFunction = [library newFunctionWithName:@"display_fragment"];
    if (!descriptor.vertexFunction || !descriptor.fragmentFunction) return false;
    descriptor.rasterSampleCount = 1;
    auto color = descriptor.colorAttachments[0];
    color.pixelFormat = MTLPixelFormatBGRA8Unorm;
    color.blendingEnabled = YES;
    color.rgbBlendOperation = MTLBlendOperationAdd;
    color.alphaBlendOperation = MTLBlendOperationAdd;
    color.sourceRGBBlendFactor = MTLBlendFactorOne;
    color.sourceAlphaBlendFactor = MTLBlendFactorOne;
    color.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    color.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
    pipeline = [device newRenderPipelineStateWithDescriptor:descriptor error:nullptr];
    return pipeline != nil;
  }

  bool PrepareInputs(const std::vector<TextureEntry>& inputs, uint32_t width, uint32_t height,
                     std::vector<Quad>& quads) const {
    quads.reserve(inputs.size());
    for (const auto& input : inputs) {
      if (!input.texture_ref) return false;
      auto buffer =
          std::static_pointer_cast<RasterBuffer>(input.texture_ref->resource()->storage());
      auto representation = buffer ? buffer->representation : nullptr;
      const auto& image = input.texture_ref->image();
      const auto& rect = image.valid_rect;
      if (image.format != display::PixelFormat::kBGRA8888 ||
          image.alpha != display::AlphaMode::kPremultiplied ||
          image.color != display::ColorSpace::kSRGB ||
          image.origin != display::ImageOrigin::kTopLeft || !image.width || !image.height ||
          rect.x < 0 || rect.y < 0 || !rect.width || !rect.height ||
          uint64_t(rect.x) + rect.width > image.width ||
          uint64_t(rect.y) + rect.height > image.height || !representation ||
          representation->GetType() != ImageRepresentationType::kMetal) {
        return false;
      }
      ClaySharedImageReadResult read{};
      if (!representation->BeginRead(&read)) return false;
      const bool metal = read.type == kClaySharedImageRepresentationTypeMetal;
      const bool native = metal && read.struct_size >= sizeof(read) &&
                          read.metal_texture.struct_size >= sizeof(ClayMetalTexture);
      id<MTLTexture> texture = native ? (__bridge id<MTLTexture>)read.metal_texture.texture : nil;
      const bool valid = native && texture && texture.device == device &&
                         !texture.framebufferOnly &&
                         texture.pixelFormat == MTLPixelFormatBGRA8Unorm &&
                         texture.textureType == MTLTextureType2D && texture.sampleCount == 1 &&
                         (texture.usage & MTLTextureUsageShaderRead) &&
                         texture.width == image.width && texture.height == image.height;
      if (metal && read.metal_texture.destruction_callback)
        read.metal_texture.destruction_callback(read.metal_texture.user_data);
      representation->EndRead();
      if (!valid) return false;
      // Match Canvas placement and clipping without scaling or resetting UVs.
      const int64_t x = int64_t(input.properties.x) + rect.x;
      const int64_t y = int64_t(input.properties.y) + rect.y;
      const int64_t left = std::max<int64_t>(0, x);
      const int64_t top = std::max<int64_t>(0, y);
      const int64_t right = std::min<int64_t>(width, x + rect.width);
      const int64_t bottom = std::min<int64_t>(height, y + rect.height);
      if (left >= right || top >= bottom) continue;
      const float x0 = 2.0f * left / width - 1.0f;
      const float x1 = 2.0f * right / width - 1.0f;
      const float y0 = 1.0f - 2.0f * top / height;
      const float y1 = 1.0f - 2.0f * bottom / height;
      const float u0 = float(rect.x + left - x) / image.width;
      const float u1 = float(rect.x + right - x) / image.width;
      const float v0 = float(rect.y + top - y) / image.height;
      const float v1 = float(rect.y + bottom - y) / image.height;
      quads.push_back({texture, {{x0, y0, x1, y1}, {u0, v0, u1, v1}}});
    }
    return true;
  }

  TextureComposition Prepare(const RasterBuffer& buffer, const RasterResult& result,
                             OutputSurface& output) override {
    if (!buffer.representation ||
        buffer.representation->GetType() != ImageRepresentationType::kMetal ||
        buffer.origin != ImageOrigin::kTopLeft || !result.config.IsValid() ||
        result.config.format != PixelFormat::kBGRA8888 ||
        result.config.alpha != AlphaMode::kPremultiplied ||
        result.config.color != ColorSpace::kSRGB)
      return {};
    ClaySharedImageWriteResult write{};
    if (!buffer.representation->BeginWrite(&write)) return {};
    const bool native = write.type == kClaySharedImageRepresentationTypeMetal &&
                        write.struct_size >= sizeof(write) &&
                        write.metal_texture.struct_size >= sizeof(ClayMetalTexture);
    id<MTLTexture> target = native ? (__bridge id<MTLTexture>)write.metal_texture.texture : nil;
    if (write.type == kClaySharedImageRepresentationTypeMetal &&
        write.metal_texture.destruction_callback)
      write.metal_texture.destruction_callback(write.metal_texture.user_data);
    const skity::Vec2 size{float(result.config.width), float(result.config.height)};
    if (!target || target.device != device || target.framebufferOnly ||
        target.pixelFormat != MTLPixelFormatBGRA8Unorm || target.textureType != MTLTextureType2D ||
        target.sampleCount != 1 || !(target.usage & MTLTextureUsageRenderTarget) ||
        !std::isfinite(size.x) || !std::isfinite(size.y) || size.x <= 0 || size.y <= 0 ||
        size.x > 16384 || size.y > 16384 || target.width != size.x || target.height != size.y) {
      return {};
    }
    std::vector<Quad> quads;
    if (!PrepareInputs(result.outputs, target.width, target.height, quads) || !PreparePipeline())
      return {};
    id<MTLCommandBuffer> command = [queue commandBuffer];
    if (!command) return {};
    auto completion = output.CreateCommandCompletion((__bridge const void*)command);
    if (!completion) return {};
    auto descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
    auto color = descriptor.colorAttachments[0];
    color.texture = target;
    color.loadAction = MTLLoadActionClear;
    color.storeAction = MTLStoreActionStore;
    color.clearColor = MTLClearColorMake(0, 0, 0, 0);
    id<MTLRenderCommandEncoder> encoder = [command renderCommandEncoderWithDescriptor:descriptor];
    if (!encoder) return {};
    [encoder setRenderPipelineState:pipeline];
    [encoder setViewport:MTLViewport{0, 0, double(target.width), double(target.height), 0, 1}];
    [encoder setScissorRect:MTLScissorRect{0, 0, target.width, target.height}];
    [encoder setCullMode:MTLCullModeNone];
    for (const auto& quad : quads) {
      [encoder setVertexBytes:&quad.params length:sizeof(quad.params) atIndex:0];
      [encoder setFragmentTexture:quad.texture atIndex:0];
      [encoder drawPrimitives:MTLPrimitiveTypeTriangleStrip vertexStart:0 vertexCount:4];
    }
    [encoder endEncoding];
    // A declined batch never commits. An accepted batch uses this draw's own
    // completion instead of flushing Canvas or submitting an extra marker.
    return {[command, completion = std::move(completion)] {
      TRACE_EVENT("clay", "TextureCompositor::NativeCompose");
      [command commit];
      return TextureComposition::Result{true, completion};
    }};
  }

 private:
  GrContextPtr context_;
  id<MTLDevice> device;
  id<MTLCommandQueue> queue;
  id<MTLRenderPipelineState> pipeline = nil;
  bool pipeline_attempted = false;
};
}  // namespace

std::unique_ptr<TextureCompositor> CreateTextureCompositor(GrContextPtr context) {
  if (!context || context->GetBackendType() != skity::GPUBackendType::kMetal) return nullptr;
  return std::make_unique<MetalTextureCompositor>(std::move(context));
}

}  // namespace clay::display
