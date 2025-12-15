// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/headless/gl/clay_headless_renderer_cgl.h"

#include <memory>

#include "clay/shell/platform/headless/clay_headless_engine.h"

namespace clay {

ClayHeadlessRendererCGL::ClayHeadlessRendererCGL(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config)
    : ClayHeadlessRendererSharedImageGL(engine, config) {}

ClayHeadlessRendererCGL ::~ClayHeadlessRendererCGL() {
  if (gl_context_ != nullptr) {
    CGLDestroyContext(gl_context_);
  }
}

bool ClayHeadlessRendererCGL::MakeCurrent() {
  if (gl_context_ == nullptr) {
    // https://developer.apple.com/library/archive/documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_pixelformats/opengl_pixelformats.html
    CGLPixelFormatAttribute attrib_list[] = {
        kCGLPFAAllowOfflineRenderers, kCGLPFAAccelerated, kCGLPFAOpenGLProfile,
        static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        static_cast<CGLPixelFormatAttribute>(0)};

    CGLPixelFormatObj pixel_format;
    GLint num_formats = 0;
    if (CGLChoosePixelFormat(attrib_list, &pixel_format, &num_formats) !=
        kCGLNoError) {
      return false;
    }

    CGLError error = CGLCreateContext(pixel_format, nullptr, &gl_context_);

    CGLDestroyPixelFormat(pixel_format);

    if (error != kCGLNoError) {
      return false;
    }
  }

  return CGLSetCurrentContext(gl_context_) == kCGLNoError;
}

bool ClayHeadlessRendererCGL::ClearCurrent() {
  return CGLSetCurrentContext(nullptr) == kCGLNoError;
}

std::unique_ptr<ClayHeadlessRenderer> ClayHeadlessRenderer::CreateGL(
    ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config) {
  return std::make_unique<ClayHeadlessRendererCGL>(engine, config);
}

}  // namespace clay
