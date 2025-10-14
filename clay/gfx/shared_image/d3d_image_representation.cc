// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/d3d_image_representation.h"

#include <Windows.h>
#include <d3d11_1.h>

#include <cstring>

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/d3d9_texture_image_backing.h"
#include "clay/gfx/shared_image/d3d_texture_image_backing.h"
#include "clay/gfx/shared_image/fence_sync.h"

namespace clay {

D3DImageRepresentation::D3DImageRepresentation(
    ID3D11Device* d3d11_device, fml::RefPtr<D3DTextureImageBacking> backing)
    : SharedImageRepresentation(backing) {
  if (!d3d11_device) {
    FML_LOG(ERROR) << "Has no d3d11 device.";
    return;
  }
  device_ = d3d11_device;

  if (!backing->OpenForDevice(d3d11_device, &d3d11_texture_, &keyed_mutex_)) {
    FML_LOG(ERROR) << "Failed to open for device.";
  }
}

D3DImageRepresentation::D3DImageRepresentation(
    IDirect3DDevice9* d3d9_device, fml::RefPtr<D3D9TextureImageBacking> backing)
    : SharedImageRepresentation(backing) {
  if (!d3d9_device) {
    FML_LOG(ERROR) << "Has no d3d9 device.";
    return;
  }
  device9_ = d3d9_device;

  if (!backing->OpenForDevice(d3d9_device, &d3d9_texture_)) {
    FML_LOG(ERROR) << "Failed to open for device.";
  }
}

D3DImageRepresentation::~D3DImageRepresentation() {
  if (scoped_keyed_mutex_) {
    FML_LOG(ERROR) << "Keyed mutex is still locked.";
    scoped_keyed_mutex_.reset();
  }
  keyed_mutex_.Reset();
  d3d11_texture_.Reset();
  device_.Reset();

  d3d9_query_.Reset();
  d3d9_texture_.Reset();
  device9_.Reset();
}

ImageRepresentationType D3DImageRepresentation::GetType() const {
  return ImageRepresentationType::kD3D;
}

bool D3DImageRepresentation::BeginRead(ClaySharedImageReadResult* out) {
  if (!d3d11_texture_ && !d3d9_texture_) {
    return false;
  }
  memset(out, 0, sizeof(ClaySharedImageReadResult));
  out->struct_size = sizeof(ClaySharedImageReadResult);
  out->type = kClaySharedImageRepresentationTypeD3D;
  out->d3d_texture.struct_size = sizeof(ClayD3DTexture);
  out->d3d_texture.texture =
      d3d11_texture_ ? static_cast<ClayD3DTextureHandle>(d3d11_texture_.Get())
                     : static_cast<ClayD3DTextureHandle>(d3d9_texture_.Get());
  out->d3d_texture.user_data = this;
  this->AddRef();
  out->d3d_texture.destruction_callback = [](void* user_data) {
    static_cast<D3DImageRepresentation*>(user_data)->Release();
  };

  return LockKeyedMutex();
}

bool D3DImageRepresentation::EndRead() { return UnlockKeyedMutex(); }

bool D3DImageRepresentation::BeginWrite(ClaySharedImageWriteResult* out) {
  if (!d3d11_texture_ && !d3d9_texture_) {
    return false;
  }
  memset(out, 0, sizeof(ClaySharedImageWriteResult));
  out->struct_size = sizeof(ClaySharedImageWriteResult);
  out->type = kClaySharedImageRepresentationTypeD3D;
  out->d3d_texture.struct_size = sizeof(ClayD3DTexture);
  out->d3d_texture.texture =
      d3d11_texture_ ? static_cast<ClayD3DTextureHandle>(d3d11_texture_.Get())
                     : static_cast<ClayD3DTextureHandle>(d3d9_texture_.Get());
  out->d3d_texture.user_data = this;
  this->AddRef();
  out->d3d_texture.destruction_callback = [](void* user_data) {
    static_cast<D3DImageRepresentation*>(user_data)->Release();
  };

  return LockKeyedMutex();
}

bool D3DImageRepresentation::EndWrite() { return UnlockKeyedMutex(); }

bool D3DImageRepresentation::LockKeyedMutex() {
  if (!keyed_mutex_) {
    return true;
  }
  if (scoped_keyed_mutex_) {
    FML_LOG(ERROR) << "Keyed mutex is already locked.";
  }
  scoped_keyed_mutex_.emplace(keyed_mutex_.Get());
  return true;
}

bool D3DImageRepresentation::UnlockKeyedMutex() {
  if (!keyed_mutex_) {
    return true;
  }
  if (!scoped_keyed_mutex_) {
    FML_LOG(ERROR) << "Keyed mutex is not locked.";
  }
  scoped_keyed_mutex_.reset();
  return true;
}

class DummyFenceSync final : public FenceSync {
  bool ClientWait() override { return true; }

  Type GetType() const override { return Type::kClientWaitOnly; }
};

void D3DImageRepresentation::ConsumeFence(
    std::unique_ptr<FenceSync> fence_sync) {
  if (!fence_sync) {
    return;
  }
  fence_sync->ClientWait();
}

bool D3DImageRepresentation::FlushD3D9() {
  FML_DCHECK(device9_);
  HRESULT hr;
  if (!d3d9_query_) {
    hr = device9_->CreateQuery(D3DQUERYTYPE_EVENT, &d3d9_query_);
    if (FAILED(hr)) {
      FML_LOG(ERROR) << "Failed to create query.";
      return false;
    }
  }
  hr = d3d9_query_->Issue(D3DISSUE_END);
  if (FAILED(hr)) {
    FML_LOG(ERROR) << "Failed to issue D3DISSUE_END.";
    return false;
  }
  hr = d3d9_query_->GetData(nullptr, 0, D3DGETDATA_FLUSH);
  if (FAILED(hr)) {
    FML_LOG(ERROR) << "Failed to get event D3DGETDATA_FLUSH.";
    return false;
  }
  return true;
}

std::unique_ptr<FenceSync> D3DImageRepresentation::ProduceFence() {
  // We use keyed mutex when BeginRW/EndRW instead of fence
  if (device_) {
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
    device_->GetImmediateContext(&device_context);
    device_context->Flush();
  } else if (device9_) {
    FlushD3D9();
  }

  return std::make_unique<DummyFenceSync>();
}

}  // namespace clay
