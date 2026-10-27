// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/utils/gl_texture_converter.h"

#include "base/include/closure.h"
#include "clay/common/graphics/gl/gl_pipeline_helper.h"
#include "clay/common/graphics/gl/gl_shader_program.h"
#include "clay/common/graphics/gl/scoped_framebuffer_binder.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"

namespace clay {

GLuint Get2DTextureFromExternalTextureOES(GLuint oes_texture_id,
                                          uint32_t texture_width,
                                          uint32_t texture_height) {
  GLint old_framebuffer;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &old_framebuffer);

  GLuint fbo;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  fml::ScopedCleanupClosure restore_fbo([old_framebuffer, fbo]() {
    glBindFramebuffer(GL_FRAMEBUFFER, old_framebuffer);
    glDeleteFramebuffers(1, &fbo);
  });

  GLuint texture_2d;
  glGenTextures(1, &texture_2d);
  clay::ScopedTextureBinder scoped_2d_texture_binder(GL_TEXTURE_2D, texture_2d);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture_width, texture_height, 0,
               GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture_2d, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    FML_LOG(ERROR) << "Framebuffer is not complete";
    glDeleteTextures(1, &texture_2d);
    return 0;
  }

  clay::GlPipelineHelper* pipeline_helper =
      clay::GlPipelineHelper::GetInstance();
  GLint current_program;
  glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
  fml::ScopedCleanupClosure restore_program(
      [program = current_program]() { glUseProgram(program); });
  GLint current_vbo;
  glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &current_vbo);
  fml::ScopedCleanupClosure restore_buffer(
      [vbo = current_vbo]() { glBindBuffer(GL_ARRAY_BUFFER, vbo); });

  if (!pipeline_helper->Initialize()) {
    FML_LOG(ERROR) << "GL pipeline is not complete";
    glDeleteTextures(1, &texture_2d);
    return 0;
  }

  pipeline_helper->Bind();

  GLint active_texture;
  glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
  fml::ScopedCleanupClosure restore_texture(
      [active_texture]() { glActiveTexture(active_texture); });

  glActiveTexture(GL_TEXTURE0);
  clay::ScopedTextureBinder scoped_external_texture_binder(
      GL_TEXTURE_EXTERNAL_OES, oes_texture_id);

  glViewport(0, 0, texture_width, texture_height);
  glClear(GL_COLOR_BUFFER_BIT);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  pipeline_helper->UnBind();
  return texture_2d;
}

}  // namespace clay
