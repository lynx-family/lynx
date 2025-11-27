// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_WINDOWS_EGL_MANAGER_H_
#define CLAY_SHELL_PLATFORM_WINDOWS_EGL_MANAGER_H_

// OpenGL ES and EGL includes
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// Windows platform specific includes
#include <d3d11.h>
#include <dcomp.h>
#include <windows.h>
#include <wrl/client.h>

#include <memory>

#include "base/include/fml/macros.h"
#include "base/include/fml/memory/ref_ptr.h"
#include "clay/fml/native_library.h"
#include "clay/shell/platform/windows/egl/context.h"
#include "clay/shell/platform/windows/egl/surface.h"
#include "clay/shell/platform/windows/egl/window_surface.h"

namespace clay {
namespace egl {

// A manager for initializing ANGLE correctly and using it to create and
// destroy surfaces
class Manager {
 public:
  static std::unique_ptr<Manager> Create();

  virtual ~Manager();

  // Whether the manager is currently valid.
  bool IsValid() const;

  // Creates an EGL surface that can be used to render a Flutter view into a
  // win32 HWND.
  //
  // After the surface is created, |WindowSurface::SetVSyncEnabled| should be
  // called on a thread that can make the surface current.
  //
  // HWND is the window backing the surface. Width and height are the surface's
  // physical pixel dimensions.
  //
  // Returns nullptr on failure.
  virtual std::unique_ptr<WindowSurface> CreateWindowSurface(
      GLImplementationType type, HWND hwnd, size_t width, size_t height);
  // Check if the current thread has a context bound.
  bool HasContextCurrent();

  // Creates a |EGLSurface| from the provided handle.
  EGLSurface CreateSurfaceFromHandle(EGLenum handle_type,
                                     EGLClientBuffer handle,
                                     const EGLint* attributes) const;

  // Gets the |EGLDisplay|.
  EGLDisplay egl_display() const { return display_; };

  // Gets the |ID3D11Device| chosen by ANGLE.
  bool GetDevice(ID3D11Device** device);

  // Get the EGL context used to render Flutter views.
  virtual Context* render_context() const;

  // Get the EGL context used for async texture uploads.
  virtual Context* resource_context() const;

 protected:
  // Creates a new surface manager retaining reference to the passed-in target
  // for the lifetime of the manager.
  explicit Manager();

 private:
  // Number of active instances of Manager
  static int instance_count_;

  // Initialize the EGL display.
  bool InitializeDisplay();

  // Initialize the EGL configs.
  bool InitializeConfig();

  // Initialize the EGL render and resource contexts.
  bool InitializeContexts();

  // Initialize the D3D11 device.
  bool InitializeDevice();

  // Try initialize the D3D11 device.
  bool TryInitializeD3D11Device();

  bool TryInitializeDirectCompositionDevice();

  void CleanUp();

  // Whether the manager was initialized successfully.
  bool is_valid_ = false;

  // EGL representation of native display.
  EGLDisplay display_ = EGL_NO_DISPLAY;

  // EGL framebuffer configuration.
  EGLConfig config_ = nullptr;

  // The EGL context used to render Flutter views.
  std::unique_ptr<Context> render_context_;

  // The EGL context used for async texture uploads.
  std::unique_ptr<Context> resource_context_;

  EGLDeviceEXT egl_device_ = nullptr;
  fml::RefPtr<fml::NativeLibrary> d3d11_;
  fml::RefPtr<fml::NativeLibrary> dcomp_;

  // The current D3D device.
  Microsoft::WRL::ComPtr<ID3D11Device> resolved_device_ = nullptr;

  Microsoft::WRL::ComPtr<IDCompositionDevice2> dcomp_device_ = nullptr;

  BASE_DISALLOW_COPY_AND_ASSIGN(Manager);
};

}  // namespace egl
}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_WINDOWS_EGL_MANAGER_H_
