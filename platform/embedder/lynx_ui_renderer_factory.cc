// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_ui_renderer_factory.h"

#include <utility>

#include "platform/embedder/interfaces/renderer_factory.h"
#include "platform/embedder/lynx_ui_renderer.h"
#include "platform/embedder/lynx_view_builder_priv.h"
#include "platform/embedder/resource/resource_provider_embedder.h"

namespace lynx::embedder {
namespace {
#if !defined(OS_IOS) || defined(ENABLE_WINDOWLESS)
RendererConfig MakeConfig(lynx_view_builder_t* builder) {
  RendererConfig config;
  config.x = builder->frame.x;
  config.y = builder->frame.y;
  config.width = builder->frame.width;
  config.height = builder->frame.height;
  config.pixel_ratio = builder->screen_size.pixel_ratio;
  config.parent = builder->parent;
  config.enable_software_rendering = builder->enable_software_rendering;
  config.default_background_color = builder->default_background_color;
  if (const char* path = builder->GetICUDataPath()) config.icu_data_path = path;
  config.native_view_creators = std::move(builder->native_view_creators);
  if (builder->generic_fetcher)
    config.resource_provider = CreateResourceProvider(builder->generic_fetcher);
  return config;
}
#endif
}  // namespace

std::unique_ptr<LynxUIRenderer> CreateUIRenderer(lynx_view_builder_t* builder) {
  if (!builder) return nullptr;
#if defined(ENABLE_WINDOWLESS)
  if (builder->windowless_renderer) {
    return builder->windowless_renderer->CreateUIRenderer(MakeConfig(builder));
  }
#endif
#if defined(OS_IOS)
  // The iOS C API requires an explicit windowless binding.
  return nullptr;
#else
  return CreateDefaultUIRenderer(MakeConfig(builder));
#endif
}
}  // namespace lynx::embedder
