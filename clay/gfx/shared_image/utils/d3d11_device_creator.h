// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_UTILS_D3D11_DEVICE_CREATOR_H_
#define CLAY_GFX_SHARED_IMAGE_UTILS_D3D11_DEVICE_CREATOR_H_

#include <d3d11.h>
#include <dxgi.h>
#include <wrl.h>

#include <optional>

namespace clay {

#if USE_DISCRETE_GPU
HRESULT GetHighPerformanceAdapter(IDXGIAdapter1** adapter);
#endif

HRESULT CreateSafeD3D11Device(
    std::optional<PFN_D3D11_CREATE_DEVICE> device_creator,
    ID3D11Device** out_device, ID3D11DeviceContext** out_context);

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_UTILS_D3D11_DEVICE_CREATOR_H_
