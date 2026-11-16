// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <memory>

#include "clay/gfx/shared_image/linux_egl_fence_sync.h"

#define CHECK(x)                                                         \
  do {                                                                   \
    if (!(x)) {                                                          \
      std::fprintf(stderr, "FAIL %d: %s (EGL=%x GL=%x)\n", __LINE__, #x, \
                   eglGetError(), glGetError());                         \
      std::abort();                                                      \
    }                                                                    \
  } while (0)

int main() {
  alarm(20);
  EGLDisplay display = EGL_NO_DISPLAY;
  auto platform_display = reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(
      eglGetProcAddress("eglGetPlatformDisplayEXT"));
  if (platform_display) {
    display = platform_display(EGL_PLATFORM_SURFACELESS_MESA,
                               EGL_DEFAULT_DISPLAY, nullptr);
    if (display != EGL_NO_DISPLAY && !eglInitialize(display, nullptr, nullptr))
      display = EGL_NO_DISPLAY;
  }
  if (display == EGL_NO_DISPLAY) {
    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    CHECK(display != EGL_NO_DISPLAY &&
          eglInitialize(display, nullptr, nullptr));
  }
  CHECK(eglBindAPI(EGL_OPENGL_ES_API));
  const EGLint attributes[] = {EGL_SURFACE_TYPE,
                               EGL_PBUFFER_BIT,
                               EGL_RENDERABLE_TYPE,
                               EGL_OPENGL_ES2_BIT,
                               EGL_RED_SIZE,
                               8,
                               EGL_GREEN_SIZE,
                               8,
                               EGL_BLUE_SIZE,
                               8,
                               EGL_ALPHA_SIZE,
                               8,
                               EGL_NONE};
  EGLConfig config;
  EGLint count;
  CHECK(eglChooseConfig(display, attributes, &config, 1, &count) && count == 1);
  const EGLint surface_attributes[] = {EGL_WIDTH, 4, EGL_HEIGHT, 4, EGL_NONE};
  const EGLint context_attributes[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  EGLSurface surface =
      eglCreatePbufferSurface(display, config, surface_attributes);
  EGLContext producer =
      eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
  EGLContext consumer =
      eglCreateContext(display, config, producer, context_attributes);
  CHECK(surface != EGL_NO_SURFACE && producer != EGL_NO_CONTEXT &&
        consumer != EGL_NO_CONTEXT);
  CHECK(eglMakeCurrent(display, surface, surface, producer));
  std::printf("EGL: %s; GL: %s\n", eglQueryString(display, EGL_VENDOR),
              glGetString(GL_RENDERER));
  GLuint texture, producer_fbo, consumer_fbo;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE,
               nullptr);
  glGenFramebuffers(1, &producer_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, producer_fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);
  CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  glClearColor(1, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);
  static const int backend = 0;
  auto fence = std::make_unique<clay::LinuxEGLFenceSync>(
      clay::LinuxEGLFenceSync::Dispatch{&backend, eglGetCurrentDisplay,
                                        eglGetCurrentContext, eglQueryString,
                                        eglGetProcAddress, glFlush});
  using Poll = clay::FenceSync::PollResult;
  CHECK(fence->Poll() == Poll::kPending || fence->Poll() == Poll::kSignaled);
  int fd = -1;
  const auto exported = fence->DupNativeFenceFD(&fd);
  CHECK(exported == clay::FenceSync::NativeExportResult::kExported ||
        exported == clay::FenceSync::NativeExportResult::kUnsupported);
  CHECK(eglMakeCurrent(display, surface, surface, consumer));
  CHECK(fence->EnqueueWait());
  glGenFramebuffers(1, &consumer_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, consumer_fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);
  CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
  unsigned char pixels[64]{};
  // Only the test readback waits; fence operations never CPU-wait.
  glReadPixels(0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  for (int i = 0; i < 64; i += 4)
    CHECK(pixels[i] == 255 && pixels[i + 1] == 0 && pixels[i + 2] == 0 &&
          pixels[i + 3] == 255);
  CHECK(fence->Poll() == Poll::kSignaled);
  glDeleteFramebuffers(1, &consumer_fbo);
  CHECK(eglMakeCurrent(display, surface, surface, producer));
  glDeleteFramebuffers(1, &producer_fbo);
  glDeleteTextures(1, &texture);
  CHECK(
      eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT));
  CHECK(fence->Poll() == Poll::kSignaled);
  fence.reset();
  if (fd >= 0) {
    CHECK(fcntl(fd, F_GETFD) >= 0);
    close(fd);
  }
  eglDestroyContext(display, consumer);
  eglDestroyContext(display, producer);
  eglDestroySurface(display, surface);
  eglTerminate(display);
  std::puts(
      "PASS EGL driver: producer/consumer GPU order, RGBA readback, "
      "context-free poll/destruction and independent FD lifetime");
}
