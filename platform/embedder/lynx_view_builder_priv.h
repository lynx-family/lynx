// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_VIEW_BUILDER_PRIV_H_
#define PLATFORM_EMBEDDER_LYNX_VIEW_BUILDER_PRIV_H_

#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "platform/embedder/public/capi/lynx_env_capi.h"
#include "platform/embedder/public/capi/lynx_generic_resource_fetcher_capi.h"
#include "platform/embedder/public/capi/lynx_group_capi.h"
#include "platform/embedder/public/capi/lynx_view_builder_capi.h"
#if defined(ENABLE_WINDOWLESS)
#include "platform/embedder/windowless/lynx_windowless_renderer_priv.h"
#endif

struct lynx_view_builder_t {
  struct screen_size {
    float width = 0;
    float height = 0;
    float pixel_ratio = 1.0;
  } screen_size;
  struct frame {
    float x = 0;
    float y = 0;
    float width = 0;
    float height = 0;
  } frame;
  float font_scale = 1.0;
  std::string icu_data_path;
  lynx_group_t* group = nullptr;
  NativeWindow parent = nullptr;
#if defined(ENABLE_WINDOWLESS)
  lynx::fml::RefPtr<lynx::embedder::LynxWindowlessRenderer>
      windowless_renderer = nullptr;
#endif
  lynx_generic_resource_fetcher_t* generic_fetcher = nullptr;

  std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
      native_modules;
  std::unordered_map<std::string,
                     std::tuple<extension_module_creator, bool, void*>>
      extension_modules_;
  std::unordered_map<std::string, std::pair<lynx_native_view_creator, void*>>
      native_view_creators;

  const char* GetICUDataPath() const {
    return !icu_data_path.empty() ? icu_data_path.c_str()
                                  : lynx_env_get_icu_data_path();
  }
};

#endif  // PLATFORM_EMBEDDER_LYNX_VIEW_BUILDER_PRIV_H_
