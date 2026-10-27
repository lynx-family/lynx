// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/gl/scoped_texture_binder.h"

#include "clay/fml/logging.h"

namespace clay {

typedef unsigned int GLenum;
#define GL_TEXTURE_2D 0x0DE1
#define GL_TEXTURE_BINDING_2D 0x8069
#define GL_TEXTURE_RECTANGLE 0x84F5
#define GL_TEXTURE_BINDING_RECTANGLE 0x84F6
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#define GL_TEXTURE_BINDING_EXTERNAL_OES 0x8D67
#define GL_TEXTURE_CUBE_MAP 0x8513
#define GL_TEXTURE_BINDING_CUBE_MAP 0x8514

ScopedTextureBinder::ScopedTextureBinder(unsigned int target, unsigned int id,
                                         GlApi* gl_api)
    : target_(target), old_id_(-1), gl_api_(gl_api) {
  if (!gl_api_) {
    FML_LOG(ERROR) << "Not get valid GLProc for ScopedTextureBinder";
    return;
  }
  GLenum target_getter = 0;
  switch (target) {
    case GL_TEXTURE_2D:
      target_getter = GL_TEXTURE_BINDING_2D;
      break;
    case GL_TEXTURE_RECTANGLE:
      target_getter = GL_TEXTURE_BINDING_RECTANGLE;
      break;
    case GL_TEXTURE_CUBE_MAP:
      target_getter = GL_TEXTURE_BINDING_CUBE_MAP;
      break;
    case GL_TEXTURE_EXTERNAL_OES:
      target_getter = GL_TEXTURE_BINDING_EXTERNAL_OES;
      break;
    default:
      FML_LOG(ERROR) << "Target not supported. " << target;
  }
  gl_api_->GlGetIntegerv(target_getter, &old_id_);
  gl_api_->GlBindTexture(target_, id);
}

ScopedTextureBinder::~ScopedTextureBinder() {
  gl_api_->GlBindTexture(target_, old_id_);
}

}  // namespace clay
