// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "clay/gfx/shared_image/utils/d3d11_device_creator.h"

#include "clay/fml/logging.h"

namespace clay {

#if USE_DISCRETE_GPU
HRESULT GetHighPerformanceAdapter(IDXGIAdapter1** out_adapter) {
  constexpr UINT PCI_VENDOR_INTEL = 0x8086;
  constexpr UINT PCI_VENDOR_NVIDIA = 0x10DE;
  constexpr UINT PCI_VENDOR_AMD = 0x1002;

  *out_adapter = nullptr;

  Microsoft::WRL::ComPtr<IDXGIFactory1> factory;
  HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), &factory);
  if (FAILED(hr)) {
    return hr;
  }

  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
  Microsoft::WRL::ComPtr<IDXGIAdapter1> best_adapter;
  UINT index = 0;

  while (factory->EnumAdapters1(index++, &adapter) != DXGI_ERROR_NOT_FOUND) {
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);

    // Skip software renderer.
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      continue;
    }

    // Prioritize discrete GPUs (NVIDIA / AMD)
    if (desc.VendorId == PCI_VENDOR_NVIDIA || desc.VendorId == PCI_VENDOR_AMD) {
      best_adapter = adapter;
      break;
    }

    // Fallback to Intel iGPU if no discrete GPU is found
    if (desc.VendorId == PCI_VENDOR_INTEL && !best_adapter) {
      best_adapter = adapter;
    }
  }

  if (!best_adapter) {
    FML_LOG(WARNING) << "Could not find discrete GPU.";
    return E_FAIL;
  }

  *out_adapter = best_adapter.Detach();
  return S_OK;
}
#endif

HRESULT CreateSafeD3D11Device(
    std::optional<PFN_D3D11_CREATE_DEVICE> device_creator,
    ID3D11Device** out_device, ID3D11DeviceContext** out_context) {
  if (!device_creator.has_value()) {
    FML_LOG(ERROR) << "Could not retrieve D3D11CreateDevice address.";
    return E_FAIL;
  }

  HRESULT hr = E_FAIL;
  Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;

  D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1,
                                        D3D_FEATURE_LEVEL_11_0};
#if USE_DISCRETE_GPU
  // Try to use high-performance discrete GPU
  if (SUCCEEDED(GetHighPerformanceAdapter(&adapter))) {
    hr = device_creator.value()(
        adapter.Get(),
        D3D_DRIVER_TYPE_UNKNOWN,  // Must be UNKNOWN when using a specific
                                  // adapter
        nullptr, 0, feature_levels, ARRAYSIZE(feature_levels),
        D3D11_SDK_VERSION, out_device, nullptr, out_context);
    FML_LOG(INFO) << "Use high-performance discrete GPU for D3D11CreateDevice."
                  << std::hex << hr;
  }
#endif

  // Fallback to system default hardware device
  if (FAILED(hr)) {
    hr = device_creator.value()(
        nullptr,                   // No specific adapter
        D3D_DRIVER_TYPE_HARDWARE,  // Standard hardware device
        nullptr, 0, feature_levels, ARRAYSIZE(feature_levels),
        D3D11_SDK_VERSION, out_device, nullptr, out_context);
    FML_LOG(INFO) << "Use system default hardware for D3D11CreateDevice."
                  << std::hex << hr;
  }

  return hr;
}

}  // namespace clay
