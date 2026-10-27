// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/skia_mtl_image_representation.h"

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/cv_pixelbuffer_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/iosurface_image_backing.h"
#include "clay/gfx/shared_image/mtl_image_representation.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/SkImageGanesh.h"
#include "third_party/skia/src/gpu/ganesh/GrDirectContextPriv.h"  // nogncheck
#include "third_party/skia/src/gpu/ganesh/mtl/GrMtlGpu.h"         // nogncheck

namespace clay {

SkiaMTLImageRepresentation::SkiaMTLImageRepresentation(GrDirectContext* gr_context,
                                                       fml::RefPtr<SharedImageBacking> backing)
    : SkiaImageRepresentation(backing), gr_context_(gr_context) {
  FML_DCHECK(gr_context_->backend() == GrBackendApi::kMetal);

  GrMtlGpu* gpu = static_cast<GrMtlGpu*>(gr_context_->priv().getGpu());

  mtl_image_representation_ =
      fml::MakeRefCounted<MTLImageRepresentation>(gpu->device(), gpu->queue(), backing);
}

SkiaMTLImageRepresentation::~SkiaMTLImageRepresentation() = default;

sk_sp<SkImage> SkiaMTLImageRepresentation::GetSkImage() {
  skity::Vec2 size = mtl_image_representation_->GetSize();

  ClaySharedImageReadResult result;
  if (!mtl_image_representation_->BeginRead(&result)) {
    return nullptr;
  }
  FML_DCHECK(result.struct_size == sizeof(result));
  FML_DCHECK(result.type == kClaySharedImageRepresentationTypeMetal &&
             result.metal_texture.struct_size == sizeof(ClayMetalTexture));
  GrMtlTextureInfo skia_texture_info;
  skia_texture_info.fTexture =
      sk_cfp{CFBridgingRetain((__bridge id<MTLTexture>)result.metal_texture.texture)};

  GrBackendTexture skia_backend_texture(size.x, size.y, GrMipMapped ::kNo, skia_texture_info);

  // We always use OpenGL texture coordinate as the standard
  return SkImages::BorrowTextureFrom(gr_context_, skia_backend_texture, kBottomLeft_GrSurfaceOrigin,
                                     kBGRA_8888_SkColorType, kPremul_SkAlphaType, nullptr,
                                     result.metal_texture.destruction_callback,
                                     result.metal_texture.user_data);
}

bool SkiaMTLImageRepresentation::EndRead() { return mtl_image_representation_->EndRead(); }

void SkiaMTLImageRepresentation::ConsumeFence(std::unique_ptr<FenceSync> fence_sync) {
  return mtl_image_representation_->ConsumeFence(std::move(fence_sync));
}

std::unique_ptr<FenceSync> SkiaMTLImageRepresentation::ProduceFence() {
  return mtl_image_representation_->ProduceFence();
}

fml::RefPtr<RepresentationStorageManager> SkiaMTLImageRepresentation::GetTextureManager() const {
  return mtl_image_representation_->GetTextureManager();
}
void SkiaMTLImageRepresentation::SetTextureManager(
    fml::RefPtr<RepresentationStorageManager> storage_manager) {
  mtl_image_representation_->SetTextureManager(storage_manager);
}

}  // namespace clay
