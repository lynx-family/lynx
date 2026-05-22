// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/skia_gl_image_representation.h"

#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/gpu/GrBackendSurface.h"
#include "third_party/skia/include/gpu/GrDirectContext.h"
#include "third_party/skia/include/gpu/ganesh/SkImageGanesh.h"
#include "third_party/skia/include/gpu/gl/GrGLTypes.h"

namespace clay {

SkiaGLImageRepresentation::SkiaGLImageRepresentation(
    GrDirectContext* gr_context,
    fml::RefPtr<GLImageRepresentation> gl_representation)
    : SkiaImageRepresentation(fml::Ref(gl_representation->GetBacking())),
      gr_context_(gr_context),
      gl_representation_(gl_representation) {
  FML_DCHECK(gr_context_->backend() == GrBackendApi::kOpenGL_GrBackend);
}

SkiaGLImageRepresentation::~SkiaGLImageRepresentation() = default;

sk_sp<SkImage> SkiaGLImageRepresentation::GetSkImage() {
  ClaySharedImageReadResult result;
  if (!gl_representation_->BeginRead(&result)) {
    return nullptr;
  }
  FML_DCHECK(result.struct_size == sizeof(result));
  FML_DCHECK(result.type == kClaySharedImageRepresentationTypeGL &&
             result.opengl_texture.struct_size == sizeof(ClayOpenGLTexture));

  GrGLTextureInfo texture_info;
  texture_info.fTarget = result.opengl_texture.target;
  texture_info.fID = result.opengl_texture.name;
  texture_info.fFormat = result.opengl_texture.format;

  GrBackendTexture backend_texture(result.opengl_texture.size.width,
                                   result.opengl_texture.size.height,
                                   GrMipMapped::kNo, texture_info);

  return SkImages::BorrowTextureFrom(
      gr_context_, backend_texture, kBottomLeft_GrSurfaceOrigin,
      kRGBA_8888_SkColorType, kPremul_SkAlphaType, nullptr,
      result.opengl_texture.destruction_callback,
      result.opengl_texture.user_data);
}

bool SkiaGLImageRepresentation::EndRead() {
  return gl_representation_->EndRead();
}

void SkiaGLImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  return gl_representation_->ConsumeFence(std::move(fence_sync));
}
std::unique_ptr<FenceSync> SkiaGLImageRepresentation::ProduceFence() {
  return gl_representation_->ProduceFence();
}

fml::RefPtr<RepresentationStorageManager>
SkiaGLImageRepresentation::GetTextureManager() const {
  return gl_representation_->GetTextureManager();
}
void SkiaGLImageRepresentation::SetTextureManager(
    fml::RefPtr<RepresentationStorageManager> storage_manager) {
  gl_representation_->SetTextureManager(storage_manager);
}

}  // namespace clay
