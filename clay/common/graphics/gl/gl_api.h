// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_GL_GL_API_H_
#define CLAY_COMMON_GRAPHICS_GL_GL_API_H_

#include <cstdint>

namespace clay {

class GlApi {
 public:
  virtual void GlGetIntegerv(uint32_t pname, int32_t* data) = 0;
  virtual void GlBindFramebuffer(uint32_t target, uint32_t framebuffer) = 0;
  virtual void GlBindTexture(uint32_t target, uint32_t texture) = 0;
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_GL_GL_API_H_
