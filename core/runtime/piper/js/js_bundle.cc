// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/piper/js/js_bundle.h"

#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>

namespace lynx::piper {
void JsBundle::AddJsContent(const std::string &path, JsContent content) {
  js_files_.emplace(path, std::move(content));
}

std::optional<std::reference_wrapper<const JsContent>> JsBundle::GetJsContent(
    const std::string &path) const {
  auto iter = js_files_.find(path);
  if (iter != js_files_.end()) {
    return std::cref(iter->second);
  }
  return std::nullopt;
}

const std::unordered_map<std::string, JsContent> &JsBundle::GetAllJsFiles()
    const {
  return js_files_;
}

}  // namespace lynx::piper
