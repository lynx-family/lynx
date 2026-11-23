// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/p3_presenter.h"

// cspell:ignore dcompiler TRIANGLELIST

#include <d3dcompiler.h>
#include <dxgi1_4.h>

namespace clay::egl {
namespace {
constexpr char kShader[] = R"(
cbuffer DisplayParameters : register(b0) { float sdrWhiteScale; };
Texture2D<float4> image : register(t0);
float4 Vertex(uint id : SV_VertexID) : SV_Position {
  float2 p = float2((id << 1) & 2, id & 2);
  return float4(p * float2(2, -2) + float2(-1, 1), 0, 1);
}
float4 Pixel(float4 position : SV_Position) : SV_Target {
  float4 c = image.Load(int3(position.xy, 0));
  if (c.a <= 0) return float4(0, 0, 0, 0);
  float3 rgb = saturate(c.rgb / c.a);
  float3 linear_rgb = float3(
      rgb.r <= 0.04045 ? rgb.r / 12.92 : pow((rgb.r + 0.055) / 1.055, 2.4),
      rgb.g <= 0.04045 ? rgb.g / 12.92 : pow((rgb.g + 0.055) / 1.055, 2.4),
      rgb.b <= 0.04045 ? rgb.b / 12.92 : pow((rgb.b + 0.055) / 1.055, 2.4));
  float3 scRGB = float3(
      dot(linear_rgb, float3(1.224940176, -0.224940176, 0)),
      dot(linear_rgb, float3(-0.042056955, 1.042056955, 0)),
      dot(linear_rgb, float3(-0.019637555, -0.078636046, 1.098273601)));
  return float4(scRGB * c.a * sdrWhiteScale, c.a);
}
)";
}  // namespace

bool P3Presenter::ConfigureSwapChain(IDXGISwapChain1* swap_chain) {
  if (!swap_chain) {
    return false;
  }
  Microsoft::WRL::ComPtr<IDXGISwapChain3> chain3;
  UINT support = 0;
  return SUCCEEDED(swap_chain->QueryInterface(IID_PPV_ARGS(&chain3))) &&
         SUCCEEDED(chain3->CheckColorSpaceSupport(
             DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709, &support)) &&
         (support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) &&
         SUCCEEDED(
             chain3->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709));
}

bool P3Presenter::Initialize(ID3D11Device* device) {
  device_.Reset();
  context_.Reset();
  multithread_.Reset();
  state_.Reset();
  white_level_buffer_.Reset();
  uploaded_white_scale_ = 0.0f;
  vertex_shader_.Reset();
  pixel_shader_.Reset();
  if (!device) {
    return false;
  }
  // Probe presentation before exposing a P3 render target. Older Windows/DXGI
  // implementations can render FP16 textures without supporting scRGB output.
  Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device;
  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
  Microsoft::WRL::ComPtr<IDXGIFactory2> factory;
  Microsoft::WRL::ComPtr<IDXGISwapChain1> probe;
  DXGI_SWAP_CHAIN_DESC1 desc = {};
  desc.Width = desc.Height = 1;
  desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
  desc.SampleDesc.Count = 1;
  desc.BufferCount = 2;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.Scaling = DXGI_SCALING_STRETCH;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
  desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
  if (FAILED(device->QueryInterface(IID_PPV_ARGS(&dxgi_device))) ||
      FAILED(dxgi_device->GetAdapter(&adapter)) ||
      FAILED(adapter->GetParent(IID_PPV_ARGS(&factory))) ||
      FAILED(factory->CreateSwapChainForComposition(device, &desc, nullptr,
                                                    &probe)) ||
      !ConfigureSwapChain(probe.Get())) {
    return false;
  }
  Microsoft::WRL::ComPtr<ID3D11Device1> device1;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
  if (FAILED(device->QueryInterface(IID_PPV_ARGS(&device1)))) {
    return false;
  }
  device->GetImmediateContext(&context);
  if (FAILED(context.As(&context_))) {
    return false;
  }
  D3D_FEATURE_LEVEL feature = device->GetFeatureLevel();
  if (FAILED(device1->CreateDeviceContextState(
          0, &feature, 1, D3D11_SDK_VERSION, __uuidof(ID3D11Device), nullptr,
          &state_))) {
    return false;
  }

  // Load the system compiler only. Do not resolve a DLL from the current
  // directory.
  HMODULE module = LoadLibraryExW(L"d3dcompiler_47.dll", nullptr,
                                  LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (!module) {
    return false;
  }
  auto compile = reinterpret_cast<decltype(&D3DCompile)>(
      GetProcAddress(module, "D3DCompile"));
  // Shader blobs can have vtables in the compiler DLL. Consume and release
  // them before unloading it, including on a compilation/creation failure.
  bool ok = [&] {
    Microsoft::WRL::ComPtr<ID3DBlob> vs, ps;
    return compile &&
           SUCCEEDED(compile(kShader, sizeof(kShader) - 1, nullptr, nullptr,
                             nullptr, "Vertex", "vs_4_0", 0, 0, &vs,
                             nullptr)) &&
           SUCCEEDED(compile(kShader, sizeof(kShader) - 1, nullptr, nullptr,
                             nullptr, "Pixel", "ps_4_0", 0, 0, &ps, nullptr)) &&
           SUCCEEDED(device->CreateVertexShader(vs->GetBufferPointer(),
                                                vs->GetBufferSize(), nullptr,
                                                &vertex_shader_)) &&
           SUCCEEDED(device->CreatePixelShader(ps->GetBufferPointer(),
                                               ps->GetBufferSize(), nullptr,
                                               &pixel_shader_));
  }();
  FreeLibrary(module);
  if (!ok) {
    return false;
  }
  // Resource uploads can use ANGLE's immediate context on another thread.
  // Lock the complete state swap/draw/restore sequence, not individual calls.
  if (FAILED(device->QueryInterface(IID_PPV_ARGS(&multithread_)))) {
    return false;
  }
  multithread_->SetMultithreadProtected(TRUE);
  if (!multithread_->GetMultithreadProtected()) {
    return false;
  }
  D3D11_BUFFER_DESC buffer_desc = {};
  buffer_desc.ByteWidth = 16;  // Constant buffers are aligned to 16 bytes.
  buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  if (FAILED(
          device->CreateBuffer(&buffer_desc, nullptr, &white_level_buffer_))) {
    return false;
  }
  device_ = device;
  return true;
}

bool P3Presenter::Present(ID3D11Texture2D* source, ID3D11Texture2D* destination,
                          float sdr_white_scale) {
  if (!device_ || !source || !destination || !(sdr_white_scale > 0)) {
    return false;
  }
  D3D11_TEXTURE2D_DESC source_desc = {};
  D3D11_TEXTURE2D_DESC desc = {};
  source->GetDesc(&source_desc);
  destination->GetDesc(&desc);
  if (source_desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM ||
      desc.Format != DXGI_FORMAT_R16G16B16A16_FLOAT ||
      source_desc.Width != desc.Width || source_desc.Height != desc.Height ||
      source_desc.SampleDesc.Count != 1 || desc.SampleDesc.Count != 1) {
    return false;
  }
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> rtv;
  if (FAILED(device_->CreateShaderResourceView(source, nullptr, &srv)) ||
      FAILED(device_->CreateRenderTargetView(destination, nullptr, &rtv))) {
    return false;
  }
  D3D11_VIEWPORT viewport = {
      0, 0, static_cast<float>(desc.Width), static_cast<float>(desc.Height),
      0, 1};
  // ANGLE caches D3D state. Restore its complete state after this pass.
  Microsoft::WRL::ComPtr<ID3DDeviceContextState> previous;
  multithread_->Enter();
  context_->SwapDeviceContextState(state_.Get(), &previous);
  context_->ClearState();
  context_->RSSetViewports(1, &viewport);
  context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  context_->VSSetShader(vertex_shader_.Get(), nullptr, 0);
  context_->PSSetShader(pixel_shader_.Get(), nullptr, 0);
  ID3D11ShaderResourceView* sources[] = {srv.Get()};
  ID3D11RenderTargetView* targets[] = {rtv.Get()};
  if (uploaded_white_scale_ != sdr_white_scale) {
    const float parameters[4] = {sdr_white_scale};
    context_->UpdateSubresource(white_level_buffer_.Get(), 0, nullptr,
                                parameters, 0, 0);
    uploaded_white_scale_ = sdr_white_scale;
  }
  context_->PSSetConstantBuffers(0, 1, white_level_buffer_.GetAddressOf());
  context_->PSSetShaderResources(0, 1, sources);
  context_->OMSetRenderTargets(1, targets, nullptr);
  context_->Draw(3, 0);
  context_->ClearState();
  context_->SwapDeviceContextState(previous.Get(), nullptr);
  multithread_->Leave();
  return true;
}

}  // namespace clay::egl
