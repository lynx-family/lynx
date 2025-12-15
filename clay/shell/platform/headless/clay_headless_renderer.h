// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_PLATFORM_HEADLESS_CLAY_HEADLESS_RENDERER_H_
#define CLAY_SHELL_PLATFORM_HEADLESS_CLAY_HEADLESS_RENDERER_H_

#include <memory>

#include "clay/public/clay.h"
#include "clay/shell/platform/embedder/embedder_surface_software.h"
#ifdef SHELL_ENABLE_GL
#include "clay/shell/platform/embedder/embedder_surface_gl.h"
#endif

#ifdef SHELL_ENABLE_METAL
#include "clay/shell/platform/embedder/embedder_surface_metal.h"
#endif

namespace clay {

class ClayHeadlessEngine;

/**
 * Provides the renderer config needed to initialize the embedder engine and
 * also handles external texture management.
 */
class ClayHeadlessRenderer {
 public:
  explicit ClayHeadlessRenderer(ClayHeadlessEngine* engine);

  /**
   * Called on UI thread
   */
  virtual ~ClayHeadlessRenderer();

  /**
   * Cleanup GPU resources, called in raster thread before destruction
   */
  virtual void CleanupGPUResources() = 0;

  virtual EmbedderSurfaceSoftwareDelegate* GetSoftwareRendererDelegate() {
    return nullptr;
  }
#ifdef SHELL_ENABLE_GL
  virtual GPUSurfaceGLDelegate* GetGLRendererDelegate() { return nullptr; }
#endif
#ifdef SHELL_ENABLE_METAL
  virtual EmbedderSurfaceMetalDelegate* GetMetalRendererDelegate() {
    return nullptr;
  }
#endif

  /**
   * Return the renderer used in FlutterRendererConfig callbacks
   * It's only overriden in ClayHeadlessRendererSharedImageHostGL,
   * since it's a "proxy" renderer
   */
  virtual ClayHeadlessRenderer* GetEngineRenderer() { return this; }

  static std::unique_ptr<ClayHeadlessRenderer> Create(
      ClayHeadlessEngine* engine, const ClayHeadlessRendererConfig& config);

#ifdef SHELL_ENABLE_GL
  static std::unique_ptr<ClayHeadlessRenderer> CreateHostGL(
      ClayHeadlessEngine* engine, const ClayOpenGLRendererConfig& config);

  static std::unique_ptr<ClayHeadlessRenderer> CreateGL(
      ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config);
#endif

#ifdef SHELL_ENABLE_METAL
  static std::unique_ptr<ClayHeadlessRenderer> CreateMetal(
      ClayHeadlessEngine* engine, const ClayHardwareRendererConfig& config);
#endif

 protected:
  ClayHeadlessEngine* engine_;
};

}  // namespace clay

#endif  // CLAY_SHELL_PLATFORM_HEADLESS_CLAY_HEADLESS_RENDERER_H_
