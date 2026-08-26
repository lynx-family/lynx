// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/js_bundle.h"

#include <functional>
#include <optional>
#include <unordered_map>
#include <utility>

namespace lynx {
namespace runtime {
namespace js {

void AddAppServiceWrapForJsContent(std::string &js_content) {
  static constexpr char kPrefix[] =
      "(function(){\n"
      "function __init_card_bundle__(lynxCoreInject){\n"
      "var tt = lynxCoreInject.tt;\n"
      "tt.define(\"app-service.js\", function("
      "require, module, exports, Card, setTimeout, setInterval, "
      "clearInterval, clearTimeout, NativeModules, tt, console, Component, "
      "TaroLynx, nativeAppId, Behavior, LynxJSBI, lynx, window, document, "
      "frames, self, location, navigator, localStorage, history, Caches, "
      "screen, alert, confirm, prompt, fetch, XMLHttpRequest, WebSocket, "
      "webkit, Reporter, print, global, requestAnimationFrame, "
      "cancelAnimationFrame){\n";
  static constexpr char kSuffix[] =
      "\n});\n"
      "tt.require(\"app-service.js\");\n"
      "}\n"
      "return {init: __init_card_bundle__};\n"
      "})();";

  js_content.reserve(js_content.size() + sizeof(kPrefix) + sizeof(kSuffix));
  js_content.insert(0, kPrefix);
  js_content.append(kSuffix);
}

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

}  // namespace js
}  // namespace runtime
}  // namespace lynx
