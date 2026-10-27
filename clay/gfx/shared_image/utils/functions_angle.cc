// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/utils/functions_angle.h"

namespace clay {

template <typename T>
static void GetEGLProcAddress(PFNEGLGETPROCADDRESSPROC eglGetProcAddressProc,
                              const std::string& proc_name,
                              T* out_proc_address) {
  T proc = nullptr;
  proc = reinterpret_cast<T>(eglGetProcAddressProc(proc_name.c_str()));
  *out_proc_address = proc;
}

FunctionsAngle::FunctionsAngle(PFNEGLGETPROCADDRESSPROC eglGetProcAddressProc)
    : eglGetProcAddressProc(eglGetProcAddressProc) {
  GetEGLProcAddress(eglGetProcAddressProc, "eglGetCurrentDisplay",
                    &eglGetCurrentDisplayProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglGetCurrentContext",
                    &eglGetCurrentContextProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglQueryString",
                    &eglQueryStringProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglGetError", &eglGetErrorProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglCreateImageKHR",
                    &eglCreateImageKHRProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglDestroyImageKHR",
                    &eglDestroyImageKHRProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglBindTexImage",
                    &eglBindTexImageProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglReleaseTexImage",
                    &eglReleaseTexImageProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglDestroySurface",
                    &eglDestroySurfaceProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglGetConfigs", &eglGetConfigsProc);
  GetEGLProcAddress(eglGetProcAddressProc, "eglGetConfigAttrib",
                    &eglGetConfigAttribProc);

  GetEGLProcAddress(eglGetProcAddressProc, "glGetError", &glGetErrorProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glGenTextures", &glGenTexturesProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glBindTexture", &glBindTextureProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glTexParameteri",
                    &glTexParameteriProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glGetIntegerv", &glGetIntegervProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glDeleteTextures",
                    &glDeleteTexturesProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glGenFramebuffers",
                    &glGenFramebuffersProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glBindFramebuffer",
                    &glBindFramebufferProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glFramebufferTexture2D",
                    &glFramebufferTexture2DProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glCheckFramebufferStatus",
                    &glCheckFramebufferStatusProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glDeleteFramebuffers",
                    &glDeleteFramebuffersProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glReadPixels", &glReadPixelsProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glActiveTexture",
                    &glActiveTextureProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glClearColor", &glClearColorProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glClear", &glClearProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glFinish", &glFinishProc);
  GetEGLProcAddress(eglGetProcAddressProc, "glFlush", &glFlushProc);

  EGLDisplay display = eglGetCurrentDisplay();
  const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
  if (!strstr(extensions, "EGL_KHR_fence_sync")) {
    angle_fence_supported_ = false;
  } else {
    GetEGLProcAddress(eglGetProcAddressProc, "eglCreateSyncKHR",
                      &eglCreateSyncKHRProc);
    GetEGLProcAddress(eglGetProcAddressProc, "eglDestroySyncKHR",
                      &eglDestroySyncKHRProc);
    GetEGLProcAddress(eglGetProcAddressProc, "eglClientWaitSyncKHR",
                      &eglClientWaitSyncKHRProc);
    GetEGLProcAddress(eglGetProcAddressProc, "eglWaitSyncKHR",
                      &eglWaitSyncKHRProc);
    angle_fence_supported_ = eglCreateSyncKHRProc && eglDestroySyncKHRProc &&
                             eglClientWaitSyncKHRProc && eglWaitSyncKHRProc;
  }
}

#define CHECK_VOID_PROC(proc)                                             \
  if (proc == nullptr) {                                                  \
    FML_LOG(ERROR) << "FunctionsAngle: " << #proc << " is unavailable !"; \
    return;                                                               \
  }

#define CHECK_PROC(proc, res)                                             \
  if (proc == nullptr) {                                                  \
    FML_LOG(ERROR) << "FunctionsAngle: " << #proc << " is unavailable !"; \
    return res;                                                           \
  }

GLenum FunctionsAngle::glGetError() const {
  CHECK_PROC(glGetErrorProc, 0);
  return glGetErrorProc();
}
void FunctionsAngle::glGenTextures(GLsizei n, GLuint* textures) const {
  CHECK_VOID_PROC(glGenTexturesProc)
  glGenTexturesProc(n, textures);
}
void FunctionsAngle::glBindTexture(GLenum target, GLuint texture) const {
  CHECK_VOID_PROC(glBindTextureProc)
  glBindTextureProc(target, texture);
}
void FunctionsAngle::glTexParameteri(GLenum target, GLenum pname,
                                     GLint param) const {
  CHECK_VOID_PROC(glTexParameteriProc)
  glTexParameteriProc(target, pname, param);
}
void FunctionsAngle::glGetIntegerv(GLenum pname, GLint* data) const {
  CHECK_VOID_PROC(glGetIntegervProc)
  glGetIntegervProc(pname, data);
}
void FunctionsAngle::glDeleteTextures(GLsizei n, const GLuint* textures) const {
  CHECK_VOID_PROC(glDeleteTexturesProc)
  glDeleteTexturesProc(n, textures);
}
void FunctionsAngle::glGenFramebuffers(GLsizei n, GLuint* framebuffers) const {
  CHECK_VOID_PROC(glGenFramebuffersProc)
  glGenFramebuffersProc(n, framebuffers);
}
void FunctionsAngle::glBindFramebuffer(GLenum target,
                                       GLuint framebuffer) const {
  CHECK_VOID_PROC(glBindFramebufferProc)
  glBindFramebufferProc(target, framebuffer);
}
void FunctionsAngle::glFramebufferTexture2D(GLenum target, GLenum attachment,
                                            GLenum textarget, GLuint texture,
                                            GLint level) const {
  CHECK_VOID_PROC(glFramebufferTexture2DProc)
  glFramebufferTexture2DProc(target, attachment, textarget, texture, level);
}
GLenum FunctionsAngle::glCheckFramebufferStatus(GLenum target) const {
  CHECK_PROC(glCheckFramebufferStatusProc, 0)
  return glCheckFramebufferStatusProc(target);
}
void FunctionsAngle::glDeleteFramebuffers(GLsizei n,
                                          const GLuint* framebuffers) const {
  CHECK_VOID_PROC(glDeleteFramebuffersProc)
  glDeleteFramebuffersProc(n, framebuffers);
}
void FunctionsAngle::glReadPixels(GLint x, GLint y, GLsizei width,
                                  GLsizei height, GLenum format, GLenum type,
                                  void* pixels) const {
  CHECK_VOID_PROC(glReadPixelsProc)
  glReadPixelsProc(x, y, width, height, format, type, pixels);
}
void FunctionsAngle::glActiveTexture(GLenum texture) const {
  CHECK_VOID_PROC(glActiveTextureProc)
  glActiveTextureProc(texture);
}
void FunctionsAngle::glClearColor(GLfloat red, GLfloat green, GLfloat blue,
                                  GLfloat alpha) const {
  CHECK_VOID_PROC(glClearColorProc)
  glClearColorProc(red, green, blue, alpha);
}
void FunctionsAngle::glClear(GLbitfield mask) const {
  CHECK_VOID_PROC(glClearProc)
  glClearProc(mask);
}
void FunctionsAngle::glFinish() const {
  CHECK_VOID_PROC(glFinishProc)
  glFinishProc();
}
void FunctionsAngle::glFlush() const {
  CHECK_VOID_PROC(glFlushProc)
  glFlushProc();
}

EGLDisplay FunctionsAngle::eglGetCurrentDisplay() const {
  CHECK_PROC(eglGetCurrentDisplayProc, nullptr);
  return eglGetCurrentDisplayProc();
}

EGLContext FunctionsAngle::eglGetCurrentContext() const {
  CHECK_PROC(eglGetCurrentContextProc, EGL_NO_CONTEXT);
  return eglGetCurrentContextProc();
}

const char* FunctionsAngle::eglQueryString(EGLDisplay dpy, EGLint name) const {
  CHECK_PROC(eglQueryStringProc, nullptr);
  return eglQueryStringProc(dpy, name);
}

EGLint FunctionsAngle::eglGetError() const {
  CHECK_PROC(eglGetErrorProc, 0);
  return eglGetErrorProc();
}

EGLImageKHR FunctionsAngle::eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx,
                                              EGLenum target,
                                              EGLClientBuffer buffer,
                                              const EGLint* attrib_list) const {
  CHECK_PROC(eglCreateImageKHRProc, nullptr);
  return eglCreateImageKHRProc(dpy, ctx, target, buffer, attrib_list);
}

EGLBoolean FunctionsAngle::eglDestroyImageKHR(EGLDisplay dpy,
                                              EGLImageKHR image) const {
  CHECK_PROC(eglDestroyImageKHRProc, false);
  return eglDestroyImageKHRProc(dpy, image);
}

EGLBoolean FunctionsAngle::eglBindTexImage(EGLDisplay dpy, EGLSurface surface,
                                           EGLint buffer) const {
  CHECK_PROC(eglBindTexImageProc, false);
  return eglBindTexImageProc(dpy, surface, buffer);
}

EGLBoolean FunctionsAngle::eglReleaseTexImage(EGLDisplay dpy,
                                              EGLSurface surface,
                                              EGLint buffer) const {
  CHECK_PROC(eglReleaseTexImageProc, false);
  return eglReleaseTexImageProc(dpy, surface, buffer);
}

EGLBoolean FunctionsAngle::eglDestroySurface(EGLDisplay dpy,
                                             EGLSurface surface) const {
  CHECK_PROC(eglDestroySurfaceProc, false);
  return eglDestroySurfaceProc(dpy, surface);
}

EGLBoolean FunctionsAngle::eglGetConfigs(EGLDisplay dpy, EGLConfig* configs,
                                         EGLint config_size,
                                         EGLint* num_config) const {
  CHECK_PROC(eglGetConfigsProc, false);
  return eglGetConfigsProc(dpy, configs, config_size, num_config);
}

EGLBoolean FunctionsAngle::eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
                                              EGLint attribute,
                                              EGLint* value) const {
  CHECK_PROC(eglGetConfigAttribProc, false);
  return eglGetConfigAttribProc(dpy, config, attribute, value);
}

EGLSyncKHR FunctionsAngle::eglCreateSyncKHR(EGLDisplay dpy, EGLenum type,
                                            const EGLint* attrib_list) const {
  CHECK_PROC(eglCreateSyncKHRProc, nullptr)
  return eglCreateSyncKHRProc(dpy, type, attrib_list);
}
EGLBoolean FunctionsAngle::eglDestroySyncKHR(EGLDisplay dpy,
                                             EGLSyncKHR sync) const {
  CHECK_PROC(eglDestroySyncKHRProc, false)
  return eglDestroySyncKHRProc(dpy, sync);
}
EGLint FunctionsAngle::eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync,
                                            EGLint flags,
                                            EGLTimeKHR timeout) const {
  CHECK_PROC(eglClientWaitSyncKHRProc, 0)
  return eglClientWaitSyncKHRProc(dpy, sync, flags, timeout);
}
EGLint FunctionsAngle::eglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync,
                                      EGLint flags) const {
  CHECK_PROC(eglWaitSyncKHRProc, 0)
  return eglWaitSyncKHRProc(dpy, sync, flags);
}

}  // namespace clay
