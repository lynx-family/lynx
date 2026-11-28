// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android_surface_texture_image_backing.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <utility>

#include "base/include/auto_reset.h"
#include "base/include/platform/android/jni_convert_helper.h"
#include "clay/common/graphics/gl/scoped_texture_binder.h"
#include "clay/fml/logging.h"
#include "clay/gfx/gfx_rendering_backend.h"
#include "clay/gfx/shared_image/android/surface_texture.h"
#include "clay/gfx/shared_image/android_egl_image_representation.h"

namespace clay {

// The fence needs shared ownership, but no weak references or polymorphic
// control block. Keep its reference count alongside the concrete EGL fence.
class SurfaceTextureFence final
    : public fml::RefCountedThreadSafe<SurfaceTextureFence> {
 public:
  explicit SurfaceTextureFence(bool allow_blocking)
      : fence(allow_blocking
                  ? AndroidEGLFenceSync::CreationMode::kAllowBlockingFallback
                  : AndroidEGLFenceSync::CreationMode::kNonblocking) {}
  AndroidEGLFenceSync fence;
};

namespace {
// Temporarily release an already-held mutex around external callbacks.
class ScopedUnlock {
 public:
  explicit ScopedUnlock(std::mutex& mutex) : mutex_(mutex) { mutex_.unlock(); }
  ~ScopedUnlock() { mutex_.lock(); }

 private:
  std::mutex& mutex_;
};

// Independent handles share the EGL sync lifetime; every FD export still
// duplicates the descriptor. Destroying a snapshot never waits for the GPU.
class FenceSnapshot final : public FenceSync {
 public:
  explicit FenceSnapshot(fml::RefPtr<SurfaceTextureFence> fence)
      : fence_(std::move(fence)) {}
  bool ClientWait() override { return fence_->fence.ClientWait(); }
  Type GetType() const override { return Type::kEGL; }
  WaitResult EnqueueWaitResult() override {
    return fence_->fence.EnqueueWaitResult();
  }
  PollResult Poll() override { return fence_->fence.Poll(); }
  NativeExportResult DupNativeFenceFD(int* fd) override {
    return fence_->fence.DupNativeFenceFD(fd);
  }

 private:
  fml::RefPtr<SurfaceTextureFence> fence_;
};

class FailedFence final : public FenceSync {
 public:
  bool ClientWait() override { return false; }
  Type GetType() const override { return Type::kClientWaitOnly; }
  PollResult Poll() override { return PollResult::kFailed; }
  WaitResult EnqueueWaitResult() override { return WaitResult::kFailed; }
  NativeExportResult DupNativeFenceFD(int* fd) override {
    if (fd) *fd = -1;
    return NativeExportResult::kFailed;
  }
};
}  // namespace

SurfaceTextureImageBacking::SurfaceTextureImageBacking(
    PixelFormat pixel_format, skity::Vec2 size,
    std::optional<GraphicsMemoryHandle> gfx_handle)
    : SharedImageBackingUnmanaged(pixel_format, size),
      surface_texture_(
          gfx_handle.has_value()
              ? SurfaceTexture::Retain(
                    reinterpret_cast<jobject>(gfx_handle.value()), false)
              : SurfaceTexture::Create()),
      uses_internal_frame_listener_(!gfx_handle.has_value()),
      frame_callback_state_(std::make_shared<FrameCallbackState>()) {
  if (uses_internal_frame_listener_) {
    // The first SurfaceTexture frame may arrive before the drawable image
    // callback is registered. Cache that signal and replay it on registration.
    RestoreFrameListener();
  }
}

SurfaceTextureImageBacking::~SurfaceTextureImageBacking() {
  if (uses_internal_frame_listener_) {
    auto state = std::move(frame_callback_state_);
    if (state) {
      std::lock_guard<std::mutex> l(state->mutex);
      state->is_valid = false;
      state->callback = nullptr;
      state->has_pending_frame = false;
    }
    surface_texture_->SetFrameAvailableCallback(nullptr);
  }
  if (texture_ != 0) {
    FML_LOG(ERROR) << "Texture not detached, maybe leaked!";
  }
}

const skity::Vec2 SurfaceTextureImageBacking::GetSize() const {
  if (size_.x <= 0 || size_.y <= 0) {
    // If the size is not set, we assume the default size is 256x256.
    return skity::Vec2(256, 256);
  }
  return size_;
}

SharedImageBacking::BackingType SurfaceTextureImageBacking::GetType() const {
  return SharedImageBacking::BackingType::kSurfaceTexture;
}

GraphicsMemoryHandle SurfaceTextureImageBacking::GetGFXHandle() const {
  return surface_texture_->j_surface_texture().Get();
}

fml::RefPtr<SharedImageRepresentation>
SurfaceTextureImageBacking::CreateRepresentation(
    const ClaySharedImageRepresentationConfig* config) {
  auto type = config->type;
  switch (type) {
    case kClaySharedImageRepresentationTypeGL: {
      return fml::MakeRefCounted<SurfaceTextureEGLImageRepresentation>(
          fml::Ref(this));
    }
    default: {
      // NOT SUPPORTED.
      FML_LOG(ERROR)
          << "Unable to call AHardwareBufferImageBacking::CreateRepresentation "
             "with type: "
          << static_cast<uint32_t>(type);
      return nullptr;
    }
  }
}

#ifndef ENABLE_SKITY
fml::RefPtr<SkiaImageRepresentation>
SurfaceTextureImageBacking::CreateSkiaRepresentation(
    GrDirectContext* gr_context) {
  switch (gr_context->backend()) {
    case GrBackendApi::kOpenGL: {
      return fml::MakeRefCounted<SkiaGLImageRepresentation>(
          gr_context, fml::MakeRefCounted<SurfaceTextureEGLImageRepresentation>(
                          fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR)
          << "Unable to call "
             "SurfaceTextureImageBacking::CreateSkiaRepresentation with "
             "backend: "
          << static_cast<uint32_t>(gr_context->backend());
      return nullptr;
    }
  }
}
#else
fml::RefPtr<SkityImageRepresentation>
SurfaceTextureImageBacking::CreateSkityRepresentation(
    skity::GPUContext* skity_context) {
  switch (skity_context->GetBackendType()) {
    case skity::GPUBackendType::kOpenGL: {
      return fml::MakeRefCounted<SkityGLImageRepresentation>(
          skity_context,
          fml::MakeRefCounted<SurfaceTextureEGLImageRepresentation>(
              fml::Ref(this)));
    }
    default: {
      FML_LOG(ERROR)
          << "Unable to call "
             "SurfaceTextureImageBacking::CreateSkityRepresentation with "
             "backend: "
          << static_cast<uint32_t>(skity_context->GetBackendType());
      return nullptr;
    }
  }
}
#endif  // ENABLE_SKITY

// This is a workaround to address the limitation that the surface texture
// provides an API to `setDefaultBufferSize` without offering a method to
// `getBufferSize`.
// Utilizing `glGetTexLevelParameterfv` is also not reliable
// on certain devices, as discussed here: https://stackoverflow.com/q/53536401.
// So the SurfaceTextureImageBacking owner should set the size during layout (as
// with `VideoPlayerAndroid` or `PlatformViewPlugin`).
bool SurfaceTextureImageBacking::SetSize(skity::Vec2 size) {
  size_ = size;
  return true;
}

const skity::Matrix SurfaceTextureImageBacking::GetTransformation() const {
  float mat[16];
  surface_texture_->GetTransformMatrix(mat);
  return skity::Matrix{mat[0],  mat[1],  mat[2],  mat[3],   //
                       mat[4],  mat[5],  mat[6],  mat[7],   //
                       mat[8],  mat[9],  mat[10], mat[11],  //
                       mat[12], mat[13], mat[14], mat[15]};
}

void SurfaceTextureImageBacking::SetTransformation(const skity::Matrix& mat) {
  FML_UNIMPLEMENTED();
}

/// `SharedImageBackingUnmanaged`
void SurfaceTextureImageBacking::SetFrameAvailableCallback(
    const fml::closure& callback) {
  if (!uses_internal_frame_listener_) {
    surface_texture_->SetFrameAvailableCallback(callback);
    return;
  }

  fml::closure pending_callback;
  {
    std::lock_guard<std::mutex> l(frame_callback_state_->mutex);
    if (!frame_callback_state_->is_valid) {
      return;
    }
    frame_callback_state_->callback = callback;
    if (frame_callback_state_->callback &&
        frame_callback_state_->has_pending_frame) {
      pending_callback = frame_callback_state_->callback;
      frame_callback_state_->has_pending_frame = false;
    }
  }
  if (pending_callback) {
    pending_callback();
  }
}

bool SurfaceTextureImageBacking::UpdateFront() {
  return UpdateFrontInternal(true) == UpdateResult::kSuccess;
}

bool SurfaceTextureImageBacking::IsOwnerContext() const {
  return owner_context_ != EGL_NO_CONTEXT &&
         owner_context_ == eglGetCurrentContext() &&
         owner_display_ == eglGetCurrentDisplay();
}

bool SurfaceTextureImageBacking::AcceptFence(std::unique_ptr<FenceSync> fence) {
  if (!fence) return true;
  std::lock_guard<std::mutex> lock(dependency_mutex_);
  dependencies_.push_back(std::move(fence));
  return true;
}

SurfaceTextureImageBacking::UpdateResult
SurfaceTextureImageBacking::OrderDependencies(bool allow_client_wait) {
  if (sync_status_ != UpdateResult::kSuccess) return sync_status_;
  if (ordering_dependencies_) return UpdateResult::kPending;
  // Keep nested access blocked while callbacks run with the inbox unlocked.
  lynx::base::AutoReset<bool> ordering(&ordering_dependencies_, true);
  if (!dependency_mutex_.try_lock()) return UpdateResult::kPending;
  std::lock_guard<std::mutex> lock(dependency_mutex_, std::adopt_lock);
  for (;;) {
    // Callbacks may use the public setter as well as the dependency inbox.
    // Revisit both before allowing the enclosing read or update to proceed.
    if (auto fence = GetFenceSync()) {
      dependencies_.push_back(std::move(fence));
    }
    if (dependencies_.empty()) return UpdateResult::kSuccess;
    auto& fence = dependencies_.back();
    const auto status = fence->Poll();
    if (status == FenceSync::PollResult::kFailed) {
      return sync_status_ = UpdateResult::kFailed;
    }
    const auto wait = status == FenceSync::PollResult::kSignaled
                          ? FenceSync::WaitResult::kSuccess
                          : fence->EnqueueWaitResult();
    if (wait == FenceSync::WaitResult::kFailed) {
      return sync_status_ = UpdateResult::kFailed;
    }
    if (wait != FenceSync::WaitResult::kSuccess) {
      if (allow_client_wait && status == FenceSync::PollResult::kUnsupported &&
          wait == FenceSync::WaitResult::kUnsupported) {
        // Legacy external fences expose only a CPU callback. Do not hold the
        // inbox mutex while invoking user code: it may enqueue dependencies.
        auto legacy_fence = std::move(fence);
        dependencies_.pop_back();
        bool completed;
        {
          ScopedUnlock unlock(dependency_mutex_);
          completed = legacy_fence->ClientWait();
        }
        if (!completed || !IsOwnerContext()) {
          dependencies_.push_back(std::move(legacy_fence));
          return sync_status_ = UpdateResult::kFailed;
        }
        // The delete callback is user code too.
        {
          ScopedUnlock unlock(dependency_mutex_);
          legacy_fence.reset();
        }
        if (!IsOwnerContext()) return sync_status_ = UpdateResult::kFailed;
        if (sync_status_ != UpdateResult::kSuccess) return sync_status_;
        continue;
      }
      // Keep the dependency for a later attempt. In particular, do not invoke
      // client-wait callbacks from the nonblocking entry point.
      return status == FenceSync::PollResult::kPending
                 ? UpdateResult::kPending
                 : UpdateResult::kUnsupported;
    }
    dependencies_.pop_back();
  }
}

fml::RefPtr<SurfaceTextureFence> SurfaceTextureImageBacking::CreateFence(
    bool allow_blocking) {
  auto fence = fml::MakeRefCounted<SurfaceTextureFence>(allow_blocking);
  const auto status = fence->fence.Poll();
  if (status == FenceSync::PollResult::kFailed) {
    sync_status_ = UpdateResult::kFailed;
  }
  return fence;
}

SurfaceTextureImageBacking::UpdateResult
SurfaceTextureImageBacking::TryUpdateFront() {
  return UpdateFrontInternal(false);
}

SurfaceTextureImageBacking::UpdateResult
SurfaceTextureImageBacking::UpdateFrontInternal(bool allow_client_wait) {
  if (reading_ || ordering_dependencies_) return UpdateResult::kPending;
  if (sync_status_ != UpdateResult::kSuccess) return sync_status_;
  // Check before attaching or latching: a missing completion capability must
  // not turn a try-update into glFinish, and legacy UpdateFront may retry.
  if (!allow_client_wait && eglGetCurrentContext() != EGL_NO_CONTEXT &&
      eglGetCurrentDisplay() != EGL_NO_DISPLAY &&
      !AndroidEGLFenceSync::CheckEGLFenceSupported()) {
    return UpdateResult::kUnsupported;
  }
  if (EnsureAttachedToGLContext(allow_client_wait) == 0) {
    FML_LOG(ERROR) << "Failed to attach to GL context";
    return UpdateResult::kFailed;
  }
  auto status = OrderDependencies(allow_client_wait);
  if (status != UpdateResult::kSuccess) return status;
  uint64_t available = 0;
  if (uses_internal_frame_listener_) {
    if (!frame_callback_state_->mutex.try_lock()) return UpdateResult::kPending;
    std::lock_guard<std::mutex> lock(frame_callback_state_->mutex,
                                     std::adopt_lock);
    available = frame_callback_state_->available_sequence;
    if (available == consumed_sequence_) {
      return has_frame_ ? UpdateResult::kSuccess : UpdateResult::kPending;
    }
  }
  // https://developer.android.com/reference/android/graphics/SurfaceTexture#updateTexImage()
  // In Android docs, updateTexImage will implicitly bind
  // GL_TEXTURE_EXTERNAL_OES. To make Skia happy, we must reset the state.
  clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_EXTERNAL_OES,
                                                  texture_);
  if (!surface_texture_->UpdateTexImage()) {
    has_frame_ = false;
    return sync_status_ = UpdateResult::kFailed;
  }
  // This is completion of the owner's update command stream, not the original
  // BufferQueue producer FD, and does not grant cross-context OES access.
  ready_fence_ = CreateFence(allow_client_wait);
  if (sync_status_ != UpdateResult::kSuccess) return sync_status_;
  has_frame_ = true;
  consumed_sequence_ = available;
  read_fence_ = nullptr;
  return UpdateResult::kSuccess;
}

bool SurfaceTextureImageBacking::BeginRead() {
  if (!IsOwnerContext() || !has_frame_ ||
      OrderDependencies(true) != UpdateResult::kSuccess) {
    return false;
  }
  if (!reading_) {
    reading_ = true;
    read_fence_ = nullptr;
  }
  return true;
}

bool SurfaceTextureImageBacking::EndRead() {
  if (!IsOwnerContext()) return false;
  if (reading_) {
    reading_ = false;
    read_fence_ = CreateFence(true);
  }
  return sync_status_ == UpdateResult::kSuccess;
}

std::unique_ptr<FenceSync> SurfaceTextureImageBacking::ProduceReadFence() {
  fml::RefPtr<SurfaceTextureFence> fence;
  if (sync_status_ == UpdateResult::kSuccess && IsOwnerContext()) {
    // Direct API callers may request a fence before EndRead. Snapshot the work
    // submitted so far, without closing the read epoch.
    fence = reading_ ? CreateFence(true)
                     : (read_fence_ ? read_fence_ : ready_fence_);
  }
  if (!fence) return std::make_unique<FailedFence>();
  return std::make_unique<FenceSnapshot>(std::move(fence));
}

std::unique_ptr<FenceSync> SurfaceTextureImageBacking::GetFenceSyncForClient() {
  // Preserve the existing public setter/getter ownership transfer. Otherwise
  // expose our completion proof, never a null (implicitly signaled) fence.
  if (auto fence = GetFenceSync()) return fence;
  return ProduceReadFence();
}

void SurfaceTextureImageBacking::ReleaseFront() {
  // Multi-buffer SurfaceTexture releases its previous image on updateTexImage.
  // Single-buffer releaseTexImage is not supported by this backing.
}

uint32_t SurfaceTextureImageBacking::AcquireBack() {
  // No need to call any functions, since eglMakeCurrent automatically requires
  // a new buffer.
  return 0;  // TODO(youfeng) support buffer age for surface texture.
}

bool SurfaceTextureImageBacking::SwapBack() {
  // No need to call any functions, since eglSwap automatically swap back
  return true;
}

uint32_t SurfaceTextureImageBacking::Capacity() const {
  // Currently only support double buffering
  return 2;
}

const fml::jni::JavaRef<jobject>&
SurfaceTextureImageBacking::GetSurfaceTexture() const {
  return surface_texture_->j_surface_texture();
}

GLuint SurfaceTextureImageBacking::EnsureAttachedToGLContext() {
  return EnsureAttachedToGLContext(true);
}

GLuint SurfaceTextureImageBacking::EnsureAttachedToGLContext(
    bool allow_blocking) {
  if (eglGetCurrentContext() == EGL_NO_CONTEXT ||
      eglGetCurrentDisplay() == EGL_NO_DISPLAY)
    return 0;
  if (texture_ != 0 && !IsOwnerContext()) return 0;
  if (texture_ == 0) {
    glGenTextures(1, &texture_);
    clay::ScopedTextureBinder scoped_texture_binder(GL_TEXTURE_EXTERNAL_OES,
                                                    texture_);
    if (!texture_ || !surface_texture_->AttachToGLContext(texture_)) {
      glDeleteTextures(1, &texture_);
      texture_ = 0;
      return 0;
    }
    owner_context_ = eglGetCurrentContext();
    owner_display_ = eglGetCurrentDisplay();
    if (has_frame_ && sync_status_ == UpdateResult::kSuccess) {
      ready_fence_ = CreateFence(allow_blocking);
    }
  }
  return texture_;
}

void SurfaceTextureImageBacking::DetachGLContext() {
  if (texture_ != 0) {
    if (!IsOwnerContext()) {
      FML_LOG(ERROR) << "SurfaceTexture detach requires its owning EGL context";
      return;
    }
    EndRead();
    if (!surface_texture_->DetachFromGLContext()) return;
    // Android detach deletes the texture name.
    texture_ = 0;
    owner_context_ = EGL_NO_CONTEXT;
    owner_display_ = EGL_NO_DISPLAY;
  }
}

fml::closure SurfaceTextureImageBacking::TakeFrameNotification() {
  if (!uses_internal_frame_listener_) return {};
  auto notify = NewFrameNotifier();
  surface_texture_->SetFrameAvailableCallback(nullptr);
  return notify;
}

void SurfaceTextureImageBacking::RestoreFrameListener() {
  surface_texture_->SetFrameAvailableCallback(NewFrameNotifier());
}

fml::closure SurfaceTextureImageBacking::NewFrameNotifier() {
  std::weak_ptr<FrameCallbackState> weak_state = frame_callback_state_;
  uint64_t generation;
  {
    std::lock_guard<std::mutex> lock(frame_callback_state_->mutex);
    generation = ++frame_callback_state_->source_generation;
  }
  return [weak_state, generation] {
    auto state = weak_state.lock();
    if (!state) return;
    fml::closure callback;
    {
      std::lock_guard<std::mutex> l(state->mutex);
      if (!state->is_valid || generation != state->source_generation) {
        return;
      }
      ++state->available_sequence;
      if (!state->callback) {
        state->has_pending_frame = true;
        return;
      }
      callback = state->callback;
    }
    callback();
  };
}

}  // namespace clay
