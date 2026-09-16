// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/platform/windows/egl/graphics_preparation.h"

#include "clay/gfx/shared_image/utils/d3d11_device_creator.h"
#include "clay/shell/platform/windows/egl/async_preparation.h"

namespace clay {
namespace egl {
namespace {

AsyncPreparation<PreparedGraphicsDevice>& GetPreparation() {
  // Avoid static destruction: joining under the DLL loader lock can deadlock.
  static auto* preparation = new AsyncPreparation<PreparedGraphicsDevice>();
  return *preparation;
}

#ifndef CLAY_FORCE_D3D9
std::unique_ptr<PreparedGraphicsDevice> CreateDevice() {
  auto result = std::make_unique<PreparedGraphicsDevice>();
  result->library = fml::NativeLibrary::Create("d3d11.dll");
  if (!result->library) {
    return nullptr;
  }
  auto create_device =
      result->library->ResolveFunction<PFN_D3D11_CREATE_DEVICE>(
          "D3D11CreateDevice");
  if (FAILED(CreateSafeD3D11Device(create_device, &result->device, nullptr))) {
    return nullptr;
  }
  return result;
}
#endif

}  // namespace

bool PrepareGraphicsAsync() {
#ifndef CLAY_FORCE_D3D9
  return GetPreparation().Start(CreateDevice);
#else
  return false;
#endif
}

std::unique_ptr<PreparedGraphicsDevice> TakePreparedGraphicsDevice() {
  return GetPreparation().WaitAndTake();
}

}  // namespace egl
}  // namespace clay
