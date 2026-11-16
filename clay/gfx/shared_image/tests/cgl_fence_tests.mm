// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
// cspell:ignore CGLPFA CGLOGLP

#import <Foundation/Foundation.h>
#import <IOSurface/IOSurface.h>
#import <OpenGL/OpenGL.h>
#import <OpenGL/gl3.h>

#include <cstdio>
#include <cstdlib>
#include "clay/gfx/shared_image/cgl_image_representation.h"
#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/gfx/shared_image/tests/api_contract.h"

#define CHECK_PROBE(expr)                                     \
  do {                                                        \
    if (!(expr)) {                                            \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #expr); \
      std::exit(1);                                           \
    }                                                         \
  } while (0)

class ProbeBacking final : public clay::SharedImageBacking {
 public:
  ProbeBacking() : SharedImageBacking(PixelFormat::kNative8888, {4, 4}) {
    surface_ = IOSurfaceCreate((__bridge CFDictionaryRef) @{
      (id)kIOSurfaceWidth : @4,
      (id)kIOSurfaceHeight : @4,
      (id)kIOSurfaceBytesPerElement : @4,
      (id)kIOSurfacePixelFormat : @((uint32_t)'BGRA')
    });
    CHECK_PROBE(surface_);
  }
  ~ProbeBacking() override { CFRelease(surface_); }
  BackingType GetType() const override { return BackingType::kIOSurface; }
  clay::GraphicsMemoryHandle GetGFXHandle() const override { return surface_; }
  fml::RefPtr<clay::SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig*) override {
    return nullptr;
  }
  fml::RefPtr<clay::SkityImageRepresentation> CreateSkityRepresentation(clay::GrContext*) override {
    return nullptr;
  }

 private:
  IOSurfaceRef surface_;
};
class Failure final : public clay::FenceSync {
 public:
  explicit Failure(int* waits) : waits_(waits) {}
  bool ClientWait() override {
    ++*waits_;
    return false;
  }
  Type GetType() const override { return Type::kClientWaitOnly; }

 private:
  int* waits_;
};
int main() {
  @autoreleasepool {
    CGLPixelFormatAttribute attrs[] = {kCGLPFAOpenGLProfile,
                                       (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core,
                                       (CGLPixelFormatAttribute)0};
    CGLPixelFormatObj format = nullptr;
    GLint count = 0;
    CHECK_PROBE(CGLChoosePixelFormat(attrs, &format, &count) == kCGLNoError);
    CGLContextObj context = nullptr;
    CHECK_PROBE(CGLCreateContext(format, nullptr, &context) == kCGLNoError);
    CGLReleasePixelFormat(format);
    CHECK_PROBE(CGLSetCurrentContext(context) == kCGLNoError);
    {
      auto backing = fml::MakeRefCounted<ProbeBacking>();
      auto representation = fml::MakeRefCounted<clay::CGLImageRepresentation>(backing);
      int waits = 0;
      representation->ConsumeFence(std::make_unique<Failure>(&waits));
      CHECK_PROBE(waits == 1);
      representation->ConsumeFence(nullptr);
      ClaySharedImageReadResult read{};
      CHECK_PROBE(!representation->BeginRead(&read));
      CHECK_PROBE(!representation->EndRead());
      ClaySharedImageWriteResult write{};
      CHECK_PROBE(!representation->BeginWrite(&write));
      CHECK_PROBE(!representation->EndWrite());
      auto returned = representation->ProduceFence();
      CHECK_PROBE(returned && !returned->ClientWait());
      CHECK_PROBE(!representation->ProduceFence()->ClientWait());
      auto recreated = fml::MakeRefCounted<clay::CGLImageRepresentation>(backing);
      recreated->ConsumeFence(std::move(returned));
      CHECK_PROBE(!recreated->BeginRead(&read));
      std::puts(
          "PASS CGL: failed/null waits block access and survive fence return and recreation.");
    }
    {
      auto backing = fml::MakeRefCounted<ProbeBacking>();
      auto representation = fml::MakeRefCounted<clay::CGLImageRepresentation>(backing);
      ClaySharedImageWriteResult write{};
      CHECK_PROBE(representation->BeginWrite(&write));
      glBindFramebuffer(GL_FRAMEBUFFER, write.opengl_framebuffer.name);
      glClearColor(1, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      CHECK_PROBE(representation->EndWrite());
      glEnable(0xdead);  // Recoverable stale error must not reject completion.
      auto complete = representation->ProduceFence();
      CGLSetCurrentContext(nullptr);
      CHECK_PROBE(complete && complete->ClientWait());
      CHECK_PROBE(complete->Poll() == clay::FenceSync::PollResult::kSignaled);
      CHECK_PROBE(CGLGetCurrentContext() == nullptr);
      CHECK_PROBE(!complete->EnqueueWait());
      CGLContextObj shared = nullptr, unrelated = nullptr;
      CHECK_PROBE(CGLCreateContext(CGLGetPixelFormat(context), context, &shared) == kCGLNoError);
      CHECK_PROBE(CGLCreateContext(CGLGetPixelFormat(context), nullptr, &unrelated) == kCGLNoError);
      CGLSetCurrentContext(unrelated);
      CHECK_PROBE(complete->EnqueueWaitResult() == clay::FenceSync::WaitResult::kUnsupported);
      CHECK_PROBE(complete->Poll() == clay::FenceSync::PollResult::kSignaled);
      CHECK_PROBE(CGLGetCurrentContext() == unrelated);
      CGLSetCurrentContext(shared);
      CHECK_PROBE(testFenceServerWait(reinterpret_cast<ClayFenceSyncRef>(complete.get())));
      CHECK_PROBE(CGLGetCurrentContext() == shared);
      CGLSetCurrentContext(nullptr);
      CGLReleaseContext(shared);
      CGLReleaseContext(unrelated);
      IOSurfaceRef surface = static_cast<IOSurfaceRef>(backing->GetGFXHandle());
      CHECK_PROBE(IOSurfaceLock(surface, kIOSurfaceLockReadOnly, nullptr) == 0);
      const auto* pixels = static_cast<const unsigned char*>(IOSurfaceGetBaseAddress(surface));
      CHECK_PROBE(pixels[0] == 0 && pixels[1] == 0 && pixels[2] == 255 && pixels[3] == 255);
      IOSurfaceUnlock(surface, kIOSurfaceLockReadOnly, nullptr);
      // Retain the fence past representation destruction below.

      CHECK_PROBE(!representation->ProduceFence()->ClientWait());
      CGLSetCurrentContext(context);
      write.opengl_framebuffer.destruction_callback(write.opengl_framebuffer.user_data);
      representation = nullptr;
      CGLSetCurrentContext(nullptr);
      CHECK_PROBE(complete->Poll() == clay::FenceSync::PollResult::kSignaled);
      complete.reset();
      CGLSetCurrentContext(context);
      std::puts("PASS CGL: GPU pixel completion, stale GL error, context-free "
                "wait/poll/destruction, missing-context failure.");
    }
    CGLSetCurrentContext(nullptr);
    CGLReleaseContext(context);
  }
}
