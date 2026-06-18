// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/logbox_embedder.h"

#include <utility>

namespace lynx {
namespace embedder {

// Static storage for the registered creator function
static LogBoxEmbedder::CreateLogBoxFunction g_create_logbox_func = nullptr;

bool LogBoxEmbedder::RegisterLogBoxCreator(CreateLogBoxFunction creator) {
  if (g_create_logbox_func != nullptr) {
    // Creator already registered
    return false;
  }
  g_create_logbox_func = creator;
  return true;
}

std::unique_ptr<LogBoxEmbedder> LogBoxEmbedder::Create(
    GetNativeWindowCallback get_native_window,
    GetAllJsSourceCallback get_all_js_source) {
  if (g_create_logbox_func) {
    return g_create_logbox_func(std::move(get_native_window),
                                std::move(get_all_js_source));
  }
  return nullptr;
}

}  // namespace embedder
}  // namespace lynx
