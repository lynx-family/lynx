// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/gl/gl_api_windows.h"

#include "clay/fml/logging.h"

namespace clay {

void GlApiWindows::GlGetIntegerv(uint32_t pname, int32_t* data) {
  if (!gl_get_integerv_proc) {
    FML_LOG(ERROR) << "GlApiWindows has no glGetIntegerv proc";
    return;
  }
  gl_get_integerv_proc(pname, data);
}
void GlApiWindows::GlBindFramebuffer(uint32_t target, uint32_t framebuffer) {
  if (!gl_bind_framebuffer_proc) {
    FML_LOG(ERROR) << "GlApiWindows has no glBindFramebuffer proc";
    return;
  }
  gl_bind_framebuffer_proc(target, framebuffer);
}
void GlApiWindows::GlBindTexture(uint32_t target, uint32_t texture) {
  if (!gl_bind_texture_proc) {
    FML_LOG(ERROR) << "GlApiWindows has no glBindTexture proc";
    return;
  }
  gl_bind_texture_proc(target, texture);
}

}  // namespace clay
