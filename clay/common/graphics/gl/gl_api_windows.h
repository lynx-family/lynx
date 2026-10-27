// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_COMMON_GRAPHICS_GL_GL_API_WINDOWS_H_
#define CLAY_COMMON_GRAPHICS_GL_GL_API_WINDOWS_H_

#include <Windows.h>

#include <cstdint>

#include "clay/common/graphics/gl/gl_api.h"

typedef void(WINAPI* PFNGLGETINTEGERVPROC)(uint32_t pname, int32_t* data);
typedef void(WINAPI* PFNGLBINDFRAMEBUFFERPROC)(uint32_t target,
                                               uint32_t framebuffer);
typedef void(WINAPI* PFNGLBINDTEXTUREPROC)(uint32_t target, uint32_t texture);

namespace clay {

class GlApiWindows : public GlApi {
 public:
  void GlGetIntegerv(uint32_t pname, int32_t* data) override;
  void GlBindFramebuffer(uint32_t target, uint32_t framebuffer) override;
  void GlBindTexture(uint32_t target, uint32_t texture) override;

  PFNGLGETINTEGERVPROC gl_get_integerv_proc = nullptr;
  PFNGLBINDFRAMEBUFFERPROC gl_bind_framebuffer_proc = nullptr;
  PFNGLBINDTEXTUREPROC gl_bind_texture_proc = nullptr;
};

}  // namespace clay

#endif  // CLAY_COMMON_GRAPHICS_GL_GL_API_WINDOWS_H_
