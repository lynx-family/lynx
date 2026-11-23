// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_EGL_P3_PRESENTER_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_EGL_P3_PRESENTER_H_

#include <d3d10_1.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace clay::egl {

// Converts premultiplied Display P3 into linear extended sRGB for a Windows
// FP16 composition swap chain. Negative channels must survive this conversion.
class P3Presenter {
 public:
  bool Initialize(ID3D11Device* device);
  bool Present(ID3D11Texture2D* source, ID3D11Texture2D* destination,
               float sdr_white_scale = 1.0f);
  static bool ConfigureSwapChain(IDXGISwapChain1* swap_chain);

 private:
  Microsoft::WRL::ComPtr<ID3D11Device> device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext1> context_;
  Microsoft::WRL::ComPtr<ID3D10Multithread> multithread_;
  Microsoft::WRL::ComPtr<ID3DDeviceContextState> state_;
  float uploaded_white_scale_ = 0.0f;
  Microsoft::WRL::ComPtr<ID3D11Buffer> white_level_buffer_;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> vertex_shader_;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> pixel_shader_;
};

}  // namespace clay::egl

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_EGL_P3_PRESENTER_H_
