// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_GL_GL_PIPELINE_HELPER_H_
#define CLAY_COMMON_GRAPHICS_GL_GL_PIPELINE_HELPER_H_

#include <GLES3/gl3.h>

#include "clay/fml/logging.h"

namespace clay {

class GlPipelineHelper {
 public:
  static GlPipelineHelper* GetInstance();
  bool Initialize();
  void Destroy();

  bool Bind();
  void UnBind();

 private:
  bool initialized_ = false;
  GLuint vao_;
  GLuint vbo_;
  GLuint program_;

  // Saved state for restoration
  GLint saved_program_ = 0;
  GLint saved_vao_ = 0;
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_GL_GL_PIPELINE_HELPER_H_
