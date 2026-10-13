// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_GL_GL_API_COMMON_H_
#define CLAY_COMMON_GRAPHICS_GL_GL_API_COMMON_H_

#include <cstdint>

#include "build/build_config.h"
#include "clay/common/graphics/gl/gl_api.h"

#if defined(OS_ANDROID)

#include <GLES2/gl2.h>

#elif defined(OS_MAC)

#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>

#elif defined(OS_IOS) || defined(OS_TVOS)

#import <OpenGLES/ES2/gl.h>

#elif defined(OS_LINUX)

#include <epoxy/egl.h>
#include <epoxy/gl.h>

#elif defined(OS_WIN)

#include <GLES2/gl2.h>

#else

#include <GLES3/gl32.h>

#endif

namespace clay {

class GlApiCommon : public GlApi {
 public:
  static GlApiCommon* GetInstance();

  void GlGetIntegerv(uint32_t pname, int32_t* data) override;
  void GlBindFramebuffer(uint32_t target, uint32_t framebuffer) override;
  void GlBindTexture(uint32_t target, uint32_t texture) override;
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_GL_GL_API_COMMON_H_
