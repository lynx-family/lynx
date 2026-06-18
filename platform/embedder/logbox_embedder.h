// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LOGBOX_EMBEDDER_H_
#define PLATFORM_EMBEDDER_LOGBOX_EMBEDDER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace lynx {
namespace embedder {

class LogBoxEmbedder {
 public:
  using GetNativeWindowCallback = std::function<void*()>;
  using GetAllJsSourceCallback =
      std::function<std::unordered_map<std::string, std::string>()>;

  // Factory function type for creating LogBoxEmbedder instances
  using CreateLogBoxFunction = std::unique_ptr<LogBoxEmbedder> (*)(
      GetNativeWindowCallback get_native_window,
      GetAllJsSourceCallback get_all_js_source);

  // Register a factory function to create LogBoxEmbedder instances.
  // Returns false if a creator was already registered.
  static bool RegisterLogBoxCreator(CreateLogBoxFunction creator);

  static std::unique_ptr<LogBoxEmbedder> Create(
      GetNativeWindowCallback get_native_window,
      GetAllJsSourceCallback get_all_js_source);

  virtual ~LogBoxEmbedder() = default;

  virtual void OnLoadTemplate(const std::string& url) = 0;
  virtual void OnErrorOccurred(
      int level, int32_t error_code, const std::string& message,
      const std::string& fix_suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only) = 0;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LOGBOX_EMBEDDER_H_
