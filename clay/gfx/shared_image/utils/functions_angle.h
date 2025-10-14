// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_UTILS_FUNCTIONS_ANGLE_H_
#define CLAY_GFX_SHARED_IMAGE_UTILS_FUNCTIONS_ANGLE_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "clay/fml/logging.h"

namespace clay {

#define GL_RGBA8 0x8058

class FunctionsAngle {
 public:
  explicit FunctionsAngle(PFNEGLGETPROCADDRESSPROC eglGetProcAddressProc);
  ~FunctionsAngle() = default;

  GLenum glGetError() const;
  void glGenTextures(GLsizei n, GLuint* textures) const;
  void glBindTexture(GLenum target, GLuint texture) const;
  void glTexParameteri(GLenum target, GLenum pname, GLint param) const;
  void glGetIntegerv(GLenum pname, GLint* data) const;
  void glDeleteTextures(GLsizei n, const GLuint* textures) const;
  void glGenFramebuffers(GLsizei n, GLuint* framebuffers) const;
  void glBindFramebuffer(GLenum target, GLuint framebuffer) const;
  void glFramebufferTexture2D(GLenum target, GLenum attachment,
                              GLenum textarget, GLuint texture,
                              GLint level) const;
  GLenum glCheckFramebufferStatus(GLenum target) const;
  void glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers) const;
  void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                    GLenum format, GLenum type, void* pixels) const;
  void glActiveTexture(GLenum texture) const;
  void glClearColor(GLfloat red, GLfloat green, GLfloat blue,
                    GLfloat alpha) const;
  void glClear(GLbitfield mask) const;
  void glFinish() const;
  void glFlush() const;

  EGLDisplay eglGetCurrentDisplay() const;
  EGLContext eglGetCurrentContext() const;

  const char* eglQueryString(EGLDisplay dpy, EGLint name) const;
  EGLint eglGetError() const;
  EGLImageKHR eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target,
                                EGLClientBuffer buffer,
                                const EGLint* attrib_list) const;
  EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image) const;
  EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface,
                             EGLint buffer) const;
  EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface,
                                EGLint buffer) const;
  EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface) const;
  EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig* configs,
                           EGLint config_size, EGLint* num_config) const;
  EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
                                EGLint attribute, EGLint* value) const;

  EGLSyncKHR eglCreateSyncKHR(EGLDisplay dpy, EGLenum type,
                              const EGLint* attrib_list) const;
  EGLBoolean eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync) const;
  EGLint eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags,
                              EGLTimeKHR timeout) const;
  EGLint eglWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags) const;

  PFNGLGETERRORPROC glGetErrorProc = nullptr;
  PFNGLGENTEXTURESPROC glGenTexturesProc = nullptr;
  PFNGLBINDTEXTUREPROC glBindTextureProc = nullptr;
  PFNGLTEXPARAMETERIPROC glTexParameteriProc = nullptr;
  PFNGLGETINTEGERVPROC glGetIntegervProc = nullptr;
  PFNGLDELETETEXTURESPROC glDeleteTexturesProc = nullptr;
  PFNGLGENFRAMEBUFFERSPROC glGenFramebuffersProc = nullptr;
  PFNGLBINDFRAMEBUFFERPROC glBindFramebufferProc = nullptr;
  PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2DProc = nullptr;
  PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatusProc = nullptr;
  PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffersProc = nullptr;
  PFNGLREADPIXELSPROC glReadPixelsProc = nullptr;
  PFNGLACTIVETEXTUREPROC glActiveTextureProc = nullptr;
  PFNGLCLEARCOLORPROC glClearColorProc = nullptr;
  PFNGLCLEARPROC glClearProc = nullptr;
  PFNGLFINISHPROC glFinishProc = nullptr;
  PFNGLFLUSHPROC glFlushProc = nullptr;

  PFNEGLCREATESYNCKHRPROC eglCreateSyncKHRProc = nullptr;
  PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHRProc = nullptr;
  PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHRProc = nullptr;
  PFNEGLWAITSYNCKHRPROC eglWaitSyncKHRProc = nullptr;
  PFNEGLGETCURRENTDISPLAYPROC eglGetCurrentDisplayProc = nullptr;
  PFNEGLGETCURRENTCONTEXTPROC eglGetCurrentContextProc = nullptr;
  PFNEGLQUERYSTRINGPROC eglQueryStringProc = nullptr;
  PFNEGLGETERRORPROC eglGetErrorProc = nullptr;
  PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHRProc = nullptr;
  PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHRProc = nullptr;
  PFNEGLBINDTEXIMAGEPROC eglBindTexImageProc = nullptr;
  PFNEGLRELEASETEXIMAGEPROC eglReleaseTexImageProc = nullptr;
  PFNEGLDESTROYSURFACEPROC eglDestroySurfaceProc = nullptr;
  PFNEGLGETCONFIGSPROC eglGetConfigsProc = nullptr;
  PFNEGLGETCONFIGATTRIBPROC eglGetConfigAttribProc = nullptr;

  PFNEGLGETPROCADDRESSPROC eglGetProcAddressProc = nullptr;

  bool angle_fence_supported_ = false;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_UTILS_FUNCTIONS_ANGLE_H_
