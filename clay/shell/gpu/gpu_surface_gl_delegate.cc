// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/gpu/gpu_surface_gl_delegate.h"

#include <cstring>

#ifndef ENABLE_SKITY
#include "clay/shell/gpu/trace_gl_fuctions.h"
#include "third_party/skia/include/gpu/gl/GrGLAssembleInterface.h"
#endif

namespace clay {

GPUSurfaceGLDelegate::~GPUSurfaceGLDelegate() = default;

bool GPUSurfaceGLDelegate::GLContextFBOResetAfterPresent() const {
  return false;
}

SurfaceFrame::FramebufferInfo GPUSurfaceGLDelegate::GLContextFramebufferInfo()
    const {
  SurfaceFrame::FramebufferInfo res;
  res.supports_readback = true;
  return res;
}

skity::Matrix GPUSurfaceGLDelegate::GLContextSurfaceTransformation() const {
  return skity::Matrix();
}

GPUSurfaceGLDelegate::GLProcResolver GPUSurfaceGLDelegate::GetGLProcResolver()
    const {
  return nullptr;
}

#ifndef ENABLE_SKITY
static bool IsProcResolverOpenGLES(
    const GPUSurfaceGLDelegate::GLProcResolver& proc_resolver) {
  // Version string prefix that identifies an OpenGL ES implementation.
#define GPU_GL_VERSION 0x1F02
  constexpr char kGLESVersionPrefix[] = "OpenGL ES";

#ifdef WIN32
  using GLGetStringProc = const char*(__stdcall*)(uint32_t);  // NOLINT
#else
  using GLGetStringProc = const char* (*)(uint32_t);
#endif  // WIN32

  GLGetStringProc gl_get_string =
      reinterpret_cast<GLGetStringProc>(proc_resolver("glGetString"));

  FML_CHECK(gl_get_string)
      << "The GL proc resolver could not resolve glGetString";

  const char* gl_version_string = gl_get_string(GPU_GL_VERSION);

  FML_CHECK(gl_version_string)
      << "The GL proc resolver's glGetString(GL_VERSION) failed";

  return strncmp(gl_version_string, kGLESVersionPrefix,
                 strlen(kGLESVersionPrefix)) == 0;
}

sk_sp<const GrGLInterface> GPUSurfaceGLDelegate::CreateGLInterface(
    const GPUSurfaceGLDelegate::GLProcResolver& proc_resolver) {
  if (proc_resolver == nullptr) {
    static std::once_flag flag;
    static sk_sp<const GrGLInterface> interface;
    static const auto create_func = []() {
      // If there is no custom proc resolver, ask Skia to guess the native
      // interface. This often leads to interesting results on most platforms.
      interface = GrGLMakeNativeInterface();
#if ENABLE_GL_FUNCTION_TRACE
      GrGLInterface* interface_ptr =
          const_cast<GrGLInterface*>(interface.get());
      TraceGlFuctions::ReplaceFunctions(interface_ptr);
#endif  // ENABLE_GL_FUNCTION_TRACE
    };
    std::call_once(flag, create_func);
    return interface;
  }

  struct ProcResolverContext {
    GPUSurfaceGLDelegate::GLProcResolver resolver;
  };

  ProcResolverContext context = {proc_resolver};

  GrGLGetProc gl_get_proc = [](void* context,
                               const char gl_proc_name[]) -> GrGLFuncPtr {
    auto proc_resolver_context =
        reinterpret_cast<ProcResolverContext*>(context);
    return reinterpret_cast<GrGLFuncPtr>(
        proc_resolver_context->resolver(gl_proc_name));
  };

  // glGetString indicates an OpenGL ES interface.
  if (IsProcResolverOpenGLES(proc_resolver)) {
    sk_sp<const GrGLInterface> interface =
        GrGLMakeAssembledGLESInterface(&context, gl_get_proc);
#if ENABLE_GL_FUNCTION_TRACE
    GrGLInterface* interface_ptr = const_cast<GrGLInterface*>(interface.get());
    TraceGlFuctions::ReplaceFunctions(interface_ptr);
#endif  // ENABLE_GL_FUNCTION_TRACE
    return interface;
  }

  // Fallback to OpenGL.
  if (auto interface = GrGLMakeAssembledGLInterface(&context, gl_get_proc)) {
    return interface;
  }
  FML_LOG(ERROR) << "Could not create a valid GL interface.";
  return nullptr;
}

sk_sp<const GrGLInterface> GPUSurfaceGLDelegate::GetGLInterface() const {
  return CreateGLInterface(GetGLProcResolver());
}

sk_sp<const GrGLInterface>
GPUSurfaceGLDelegate::GetDefaultPlatformGLInterface() {
  return CreateGLInterface(nullptr);
}
#endif  // ENABLE_SKITY

bool GPUSurfaceGLDelegate::AllowsDrawingWhenGpuDisabled() const { return true; }

int GPUSurfaceGLDelegate::GetSampleCount() const { return 1; }

}  // namespace clay
