// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/common/graphics/gl/scoped_framebuffer_binder.h"

#include "clay/fml/logging.h"

namespace clay {

typedef unsigned int GLenum;
#define GL_FRAMEBUFFER 0x8D40
#define GL_FRAMEBUFFER_BINDING 0x8CA6

ScopedFramebufferBinder::ScopedFramebufferBinder(unsigned int target,
                                                 unsigned int framebuffer,
                                                 GlApi* gl_api)
    : target_(target), old_framebuffer_(-1), gl_api_(gl_api) {
  if (!gl_api_) {
    FML_LOG(ERROR) << "Not get valid GLProc for ScopedFramebufferBinder";
    return;
  }
  GLenum target_getter = 0;
  switch (target) {
    case GL_FRAMEBUFFER:
      target_getter = GL_FRAMEBUFFER_BINDING;
      break;
    default:
      FML_LOG(ERROR) << "Target not supported. " << target;
  }
  gl_api_->GlGetIntegerv(target_getter, &old_framebuffer_);
  gl_api_->GlBindFramebuffer(target_, framebuffer);
}

ScopedFramebufferBinder::~ScopedFramebufferBinder() {
  gl_api_->GlBindFramebuffer(target_, old_framebuffer_);
}

}  // namespace clay
