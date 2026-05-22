// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_EGL_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_EGL_IMAGE_REPRESENTATION_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <memory>

#include "clay/gfx/shared_image/fence_sync.h"
#include "clay/gfx/shared_image/shared_image_representation.h"

namespace clay {

class EGLImageBacking;
class AHardwareBufferImageBacking;
class SurfaceTextureImageBacking;

class AndroidEGLFenceSync final : public FenceSync {
 public:
  AndroidEGLFenceSync();

  ~AndroidEGLFenceSync() override;

  int32_t GetSyncFD() { return GetCurrentFD(); }

  bool ClientWait() override;

  Type GetType() const override { return Type::kEGL; }

  void ServerWait();

  static bool CheckEGLFenceSupported();
  static bool CheckAndroidNativeFenceSupported();

 private:
  friend class NativeBufferEGLImageRepresentation;
  friend class VulkanFenceHelper;
  int32_t GetCurrentFD();

  EGLSyncKHR fence_ = EGL_NO_SYNC_KHR;

  static EGLDisplay display;
  static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR;
  static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR;
  static PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR;
  static PFNEGLWAITSYNCKHRPROC eglWaitSyncKHR;
  static PFNEGLDUPNATIVEFENCEFDANDROIDPROC eglDupNativeFenceFDANDROID;
};

class NativeBufferEGLImageRepresentation final : public GLImageRepresentation {
 public:
  explicit NativeBufferEGLImageRepresentation(
      fml::RefPtr<AHardwareBufferImageBacking> backing);
  explicit NativeBufferEGLImageRepresentation(
      fml::RefPtr<EGLImageBacking> backing);

  ~NativeBufferEGLImageRepresentation() override;

  ImageRepresentationType GetType() const override;
  void ConsumeFence(std::unique_ptr<FenceSync>) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

 private:
  std::optional<TextureInfo> GetTexImage() override;
  bool ReleaseTexImage() override;
  std::optional<FramebufferInfo> BindFrameBuffer() override;
  bool UnbindFrameBuffer() override;

 private:
  uint32_t texture_id_ = 0;
  uint32_t fbo_id_ = 0;
  uint32_t depth_stencil_id_ = 0;

  // For AHardwareBufferImageBacking.
  EGLImageKHR egl_image_ = nullptr;
  EGLDisplay egl_display_ = nullptr;
};

class SurfaceTextureEGLImageRepresentation final
    : public GLImageRepresentation {
 public:
  explicit SurfaceTextureEGLImageRepresentation(
      fml::RefPtr<SurfaceTextureImageBacking> backing);

  ~SurfaceTextureEGLImageRepresentation() override;

  ImageRepresentationType GetType() const override;
  void ConsumeFence(std::unique_ptr<FenceSync>) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

 private:
  std::optional<TextureInfo> GetTexImage() override;
  bool ReleaseTexImage() override;
  std::optional<FramebufferInfo> BindFrameBuffer() override;
  bool UnbindFrameBuffer() override;

  fml::RefPtr<SurfaceTextureImageBacking> backing_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_EGL_IMAGE_REPRESENTATION_H_
