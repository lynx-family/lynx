// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <utility>

#include "clay/gfx/shared_image/android_egl_image_representation.h"
#include "clay/gfx/shared_image/android_surface_texture_image_backing.h"
#include "clay/gfx/shared_image/tests/surface_texture_host_fixture.h"

#define FENCE_CHECK(condition)                                     \
  do {                                                             \
    if (!(condition)) {                                            \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #condition); \
      std::abort();                                                \
    }                                                              \
  } while (0)

namespace {
using Backing = clay::SurfaceTextureImageBacking;
using Fence = clay::FenceSync;
using Result = Backing::UpdateResult;
using clay::surface_test::attach_succeeds;
using clay::surface_test::attaches;
using clay::surface_test::frame_callback;
using clay::surface_test::update_succeeds;
using clay::surface_test::updates;

EGLContext context = reinterpret_cast<EGLContext>(1);
const char* extensions = "EGL_KHR_fence_sync EGL_KHR_wait_sync";
bool create_fails = false;
int finishes = 0, cpu_waits = 0, gpu_waits = 0, live_syncs = 0;
EGLint poll_result = EGL_TIMEOUT_EXPIRED_KHR;

EGLSyncKHR Create(EGLDisplay, EGLenum, const EGLint*) {
  if (create_fails) return EGL_NO_SYNC_KHR;
  ++live_syncs;
  return reinterpret_cast<EGLSyncKHR>(new int(0));
}
EGLBoolean Destroy(EGLDisplay, EGLSyncKHR sync) {
  --live_syncs;
  delete reinterpret_cast<int*>(sync);
  return EGL_TRUE;
}
EGLint WaitClient(EGLDisplay, EGLSyncKHR, EGLint, EGLTimeKHR timeout) {
  if (timeout) ++cpu_waits;
  return poll_result;
}
EGLBoolean WaitGPU(EGLDisplay, EGLSyncKHR, EGLint) {
  ++gpu_waits;
  return EGL_TRUE;
}

class CallbackFence final : public Fence {
 public:
  CallbackFence(std::function<bool()> wait, fml::closure destroy = {})
      : wait_(std::move(wait)), destroy_(std::move(destroy)) {}
  ~CallbackFence() override {
    if (destroy_) destroy_();
  }
  Type GetType() const override { return Type::kClientWaitOnly; }
  bool ClientWait() override { return wait_(); }

 private:
  std::function<bool()> wait_;
  fml::closure destroy_;
};

struct Fixture {
  Fixture() {
    FENCE_CHECK(live_syncs == 0);
    context = reinterpret_cast<EGLContext>(1);
    extensions = "EGL_KHR_fence_sync EGL_KHR_wait_sync";
    create_fails = false;
    attach_succeeds = update_succeeds = true;
    finishes = cpu_waits = gpu_waits = updates = attaches = 0;
    poll_result = EGL_TIMEOUT_EXPIRED_KHR;
    backing = fml::MakeRefCounted<Backing>(
        clay::SharedImageBacking::PixelFormat::kRGBA8888, skity::Vec2{4, 4});
  }
  ~Fixture() {
    context = reinterpret_cast<EGLContext>(1);
    backing->DetachGLContext();
    backing = nullptr;
    FENCE_CHECK(live_syncs == 0);
  }
  void Frame() {
    FENCE_CHECK(frame_callback);
    frame_callback();
  }
  fml::RefPtr<Backing> backing;
};

void TestTryAndLegacyUpdates() {
  Fixture f;
  FENCE_CHECK(f.backing->TryUpdateFront() == Result::kPending);
  f.Frame();
  int waits = 0, deletes = 0;
  f.backing->AcceptFence(std::make_unique<CallbackFence>(
      [&] {
        ++waits;
        return true;
      },
      [&] { ++deletes; }));
  FENCE_CHECK(f.backing->TryUpdateFront() == Result::kUnsupported);
  FENCE_CHECK(waits == 0 && deletes == 0 && updates == 0);
  FENCE_CHECK(finishes == 0 && cpu_waits == 0);
  FENCE_CHECK(f.backing->UpdateFront());
  FENCE_CHECK(waits == 1 && deletes == 1 && updates == 1);
  FENCE_CHECK(f.backing->TryUpdateFront() == Result::kSuccess);
  FENCE_CHECK(updates == 1);  // No new notification, no relatch.
  FENCE_CHECK(f.backing->BeginRead());
  f.Frame();
  FENCE_CHECK(f.backing->TryUpdateFront() == Result::kPending);
  FENCE_CHECK(updates == 1);
  FENCE_CHECK(f.backing->EndRead());
  FENCE_CHECK(f.backing->TryUpdateFront() == Result::kSuccess);
  FENCE_CHECK(updates == 2 && finishes == 0 && cpu_waits == 0);
}

void TestCapabilitiesAndFailures() {
  for (bool missing : {true, false}) {
    Fixture f;
    f.Frame();
    extensions = missing ? "" : "EGL_KHR_fence_sync EGL_KHR_wait_sync";
    create_fails = !missing;
    FENCE_CHECK(f.backing->TryUpdateFront() ==
                (missing ? Result::kUnsupported : Result::kFailed));
    FENCE_CHECK(finishes == 0 && cpu_waits == 0);
    if (missing) {
      FENCE_CHECK(updates == 0 && attaches == 0);
      FENCE_CHECK(f.backing->UpdateFront());
      FENCE_CHECK(finishes == 1 && updates == 1);
      FENCE_CHECK(f.backing->ProduceReadFence()->Poll() ==
                  Fence::PollResult::kSignaled);
    } else {
      FENCE_CHECK(!f.backing->BeginRead());
      FENCE_CHECK(!f.backing->UpdateFront());
      FENCE_CHECK(f.backing->ProduceReadFence()->Poll() ==
                  Fence::PollResult::kFailed);
    }
  }
  {
    Fixture f;
    f.Frame();
    attach_succeeds = false;
    FENCE_CHECK(f.backing->TryUpdateFront() == Result::kFailed);
    FENCE_CHECK(updates == 0);
    attach_succeeds = true;
    FENCE_CHECK(f.backing->TryUpdateFront() == Result::kSuccess);
    FENCE_CHECK(attaches == 2 && updates == 1);
  }
  {
    Fixture f;
    f.Frame();
    update_succeeds = false;
    FENCE_CHECK(f.backing->TryUpdateFront() == Result::kFailed);
    FENCE_CHECK(!f.backing->BeginRead());
    FENCE_CHECK(f.backing->ProduceReadFence()->Poll() ==
                Fence::PollResult::kFailed);
  }
}

void TestCallbackReentry(bool begin_read, bool fail_nested,
                         bool setter_in_wait) {
  Fixture f;
  f.Frame();
  FENCE_CHECK(f.backing->TryUpdateFront() == Result::kSuccess);
  int waits = 0, deletes = 0, nested_waits = 0;
  const int old_updates = updates;
  auto enqueue = [&](bool setter) {
    auto nested = std::make_unique<CallbackFence>([&] {
      ++nested_waits;
      FENCE_CHECK(!f.backing->BeginRead());
      FENCE_CHECK(f.backing->TryUpdateFront() == Result::kPending);
      return !fail_nested;
    });
    if (setter)
      f.backing->SetFenceSync(std::move(nested));
    else
      FENCE_CHECK(f.backing->AcceptFence(std::move(nested)));
  };
  f.backing->AcceptFence(std::make_unique<CallbackFence>(
      [&] {
        ++waits;
        FENCE_CHECK(!f.backing->BeginRead());
        FENCE_CHECK(f.backing->TryUpdateFront() == Result::kPending);
        enqueue(setter_in_wait);
        return true;
      },
      [&] {
        ++deletes;
        FENCE_CHECK(!f.backing->BeginRead());
        FENCE_CHECK(f.backing->TryUpdateFront() == Result::kPending);
        enqueue(!setter_in_wait);
      }));
  f.Frame();
  const bool success =
      begin_read ? f.backing->BeginRead() : f.backing->UpdateFront();
  FENCE_CHECK(success == !fail_nested);
  FENCE_CHECK(waits == 1 && deletes == 1);
  FENCE_CHECK(nested_waits == (fail_nested ? 1 : 2));
  FENCE_CHECK(updates == old_updates + (!begin_read && !fail_nested ? 1 : 0));
  if (fail_nested) {
    FENCE_CHECK(!f.backing->BeginRead());
    FENCE_CHECK(f.backing->TryUpdateFront() == Result::kFailed);
  } else if (begin_read) {
    FENCE_CHECK(f.backing->EndRead());
  }
  FENCE_CHECK(finishes == 0 && cpu_waits == 0);
}

void TestCallbackContextChange(bool during_delete) {
  Fixture f;
  f.Frame();
  auto change_context = [] { context = reinterpret_cast<EGLContext>(2); };
  f.backing->AcceptFence(std::make_unique<CallbackFence>(
      [&] {
        if (!during_delete) change_context();
        return true;
      },
      [=] {
        if (during_delete) change_context();
      }));
  FENCE_CHECK(!f.backing->UpdateFront());
  FENCE_CHECK(updates == 0);
  context = reinterpret_cast<EGLContext>(1);
  FENCE_CHECK(f.backing->TryUpdateFront() == Result::kFailed);
  FENCE_CHECK(f.backing->ProduceReadFence()->Poll() ==
              Fence::PollResult::kFailed);
}

void TestSnapshotLifetime() {
  std::unique_ptr<Fence> first, second;
  {
    Fixture f;
    f.Frame();
    FENCE_CHECK(f.backing->TryUpdateFront() == Result::kSuccess);
    FENCE_CHECK(f.backing->BeginRead() && f.backing->EndRead());
    first = f.backing->ProduceReadFence();
    second = f.backing->ProduceReadFence();
    FENCE_CHECK(first && second && first.get() != second.get());
    f.Frame();
    FENCE_CHECK(f.backing->TryUpdateFront() == Result::kSuccess);
    FENCE_CHECK(first->Poll() == Fence::PollResult::kPending);
    FENCE_CHECK(first->EnqueueWaitResult() == Fence::WaitResult::kSuccess);
    f.backing->DetachGLContext();
    // Keep the snapshots past backing destruction; release before Fixture's
    // leak check and verify that the last snapshot owns the remaining sync.
    f.backing = fml::MakeRefCounted<Backing>(
        clay::SharedImageBacking::PixelFormat::kRGBA8888, skity::Vec2{4, 4});
    FENCE_CHECK(live_syncs == 1);
    first.reset();
    FENCE_CHECK(live_syncs == 1);
    poll_result = EGL_CONDITION_SATISFIED_KHR;
    FENCE_CHECK(second->Poll() == Fence::PollResult::kSignaled);
    second.reset();
    FENCE_CHECK(live_syncs == 0 && finishes == 0 && cpu_waits == 0);
  }
}
}  // namespace

extern "C" {
EGLDisplay eglGetCurrentDisplay() { return reinterpret_cast<EGLDisplay>(1); }
EGLContext eglGetCurrentContext() { return context; }
EGLint eglGetError() { return EGL_SUCCESS; }
const char* eglQueryString(EGLDisplay, EGLint name) {
  return name == EGL_EXTENSIONS ? extensions : "1.4";
}
__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char* name) {
  if (!std::strcmp(name, "eglCreateSyncKHR"))
    return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(Create);
  if (!std::strcmp(name, "eglDestroySyncKHR"))
    return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(Destroy);
  if (!std::strcmp(name, "eglClientWaitSyncKHR"))
    return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(
        WaitClient);
  if (!std::strcmp(name, "eglWaitSyncKHR"))
    return reinterpret_cast<__eglMustCastToProperFunctionPointerType>(WaitGPU);
  return nullptr;
}
void glFlush() {}
void glFinish() { ++finishes; }
GLenum glGetError() { return GL_NO_ERROR; }
void glGenTextures(GLsizei count, GLuint* textures) {
  for (int i = 0; i < count; ++i) textures[i] = i + 1;
}
void glDeleteTextures(GLsizei, const GLuint*) {}
}

// The renderer factory is linked through the backing vtable but is outside
// these tests. Fail immediately if any case accidentally enters that boundary.
namespace clay {
SkityGLImageRepresentation::SkityGLImageRepresentation(
    skity::GPUContext*, fml::RefPtr<GLImageRepresentation> repr)
    : SkityImageRepresentation(fml::Ref(repr->GetBacking())) {
  std::abort();
}
SkityGLImageRepresentation::~SkityGLImageRepresentation() = default;
std::shared_ptr<SkityImage> SkityGLImageRepresentation::GetSkityImage() {
  std::abort();
}
bool SkityGLImageRepresentation::EndRead() { std::abort(); }
void SkityGLImageRepresentation::ConsumeFence(std::unique_ptr<FenceSync>) {
  std::abort();
}
std::unique_ptr<FenceSync> SkityGLImageRepresentation::ProduceFence() {
  std::abort();
}
fml::RefPtr<RepresentationStorageManager>
SkityGLImageRepresentation::GetTextureManager() const {
  std::abort();
}
void SkityGLImageRepresentation::SetTextureManager(
    fml::RefPtr<RepresentationStorageManager>) {
  std::abort();
}
}  // namespace clay

int main() {
  alarm(20);  // Reentrant callback regressions must fail rather than hang.
  TestTryAndLegacyUpdates();
  TestCapabilitiesAndFailures();
  for (bool begin_read : {false, true})
    for (bool fail_nested : {false, true})
      for (bool setter_in_wait : {false, true})
        TestCallbackReentry(begin_read, fail_nested, setter_in_wait);
  TestCallbackContextChange(false);
  TestCallbackContextChange(true);
  TestSnapshotLifetime();
  alarm(0);
  std::puts(
      "PASS SurfaceTexture host: try/legacy updates, reentrant wait/delete "
      "callbacks, context changes, frame epochs and snapshot lifetime");
}
