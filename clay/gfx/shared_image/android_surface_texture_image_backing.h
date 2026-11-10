// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_IMAGE_BACKING_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES/gl.h>

#include <memory>
#include <mutex>
#include <vector>

#include "base/include/platform/android/scoped_java_ref.h"
#include "clay/gfx/shared_image/shared_image_backing.h"
#include "clay/public/clay.h"

namespace clay {

class SurfaceTexture;
class SurfaceTextureFence;
class SharedImageRepresentation;

class SurfaceTextureImageBacking final : public SharedImageBackingUnmanaged {
 public:
  SurfaceTextureImageBacking(
      PixelFormat pixel_format, skity::Vec2 size,
      std::optional<GraphicsMemoryHandle> gfx_handle = {});
  ~SurfaceTextureImageBacking() override;

  const skity::Vec2 GetSize() const override;
  BackingType GetType() const override;
  GraphicsMemoryHandle GetGFXHandle() const override;
  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig* config) override;
#ifndef ENABLE_SKITY
  fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrDirectContext* gr_context) override;
#else
  fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      skity::GPUContext* skity_context) override;
#endif  // ENABLE_SKITY
  const skity::Matrix GetTransformation() const override;
  void SetTransformation(const skity::Matrix& mat) override;

  /// `SharedImageBackingUnmanaged`
  void SetFrameAvailableCallback(const fml::closure& callback) override;
  bool UpdateFront() override;
  void ReleaseFront() override;
  uint32_t AcquireBack() override;
  bool SwapBack() override;
  uint32_t Capacity() const override;

  bool SetSize(skity::Vec2 size) override;
  const fml::jni::JavaRef<jobject>& GetSurfaceTexture() const;
  GLuint EnsureAttachedToGLContext();
  void DetachGLContext();
  bool RequiresReadCompletionBeforeUpdate() const override { return true; }
  bool AcceptFence(std::unique_ptr<FenceSync> fence) override;

  enum class UpdateResult { kSuccess, kPending, kUnsupported, kFailed };
  // This entry point never CPU-waits on explicit dependencies or falls back
  // to glFinish for completion fences. Android's updateTexImage itself may
  // wait inside the platform BufferQueue.
  // Legacy UpdateFront/BeginRead may wait on callback-only dependencies.
  UpdateResult TryUpdateFront();
  bool BeginRead();
  bool EndRead();
  std::unique_ptr<FenceSync> ProduceReadFence();
  std::unique_ptr<FenceSync> GetFenceSyncForClient() override;

 private:
  UpdateResult UpdateFrontInternal(bool allow_client_wait);
  UpdateResult OrderDependencies(bool allow_client_wait);
  bool IsOwnerContext() const;
  GLuint EnsureAttachedToGLContext(bool allow_blocking);
  fml::RefPtr<SurfaceTextureFence> CreateFence(bool allow_blocking);
  struct FrameCallbackState {
    std::mutex mutex;
    fml::closure callback;
    bool has_pending_frame = false;
    bool is_valid = true;
    uint64_t available_sequence = 0;
  };

  static void TriggerFrameCallback(
      const std::shared_ptr<FrameCallbackState>& state);

  fml::RefPtr<SurfaceTexture> surface_texture_;
  GLuint texture_ = 0;
  bool uses_internal_frame_listener_ = false;
  std::shared_ptr<FrameCallbackState> frame_callback_state_;
  // Only the dependency inbox and frame callbacks may run off the GL owner.
  std::mutex dependency_mutex_;
  std::vector<std::unique_ptr<FenceSync>> dependencies_;
  EGLDisplay owner_display_ = EGL_NO_DISPLAY;
  EGLContext owner_context_ = EGL_NO_CONTEXT;
  uint64_t consumed_sequence_ = 0;
  bool has_frame_ = false;
  bool reading_ = false;
  bool ordering_dependencies_ = false;
  UpdateResult sync_status_ = UpdateResult::kSuccess;
  fml::RefPtr<SurfaceTextureFence> ready_fence_;
  fml::RefPtr<SurfaceTextureFence> read_fence_;
};

}  // namespace clay
   //
#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_SURFACE_TEXTURE_IMAGE_BACKING_H_
