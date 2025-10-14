// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/skity_gl_image_representation.h"

#include <utility>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/utils/gl_texture_converter.h"
#include "clay/gfx/skity/skity_image.h"
#include "skity/gpu/gpu_context.hpp"
#include "skity/gpu/gpu_context_gl.hpp"

namespace clay {

SkityGLImageRepresentation::SkityGLImageRepresentation(
    skity::GPUContext* skity_context,
    fml::RefPtr<GLImageRepresentation> gl_representation)
    : SkityImageRepresentation(fml::Ref(gl_representation->GetBacking())),
      skity_context_(skity_context),
      gl_representation_(gl_representation) {
  FML_DCHECK(skity_context_->GetBackendType() ==
             skity::GPUBackendType::kOpenGL);
}

SkityGLImageRepresentation::~SkityGLImageRepresentation() = default;

std::shared_ptr<SkityImage> SkityGLImageRepresentation::GetSkityImage() {
  ClaySharedImageReadResult result;
  if (!gl_representation_->BeginRead(&result)) {
    return nullptr;
  }
  FML_DCHECK(result.struct_size == sizeof(result));
  FML_DCHECK(result.type == kClaySharedImageRepresentationTypeGL &&
             result.opengl_texture.struct_size == sizeof(ClayOpenGLTexture));

  GLuint texture_2d = 0;
  uint32_t texture_width = result.opengl_texture.size.width;
  uint32_t texture_height = result.opengl_texture.size.height;
  if (result.opengl_texture.target == GL_TEXTURE_EXTERNAL_OES) {
    texture_2d = Get2DTextureFromExternalTextureOES(
        result.opengl_texture.name, texture_width, texture_height);
  } else {
    texture_2d = result.opengl_texture.name;
  }
  if (texture_2d == 0) {
    return nullptr;
  }

  skity::GPUBackendTextureInfoGL texture_info = {
      {skity::GPUBackendType::kOpenGL, texture_width, texture_height,
       skity::TextureFormat::kRGBA, skity::AlphaType::kPremul_AlphaType},
      texture_2d,
      // We have created a new GL_TEXTURE_2D texture for converting
      // GL_TEXTUER_EXTERNAL_OES texture. In that case we assume that the extra
      // GL_TEXTURE_2D texture is owned by Skity.
      result.opengl_texture.target == GL_TEXTURE_EXTERNAL_OES ? true : false};

  auto texture = skity_context_->WrapTexture(&texture_info);
  // TODO(yudingqian): This is a workaround that we create a `SkityImage` which
  // wraps the `skity::Image`. Because we need the `destruction_callback` to
  // release the texture and Skity doesn't provide this parameter for now.
  return std::make_shared<SkityImage>(
      skity::Image::MakeHWImage(texture),
      result.opengl_texture.destruction_callback,
      result.opengl_texture.user_data);
}

bool SkityGLImageRepresentation::EndRead() {
  return gl_representation_->EndRead();
}

void SkityGLImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  return gl_representation_->ConsumeFence(std::move(fence_sync));
}
std::unique_ptr<FenceSync> SkityGLImageRepresentation::ProduceFence() {
  return gl_representation_->ProduceFence();
}

fml::RefPtr<RepresentationStorageManager>
SkityGLImageRepresentation::GetTextureManager() const {
  return gl_representation_->GetTextureManager();
}
void SkityGLImageRepresentation::SetTextureManager(
    fml::RefPtr<RepresentationStorageManager> storage_manager) {
  gl_representation_->SetTextureManager(storage_manager);
}

}  // namespace clay
