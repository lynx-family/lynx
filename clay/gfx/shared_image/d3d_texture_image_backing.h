// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_D3D_TEXTURE_IMAGE_BACKING_H_
#define CLAY_GFX_SHARED_IMAGE_D3D_TEXTURE_IMAGE_BACKING_H_

#include <d3d11.h>
#include <wrl/client.h>

#include <memory>
#include <mutex>
#include <optional>

#include "clay/gfx/shared_image/shared_image_backing.h"

namespace clay {

class SharedImageRepresentation;

class D3DTextureImageBacking final : public SharedImageBacking {
 public:
  D3DTextureImageBacking(PixelFormat pixel_format, skity::Vec2 size,
                         std::optional<GraphicsMemoryHandle> gfx_handle = {});

  ~D3DTextureImageBacking() override;

  bool HasKeyedMutex() const {
    return (d3d11_texture_desc_.MiscFlags &
            D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX) != 0;
  }

  bool OpenForDevice(ID3D11Device* device, ID3D11Texture2D** out_texture,
                     IDXGIKeyedMutex** out_keyed_mutex) const;

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

  static bool IsSupported();

 private:
  HANDLE shared_handle_ = nullptr;
  bool owned_nt_handle_ = false;

  ID3D11Texture2D* GetOrCreateStagingTexture(ID3D11Device* device);
  IDXGIKeyedMutex* GetOrCreateKeyedMutex();

  // The texture and device should never be used outside the class.
  // User MUST use OpenSharedResource on its own D3D11Device
  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture_;
  D3D11_TEXTURE2D_DESC d3d11_texture_desc_;
  // Staging texture used for copy to/from shared memory GMB.
  Microsoft::WRL::ComPtr<ID3D11Texture2D> staging_texture_;
  // Keyed mutex to for usage of staging texture read/write
  Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_D3D_TEXTURE_IMAGE_BACKING_H_
