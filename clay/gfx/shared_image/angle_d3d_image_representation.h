// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANGLE_D3D_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_ANGLE_D3D_IMAGE_REPRESENTATION_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <d3d11.h>
#include <d3d9.h>
#include <wrl/client.h>

#include <memory>
#include <optional>

#include "clay/common/graphics/gl/gl_api_windows.h"
#include "clay/gfx/shared_image/shared_image_representation.h"
#include "clay/gfx/shared_image/utils/dxgi_utils.h"
#include "clay/gfx/shared_image/utils/functions_angle.h"

namespace clay {

class D3DTextureImageBacking;
class D3D9TextureImageBacking;

class AngleD3DImageRepresentation : public GLImageRepresentation {
 public:
  AngleD3DImageRepresentation(
      fml::RefPtr<D3DTextureImageBacking> backing,
      PFNEGLGETPROCADDRESSPROC egl_get_proc_address_proc);

  AngleD3DImageRepresentation(
      fml::RefPtr<D3D9TextureImageBacking> backing,
      PFNEGLGETPROCADDRESSPROC egl_get_proc_address_proc);

  ~AngleD3DImageRepresentation() override;

  bool BeginRead(ClaySharedImageReadResult* out) override;
  bool EndRead() override;
  bool BeginWrite(ClaySharedImageWriteResult* out) override;
  bool EndWrite() override;

  ImageRepresentationType GetType() const override;

  void ConsumeFence(std::unique_ptr<FenceSync> fence_sync) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

 private:
  EGLAttrib SetupEGL(PFNEGLGETPROCADDRESSPROC egl_get_proc_address_proc,
                     bool is_d3d11);

  std::optional<TextureInfo> GetTexImage() override;
  bool ReleaseTexImage() override;
  std::optional<FramebufferInfo> BindFrameBuffer() override;
  bool UnbindFrameBuffer() override;

  bool LockKeyedMutex();
  bool UnlockKeyedMutex();

  std::optional<uint32_t> GetTexFromD3D11Texture();
  std::optional<uint32_t> GetTexFromD3D9Texture();

  std::unique_ptr<FunctionsAngle> functions_angle_;
  std::unique_ptr<clay::GlApiWindows> gl_api_windows_;
  EGLDisplay egl_display_ = EGL_NO_DISPLAY;

  // only used in D3D11Texture
  EGLImage egl_image_ = EGL_NO_IMAGE_KHR;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture_;
  Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex_;
  std::optional<ScopedDXGIKeyedMutex> scoped_keyed_mutex_;

  // only used in D3D9Texture
  EGLSurface egl_surface_ = EGL_NO_SURFACE;
  Microsoft::WRL::ComPtr<IDirect3DTexture9> d3d9_texture_;

  uint32_t texture_id_ = 0;
  uint32_t fbo_id_ = 0;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANGLE_D3D_IMAGE_REPRESENTATION_H_
