// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_EGL_GRAPHICS_PREPARATION_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_EGL_GRAPHICS_PREPARATION_H_

#include <d3d11.h>
#include <wrl/client.h>

#include <memory>

#include "clay/fml/native_library.h"

namespace clay {
namespace egl {

struct PreparedGraphicsDevice {
  // Release the device before unloading its library.
  fml::RefPtr<fml::NativeLibrary> library;
  Microsoft::WRL::ComPtr<ID3D11Device> device;
};

bool PrepareGraphicsAsync();
std::unique_ptr<PreparedGraphicsDevice> TakePreparedGraphicsDevice();

}  // namespace egl
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_EGL_GRAPHICS_PREPARATION_H_
