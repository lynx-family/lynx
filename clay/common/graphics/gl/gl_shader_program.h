// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_GL_GL_SHADER_PROGRAM_H_
#define CLAY_COMMON_GRAPHICS_GL_GL_SHADER_PROGRAM_H_

#include <GLES2/gl2.h>

#include "clay/fml/logging.h"

namespace clay {

GLuint CompileShader(GLenum type, const char* shader_src);
GLuint CreateProgram(const char* vertex_source, const char* fragment_source);

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_GL_GL_SHADER_PROGRAM_H_
