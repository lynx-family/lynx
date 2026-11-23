// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

#include "clay/shell/platform/windows/egl/p3_presenter.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

// cspell:ignore LINESTRIP

namespace clay::egl {
namespace {

using Microsoft::WRL::ComPtr;
using Pixel = std::array<float, 4>;

float HalfToFloat(uint16_t half) {
  const int exponent = (half >> 10) & 31;
  const int fraction = half & 1023;
  const float magnitude =
      exponent == 0 ? std::ldexp(static_cast<float>(fraction), -24)
                    : std::ldexp(1.0f + fraction / 1024.0f, exponent - 15);
  return half & 0x8000 ? -magnitude : magnitude;
}

class P3PresenterTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // WARP exercises the real shader and DXGI path without requiring a P3
    // monitor. Windows 10+ and the system D3D compiler are required.
    ASSERT_TRUE(SUCCEEDED(
        D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
                          D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                          D3D11_SDK_VERSION, &device_, nullptr, &context_)));
    ASSERT_TRUE(presenter_.Initialize(device_.Get()));
  }

  ComPtr<ID3D11Texture2D> Texture(UINT width, UINT height, DXGI_FORMAT format,
                                  const uint8_t* pixels = nullptr) {
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = desc.ArraySize = desc.SampleDesc.Count = 1;
    desc.Format = format;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    D3D11_SUBRESOURCE_DATA data = {};
    data.pSysMem = pixels;
    data.SysMemPitch = width * 4;
    ComPtr<ID3D11Texture2D> texture;
    EXPECT_TRUE(SUCCEEDED(
        device_->CreateTexture2D(&desc, pixels ? &data : nullptr, &texture)));
    return texture;
  }

  std::vector<Pixel> ReadPixels(ID3D11Texture2D* texture) {
    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;
    if (FAILED(device_->CreateTexture2D(&desc, nullptr, &staging))) {
      ADD_FAILURE() << "Cannot create readback texture";
      return {};
    }
    context_->CopyResource(staging.Get(), texture);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(context_->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
      ADD_FAILURE() << "Cannot read converted pixels";
      return {};
    }
    std::vector<Pixel> pixels;
    for (UINT y = 0; y < desc.Height; ++y) {
      const auto* row = reinterpret_cast<const uint16_t*>(
          static_cast<const uint8_t*>(mapped.pData) + y * mapped.RowPitch);
      for (UINT x = 0; x < desc.Width; ++x) {
        pixels.push_back({HalfToFloat(row[x * 4]), HalfToFloat(row[x * 4 + 1]),
                          HalfToFloat(row[x * 4 + 2]),
                          HalfToFloat(row[x * 4 + 3])});
      }
    }
    context_->Unmap(staging.Get(), 0);
    return pixels;
  }

  ComPtr<ID3D11Device> device_;
  ComPtr<ID3D11DeviceContext> context_;
  P3Presenter presenter_;
};

TEST_F(P3PresenterTest, ConvertsPrimariesAlphaAndTransferFunction) {
  // BGRA: red, green, blue; half-alpha red, transparent, neutral gray.
  const uint8_t source_pixels[] = {0,   0, 255, 255, 0,   255, 0,   255,
                                   255, 0, 0,   255, 0,   0,   128, 128,
                                   0,   0, 0,   0,   128, 128, 128, 255};
  auto source = Texture(3, 2, DXGI_FORMAT_B8G8R8A8_UNORM, source_pixels);
  auto output = Texture(3, 2, DXGI_FORMAT_R16G16B16A16_FLOAT);
  ASSERT_TRUE(presenter_.Present(source.Get(), output.Get()));
  const auto actual = ReadPixels(output.Get());
  // D65 Display P3 primaries expressed in linear sRGB. Extended channels
  // must not be clamped by the shader or the render-target format.
  constexpr float alpha = 128.0f / 255.0f;
  const Pixel expected[] = {
      {1.2249402f, -0.0420570f, -0.0196376f, 1},
      {-0.2249402f, 1.0420570f, -0.0786360f, 1},
      {0, 0, 1.0982736f, 1},
      {1.2249402f * alpha, -0.0420570f * alpha, -0.0196376f * alpha, alpha},
      {0, 0, 0, 0},
      {0.2158605f, 0.2158605f, 0.2158605f, 1}};
  ASSERT_EQ(actual.size(), 6u);
  for (size_t i = 0; i < actual.size(); ++i) {
    for (size_t channel = 0; channel < 4; ++channel) {
      EXPECT_NEAR(actual[i][channel], expected[i][channel], 0.001f)
          << "pixel " << i << ", channel " << channel;
    }
  }
}

TEST_F(P3PresenterTest, UpdatesReferenceWhiteForRetainedContent) {
  const uint8_t bgra[] = {255, 255, 255, 255, 128, 128, 128, 255,
                          0,   0,   128, 128, 0,   0,   0,   0};
  auto source = Texture(4, 1, DXGI_FORMAT_B8G8R8A8_UNORM, bgra);
  auto output = Texture(4, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
  ASSERT_TRUE(presenter_.Present(source.Get(), output.Get()));
  const auto sdr = ReadPixels(output.Get());
  ASSERT_EQ(sdr.size(), 4u);
  for (float scale : {3.0f, 2.0f, 1.0f}) {
    ASSERT_TRUE(presenter_.Present(source.Get(), output.Get(), scale));
    const auto actual = ReadPixels(output.Get());
    ASSERT_EQ(actual.size(), sdr.size());
    for (size_t i = 0; i < sdr.size(); ++i) {
      for (size_t channel = 0; channel < 3; ++channel) {
        EXPECT_NEAR(actual[i][channel], sdr[i][channel] * scale, 0.002f);
      }
      EXPECT_EQ(actual[i][3], sdr[i][3]);
    }
  }
}

TEST_F(P3PresenterTest, RestoresImmediateContextState) {
  ComPtr<ID3D10Multithread> multithread;
  ASSERT_TRUE(SUCCEEDED(device_.As(&multithread)));
  EXPECT_TRUE(multithread->GetMultithreadProtected());
  auto source = Texture(2, 2, DXGI_FORMAT_B8G8R8A8_UNORM);
  auto output = Texture(2, 2, DXGI_FORMAT_R16G16B16A16_FLOAT);
  ComPtr<ID3D11RenderTargetView> target;
  ASSERT_TRUE(SUCCEEDED(
      device_->CreateRenderTargetView(source.Get(), nullptr, &target)));
  ID3D11RenderTargetView* targets[] = {target.Get()};
  context_->OMSetRenderTargets(1, targets, nullptr);
  const D3D11_VIEWPORT viewport = {1, 2, 13, 17, 0.25f, 0.75f};
  context_->RSSetViewports(1, &viewport);
  context_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
  const float blend_factor[] = {0.1f, 0.2f, 0.3f, 0.4f};
  context_->OMSetBlendState(nullptr, blend_factor, 0x12345678);

  // Repeat to catch state saved into the wrong context-state object.
  for (int frame = 0; frame < 3; ++frame) {
    ASSERT_TRUE(presenter_.Present(source.Get(), output.Get()));
    ComPtr<ID3D11RenderTargetView> restored_target;
    context_->OMGetRenderTargets(1, &restored_target, nullptr);
    EXPECT_EQ(restored_target.Get(), target.Get());
    D3D11_PRIMITIVE_TOPOLOGY topology;
    context_->IAGetPrimitiveTopology(&topology);
    EXPECT_EQ(topology, D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    D3D11_VIEWPORT restored_viewport = {};
    UINT count = 1;
    context_->RSGetViewports(&count, &restored_viewport);
    EXPECT_EQ(count, 1u);
    EXPECT_EQ(restored_viewport.TopLeftX, viewport.TopLeftX);
    EXPECT_EQ(restored_viewport.TopLeftY, viewport.TopLeftY);
    EXPECT_EQ(restored_viewport.Width, viewport.Width);
    EXPECT_EQ(restored_viewport.Height, viewport.Height);
    EXPECT_EQ(restored_viewport.MinDepth, viewport.MinDepth);
    EXPECT_EQ(restored_viewport.MaxDepth, viewport.MaxDepth);
    float restored_blend[4] = {};
    UINT mask = 0;
    context_->OMGetBlendState(nullptr, restored_blend, &mask);
    EXPECT_EQ(mask, 0x12345678u);
    for (int i = 0; i < 4; ++i) {
      EXPECT_EQ(restored_blend[i], blend_factor[i]);
    }
  }
}

TEST_F(P3PresenterTest, RejectsInvalidTargetsAndCanReinitialize) {
  auto source = Texture(2, 2, DXGI_FORMAT_B8G8R8A8_UNORM);
  auto wrong_size = Texture(1, 2, DXGI_FORMAT_R16G16B16A16_FLOAT);
  auto output = Texture(2, 2, DXGI_FORMAT_R16G16B16A16_FLOAT);
  EXPECT_FALSE(presenter_.Present(source.Get(), wrong_size.Get()));
  EXPECT_FALSE(presenter_.Present(source.Get(), source.Get()));
  EXPECT_FALSE(presenter_.Present(nullptr, output.Get()));
  EXPECT_FALSE(presenter_.Present(source.Get(), nullptr));
  EXPECT_FALSE(P3Presenter::ConfigureSwapChain(nullptr));
  EXPECT_FALSE(presenter_.Initialize(nullptr));
  EXPECT_FALSE(presenter_.Present(source.Get(), output.Get()));
  ASSERT_TRUE(presenter_.Initialize(device_.Get()));
  EXPECT_TRUE(presenter_.Present(source.Get(), output.Get()));
}

TEST_F(P3PresenterTest, RetainsUnchangedPixelsAcrossFramesAndResizes) {
  const uint8_t source_pixels[] = {0, 0, 255, 255, 255, 0, 0, 255};
  auto source = Texture(2, 1, DXGI_FORMAT_B8G8R8A8_UNORM, source_pixels);
  auto output = Texture(2, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
  ASSERT_TRUE(presenter_.Present(source.Get(), output.Get()));
  const auto first = ReadPixels(output.Get());
  const uint8_t green[] = {0, 255, 0, 255};
  const D3D11_BOX update = {0, 0, 0, 1, 1, 1};
  context_->UpdateSubresource(source.Get(), 0, &update, green, 4, 0);
  ASSERT_TRUE(presenter_.Present(source.Get(), output.Get()));
  const auto second = ReadPixels(output.Get());
  ASSERT_EQ(first.size(), 2u);
  ASSERT_EQ(second.size(), 2u);
  EXPECT_LT(second[0][0], 0);
  EXPECT_GT(second[0][1], 1);
  EXPECT_EQ(second[1], first[1]);
  auto resized_source = Texture(1, 1, DXGI_FORMAT_B8G8R8A8_UNORM, green);
  auto resized_output = Texture(1, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
  ASSERT_TRUE(presenter_.Present(resized_source.Get(), resized_output.Get()));
  const auto resized = ReadPixels(resized_output.Get());
  ASSERT_EQ(resized.size(), 1u);
  EXPECT_EQ(resized[0], second[0]);
}

}  // namespace
}  // namespace clay::egl
