// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_D3D_IMAGE_REPRESENTATION_H_
#define CLAY_GFX_SHARED_IMAGE_D3D_IMAGE_REPRESENTATION_H_

#include <d3d11.h>
#include <d3d9.h>
#include <wrl/client.h>

#include <memory>

#include "clay/gfx/shared_image/shared_image_representation.h"
#include "clay/gfx/shared_image/utils/dxgi_utils.h"

namespace clay {

class D3DTextureImageBacking;
class D3D9TextureImageBacking;

class D3DImageRepresentation final : public SharedImageRepresentation {
 public:
  D3DImageRepresentation(ID3D11Device* d3d11_device,
                         fml::RefPtr<D3DTextureImageBacking> backing);

  D3DImageRepresentation(IDirect3DDevice9* d3d9_device,
                         fml::RefPtr<D3D9TextureImageBacking> backing);

  ~D3DImageRepresentation() override;

  ImageRepresentationType GetType() const override;

  bool BeginRead(ClaySharedImageReadResult* out) override;
  bool EndRead() override;
  bool BeginWrite(ClaySharedImageWriteResult* out) override;
  bool EndWrite() override;

  void ConsumeFence(std::unique_ptr<FenceSync> fence_sync) override;
  std::unique_ptr<FenceSync> ProduceFence() override;

 private:
  bool LockKeyedMutex();
  bool UnlockKeyedMutex();

  bool FlushD3D9();

  // Only used in D3D11
  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture_;
  Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex_;
  std::optional<ScopedDXGIKeyedMutex> scoped_keyed_mutex_;

  // Only used in D3D9
  Microsoft::WRL::ComPtr<IDirect3DDevice9> device9_;
  Microsoft::WRL::ComPtr<IDirect3DTexture9> d3d9_texture_;
  Microsoft::WRL::ComPtr<IDirect3DQuery9> d3d9_query_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_D3D_IMAGE_REPRESENTATION_H_
