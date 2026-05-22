// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/gl/gl_api_common.h"

namespace clay {

GlApiCommon* GlApiCommon::GetInstance() {
  static GlApiCommon instance;
  return &instance;
}

void GlApiCommon::GlGetIntegerv(uint32_t pname, int32_t* data) {
  glGetIntegerv(pname, data);
}
void GlApiCommon::GlBindFramebuffer(uint32_t target, uint32_t framebuffer) {
  glBindFramebuffer(target, framebuffer);
}
void GlApiCommon::GlBindTexture(uint32_t target, uint32_t texture) {
  glBindTexture(target, texture);
}
}  // namespace clay
