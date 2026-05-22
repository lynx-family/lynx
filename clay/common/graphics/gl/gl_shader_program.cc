// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/gl/gl_shader_program.h"

namespace clay {

GLuint CompileShader(GLenum type, const char* shader_src) {
  if (type <= 0 || !shader_src) {
    return 0;
  }
  GLuint shader = glCreateShader(type);
  if (shader == 0) {
    FML_LOG(ERROR) << "Error occurred when creating shader";
    return 0;
  }
  glShaderSource(shader, 1, &shader_src, nullptr);
  glCompileShader(shader);

  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    FML_LOG(ERROR) << "Error occurred when compiling shader, type: " << type;
    glDeleteShader(shader);
    return 0;
  }
  return shader;
}

GLuint CreateProgram(const char* vertex_source, const char* fragment_source) {
  GLuint vertex_shader = CompileShader(GL_VERTEX_SHADER, vertex_source);
  GLuint fragment_shader = CompileShader(GL_FRAGMENT_SHADER, fragment_source);
  if (vertex_shader == 0 || fragment_shader == 0) {
    FML_LOG(ERROR) << "Cannot create program due to shader error";
    return 0;
  }

  GLuint program = glCreateProgram();
  if (program == 0) {
    FML_LOG(ERROR) << "Error occurred when creating program";
    return 0;
  }
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);
  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  GLint linked;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (!linked) {
    FML_LOG(ERROR) << "Linking program error";
    glDeleteProgram(program);
    return 0;
  }
  return program;
}

}  // namespace clay
