// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Foundation/Foundation.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/ES3/gl.h>

#include <cstdio>
#include <cstdlib>
#include "clay/gfx/shared_image/cv_pixelbuffer_image_backing.h"
#include "clay/gfx/shared_image/eagl_image_representation.h"
#include "clay/gfx/shared_image/fence_sync.h"

#define CHECK(x)                                          \
  do {                                                    \
    if (!(x)) {                                           \
      std::fprintf(stderr, "FAIL %d %s\n", __LINE__, #x); \
      std::abort();                                       \
    }                                                     \
  } while (0)

namespace clay {
CVPixelBufferImageBacking::CVPixelBufferImageBacking(PixelFormat format, skity::Vec2 size,
                                                     std::optional<GraphicsMemoryHandle>)
    : SharedImageBacking(format, size) {}
SharedImageBacking::BackingType CVPixelBufferImageBacking::GetType() const {
  return BackingType::kCVPixelBuffer;
}
GraphicsMemoryHandle CVPixelBufferImageBacking::GetGFXHandle() const { return nullptr; }
fml::RefPtr<SharedImageRepresentation> CVPixelBufferImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig*) {
  return nullptr;
}
fml::RefPtr<SkityImageRepresentation> CVPixelBufferImageBacking::CreateSkityRepresentation(
    GrContext*) {
  return nullptr;
}
bool CVPixelBufferImageBacking::ReadbackToMemory(SharedImageReadbackPixmap*, uint32_t) {
  return false;
}
}  // namespace clay

class Failed final : public clay::FenceSync {
 public:
  bool ClientWait() override { return false; }
  Type GetType() const override { return Type::kClientWaitOnly; }
};

void Run(EAGLRenderingAPI api) {
  @autoreleasepool {
    EAGLContext* context = [[EAGLContext alloc] initWithAPI:api];
    CHECK(context && [EAGLContext setCurrentContext:context]);
    auto backing = fml::MakeRefCounted<clay::CVPixelBufferImageBacking>(
        clay::SharedImageBacking::PixelFormat::kRGBA8888, skity::Vec2(4, 4));
    auto rep = fml::MakeRefCounted<clay::EAGLImageRepresentation>(backing);
    rep->ConsumeFence(std::make_unique<Failed>());
    rep->ConsumeFence(nullptr);
    ClaySharedImageReadResult read{};
    ClaySharedImageWriteResult write{};
    CHECK(!rep->BeginRead(&read) && !rep->BeginWrite(&write));
    CHECK(!rep->EndRead() && !rep->EndWrite());
    CHECK(!rep->ProduceFence()->ClientWait());
    CHECK(rep->ProduceFence()->Poll() == clay::FenceSync::PollResult::kFailed);
    rep = fml::MakeRefCounted<clay::EAGLImageRepresentation>(backing);
    GLuint texture, framebuffer;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glClearColor(1, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(0xdead);
    auto complete = rep->ProduceFence();
    CHECK([EAGLContext setCurrentContext:nil]);
    CHECK(complete->ClientWait() && !complete->EnqueueWait());
    CHECK([EAGLContext currentContext] == nil);
    EAGLContext* shared = [[EAGLContext alloc] initWithAPI:api sharegroup:context.sharegroup];
    EAGLContext* unrelated = [[EAGLContext alloc] initWithAPI:api];
    CHECK(shared && unrelated && [EAGLContext setCurrentContext:unrelated]);
    CHECK(complete->EnqueueWaitResult() == clay::FenceSync::WaitResult::kUnsupported);
    CHECK(complete->Poll() == clay::FenceSync::PollResult::kSignaled);
    CHECK([EAGLContext currentContext] == unrelated);
    CHECK([EAGLContext setCurrentContext:shared]);
    CHECK(complete->EnqueueWait());
    CHECK([EAGLContext currentContext] == shared);
    CHECK([EAGLContext setCurrentContext:nil]);
    CHECK(complete->Poll() == clay::FenceSync::PollResult::kSignaled);

    CHECK(!rep->ProduceFence()->ClientWait());
    CHECK([EAGLContext setCurrentContext:context]);
    unsigned char pixels[4 * 4 * 4]{};
    glReadPixels(0, 0, 4, 4, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    CHECK(pixels[0] == 255 && pixels[1] == 0 && pixels[2] == 0 && pixels[3] == 255);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &texture);
    rep = nullptr;
    CHECK([EAGLContext setCurrentContext:nil]);
    context = nil;
    shared = nil;
    CHECK(complete->Poll() == clay::FenceSync::PollResult::kSignaled);
    complete.reset();
    std::puts("PASS EAGL simulator: failure propagation, GPU completion, stale GL error, "
              "context-free wait/poll/destruction, missing context");
  }
}

int main() {
  Run(kEAGLRenderingAPIOpenGLES2);
  Run(kEAGLRenderingAPIOpenGLES3);
}
