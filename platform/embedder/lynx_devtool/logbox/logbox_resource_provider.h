// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_DEVTOOL_LOGBOX_LOGBOX_RESOURCE_PROVIDER_H_
#define PLATFORM_EMBEDDER_LYNX_DEVTOOL_LOGBOX_LOGBOX_RESOURCE_PROVIDER_H_

#include <string>
#include <string_view>
#include <unordered_map>

namespace lynx {
namespace embedder {

class LogBoxResourceProvider {
 public:
  virtual ~LogBoxResourceProvider() = default;

  virtual std::string GetEntryUrl() const = 0;
  virtual void* GetHostView() const = 0;
  virtual std::unordered_map<std::string, std::string> GetLogSources()
      const = 0;
  virtual std::string GetLogSourceByFileName(
      std::string_view file_name) const = 0;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_DEVTOOL_LOGBOX_LOGBOX_RESOURCE_PROVIDER_H_
