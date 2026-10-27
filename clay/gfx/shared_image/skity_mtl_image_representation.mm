// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/skity_mtl_image_representation.h"

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/cv_pixelbuffer_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/iosurface_image_backing.h"
#include "clay/gfx/shared_image/mtl_image_representation.h"
#include "clay/gfx/skity/skity_image.h"
#include "skity/gpu/gpu_context.hpp"
#include "skity/gpu/gpu_context_mtl.h"

namespace clay {

SkityMTLImageRepresentation::SkityMTLImageRepresentation(skity::GPUContext* skity_context,
                                                         fml::RefPtr<IOSurfaceImageBacking> backing)
    : SkityImageRepresentation(backing), skity_context_(skity_context) {
  id<MTLDevice> mtl_device = skity::MTLContextGetDevice(skity_context);
  id<MTLCommandQueue> mtl_queue = skity::MTLContextGetCommandQueue(skity_context);

  mtl_image_representation_ =
      fml::MakeRefCounted<MTLImageRepresentation>(mtl_device, mtl_queue, backing);
}

SkityMTLImageRepresentation::SkityMTLImageRepresentation(
    skity::GPUContext* skity_context, fml::RefPtr<CVPixelBufferImageBacking> backing)
    : SkityImageRepresentation(backing), skity_context_(skity_context) {
  id<MTLDevice> mtl_device = skity::MTLContextGetDevice(skity_context);
  id<MTLCommandQueue> mtl_queue = skity::MTLContextGetCommandQueue(skity_context);

  mtl_image_representation_ =
      fml::MakeRefCounted<MTLImageRepresentation>(mtl_device, mtl_queue, backing);
}

SkityMTLImageRepresentation::~SkityMTLImageRepresentation() = default;

std::shared_ptr<SkityImage> SkityMTLImageRepresentation::GetSkityImage() {
  skity::Vec2 size = mtl_image_representation_->GetSize();

  ClaySharedImageReadResult result;
  if (!mtl_image_representation_->BeginRead(&result)) {
    return nullptr;
  }
  FML_DCHECK(result.struct_size == sizeof(result));
  FML_DCHECK(result.type == kClaySharedImageRepresentationTypeMetal &&
             result.metal_texture.struct_size == sizeof(ClayMetalTexture));

  skity::GPUBackendTextureInfoMTL texture_info = {
      {skity::GPUBackendType::kMetal, static_cast<uint32_t>(size.x), static_cast<uint32_t>(size.y),
       skity::TextureFormat::kBGRA, skity::AlphaType::kPremul_AlphaType},
      (__bridge id<MTLTexture>)result.metal_texture.texture};

  auto texture = skity_context_->WrapTexture(&texture_info);
  // TODO(yudingqian): This is a workaround that we create a `SkityImage` which wraps the
  // `skity::Image`. Because we need the `destruction_callback` to release the texture and Skity
  // doesn't provide this parameter for now.
  return std::make_shared<SkityImage>(skity::Image::MakeHWImage(texture),
                                      result.metal_texture.destruction_callback,
                                      result.metal_texture.user_data);
}

bool SkityMTLImageRepresentation::EndRead() { return mtl_image_representation_->EndRead(); }

void SkityMTLImageRepresentation::ConsumeFence(std::unique_ptr<FenceSync> fence_sync) {
  return mtl_image_representation_->ConsumeFence(std::move(fence_sync));
}

std::unique_ptr<FenceSync> SkityMTLImageRepresentation::ProduceFence() {
  return mtl_image_representation_->ProduceFence();
}

fml::RefPtr<RepresentationStorageManager> SkityMTLImageRepresentation::GetTextureManager() const {
  return mtl_image_representation_->GetTextureManager();
}
void SkityMTLImageRepresentation::SetTextureManager(
    fml::RefPtr<RepresentationStorageManager> storage_manager) {
  mtl_image_representation_->SetTextureManager(storage_manager);
}

}  // namespace clay
