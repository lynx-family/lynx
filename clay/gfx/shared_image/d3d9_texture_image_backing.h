// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_D3D9_TEXTURE_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_D3D9_TEXTURE_IMAGE_BACKING_H_

#include <d3d9.h>
#include <wrl/client.h>

#include <memory>
#include <optional>

#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

class SharedImageRepresentation;

class D3D9TextureImageBacking final : public SharedImageBacking {
 public:
  D3D9TextureImageBacking(PixelFormat pixel_format, skity::Vec2 size,
                          std::optional<GraphicsMemoryHandle> gfx_handle = {});

  ~D3D9TextureImageBacking() override;

  bool OpenForDevice(IDirect3DDevice9* device,
                     IDirect3DTexture9** out_texture) const;

  BackingType GetType() const override;
  GraphicsMemoryHandle GetGFXHandle() const override;

  fml::RefPtr<SharedImageRepresentation> CreateRepresentation(
      const ClaySharedImageRepresentationConfig* config) override;
#ifdef ENABLE_SKITY
  virtual fml::RefPtr<SkityImageRepresentation> CreateSkityRepresentation(
      skity::GPUContext* skity_context) override;
#else
  bool ReadbackToMemory(const SkPixmap* pixmaps, uint32_t planes) override;
  virtual fml::RefPtr<SkiaImageRepresentation> CreateSkiaRepresentation(
      GrDirectContext* gr_context) override;
#endif  // ENABLE_SKITY

 private:
  HANDLE shared_handle_ = nullptr;

  IDirect3DTexture9* GetOrCreateStagingTexture();

  D3DFORMAT d3d_format_;

  // The texture and device should never be used outside the class.
  // User MUST use OpenSharedResource on its own D3D9Device
  Microsoft::WRL::ComPtr<IDirect3DDevice9> d3d9_device_;
  Microsoft::WRL::ComPtr<IDirect3DTexture9> d3d9_texture_;
  Microsoft::WRL::ComPtr<IDirect3DTexture9> staging_texture_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_D3D9_TEXTURE_IMAGE_BACKING_H_
