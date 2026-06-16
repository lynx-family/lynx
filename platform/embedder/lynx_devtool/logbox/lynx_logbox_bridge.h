// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_DEVTOOL_LOGBOX_LYNX_LOGBOX_BRIDGE_H_
#define PLATFORM_EMBEDDER_LYNX_DEVTOOL_LOGBOX_LYNX_LOGBOX_BRIDGE_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace lynx {
namespace embedder {

class LogBoxResourceProvider;

struct LogBoxErrorInfo {
  int level = 0;
  int32_t error_code = 0;
  std::string message;
  std::string fix_suggestion;
  std::unordered_map<std::string, std::string> custom_info;
  bool is_logbox_only = false;
};

class LynxLogBoxBridge {
 public:
  static std::unique_ptr<LynxLogBoxBridge> Create(
      LogBoxResourceProvider* provider);

  virtual ~LynxLogBoxBridge() = default;

  virtual void OnHostViewAttached() = 0;
  virtual void OnError(const LogBoxErrorInfo& error) = 0;
  virtual void OnReload() = 0;
  virtual void OnDestroy() = 0;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_DEVTOOL_LOGBOX_LYNX_LOGBOX_BRIDGE_H_
