// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/gl/gl_pipeline_helper.h"

#include "base/include/closure.h"
#include "clay/common/graphics/gl/gl_shader_program.h"

namespace clay {

GlPipelineHelper* GlPipelineHelper::GetInstance() {
  static thread_local GlPipelineHelper* instance = new GlPipelineHelper();
  return instance;
}

bool GlPipelineHelper::Initialize() {
  if (initialized_) {
    return true;
  }

  static const char* vertex_shader_source = R"(
      # version 100
      precision mediump float;
      attribute vec4 position;
      attribute vec2 texCoord;
      varying vec2 vTexCoord;
      void main() {
          gl_Position = position;
          vTexCoord = texCoord;
      }
    )";

  static const char* fragment_shader_source = R"(
      # version 100
      #extension GL_OES_EGL_image_external : require
      precision mediump float;
      varying vec2 vTexCoord;
      uniform samplerExternalOES sTexture;
      void main() {
          gl_FragColor = texture2D(sTexture, vTexCoord);
      }
    )";

  program_ = CreateProgram(vertex_shader_source, fragment_shader_source);
  if (program_ == 0) {
    FML_LOG(ERROR) << "Create program error";
    return false;
  }

  GLint current_program;
  glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
  fml::ScopedCleanupClosure restore_program(
      [current_program]() { glUseProgram(current_program); });
  glUseProgram(program_);
  glUniform1i(glGetUniformLocation(program_, "sTexture"), 0);

  GLint current_vao;
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &current_vao);
  fml::ScopedCleanupClosure restore_vao(
      [current_vao]() { glBindVertexArray(current_vao); });
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);

  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);

  GLfloat vertices[] = {
      -1.0f, -1.0f, 0.0f, 0.0f,  //
      1.0f,  -1.0f, 1.0f, 0.0f,  //
      -1.0f, 1.0f,  0.0f, 1.0f,  //
      1.0f,  1.0f,  1.0f, 1.0f,
  };

  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  auto location_pos = glGetAttribLocation(program_, "position");
  auto location_tex = glGetAttribLocation(program_, "texCoord");

  glEnableVertexAttribArray(location_pos);
  glVertexAttribPointer(location_pos, 2, GL_FLOAT, GL_FALSE,
                        4 * sizeof(GLfloat), (const void*)0);

  glEnableVertexAttribArray(location_tex);
  glVertexAttribPointer(location_tex, 2, GL_FLOAT, GL_FALSE,
                        4 * sizeof(GLfloat),
                        (const void*)(2 * sizeof(GLfloat)));

  initialized_ = true;
  return true;
}

void GlPipelineHelper::Destroy() {
  if (!initialized_) {
    return;
  }

  glDeleteBuffers(1, &vbo_);
  glDeleteVertexArrays(1, &vao_);
  glDeleteProgram(program_);
  initialized_ = false;
}

bool GlPipelineHelper::Bind() {
  if (!initialized_) {
    return false;
  }

  glGetIntegerv(GL_CURRENT_PROGRAM, &saved_program_);
  glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &saved_vao_);

  // Bind our program and VAO
  glUseProgram(program_);
  glBindVertexArray(vao_);
  return true;
}

void GlPipelineHelper::UnBind() {
  if (!initialized_) {
    return;
  }
  // Restore previous program and VAO state
  glUseProgram(saved_program_);
  glBindVertexArray(saved_vao_);
}

}  // namespace clay
