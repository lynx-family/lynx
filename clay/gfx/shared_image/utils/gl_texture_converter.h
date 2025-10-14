// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_UTILS_GL_TEXTURE_CONVERTER_H_
#define CLAY_GFX_SHARED_IMAGE_UTILS_GL_TEXTURE_CONVERTER_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <stdint.h>

namespace clay {

GLuint Get2DTextureFromExternalTextureOES(GLuint oes_texture_id,
                                          uint32_t texture_width,
                                          uint32_t texture_height);

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_UTILS_GL_TEXTURE_CONVERTER_H_
