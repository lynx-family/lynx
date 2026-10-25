// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_INTERFACES_RENDERER_CONFIG_H_
#define PLATFORM_EMBEDDER_INTERFACES_RENDERER_CONFIG_H_
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

#include "base/include/fml/memory/ref_counted.h"
#include "platform/embedder/interfaces/resource_provider.h"
#include "platform/embedder/public/capi/lynx_view_builder_capi.h"
namespace lynx::embedder {
class LynxUIRenderer;
// Immutable creation snapshot. It owns all data needed after Builder release.
struct RendererConfig {
  float x = 0, y = 0, width = 0, height = 0, pixel_ratio = 1;
  NativeWindow parent = nullptr;
  bool enable_software_rendering = false;
  std::optional<uint32_t> default_background_color;
  std::string icu_data_path;
  std::shared_ptr<ResourceProvider> resource_provider;
  std::unordered_map<std::string, std::pair<lynx_native_view_creator, void*>>
      native_view_creators;
};
class RendererBinding : public fml::RefCountedThreadSafeStorage {
 public:
  virtual std::unique_ptr<LynxUIRenderer> CreateUIRenderer(
      RendererConfig config) = 0;

 protected:
  ~RendererBinding() override = default;
};
class FrameTimingClient {
 public:
  virtual ~FrameTimingClient() = default;
  virtual void OnFrameTiming(int64_t start_ns, int64_t finish_ns) = 0;
};
}  // namespace lynx::embedder
#endif  // PLATFORM_EMBEDDER_INTERFACES_RENDERER_CONFIG_H_
