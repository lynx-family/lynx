// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_DEVTOOL_DEVTOOL_ENV_EMBEDDER_H_
#define PLATFORM_EMBEDDER_LYNX_DEVTOOL_DEVTOOL_ENV_EMBEDDER_H_

#include <string>
#include <unordered_map>
#include <vector>

namespace lynx {
namespace embedder {

class DevToolEnvEmbedder {
 public:
  static DevToolEnvEmbedder& GetInstance();
  DevToolEnvEmbedder();
  DevToolEnvEmbedder(const DevToolEnvEmbedder&) = delete;
  DevToolEnvEmbedder& operator=(const DevToolEnvEmbedder&) = delete;

  void EnableLynxDebug(bool enable);
  bool IsLynxDebugEnabled() const;

  void EnableDevTool(bool enabled);
  bool IsDevToolEnabled() const;

  bool IsLogBoxEnabled() const;

  void SetDevToolSwitch(std::string key, bool value);
  bool GetDevToolSwitch(std::string key) const;

  void SetAppInfo(const std::string& key, const std::string& value);
  void SetAppInfo(const std::unordered_map<std::string, std::string>& app_info);

 private:
  bool NeedPersist(std::string key);

 private:
  std::unordered_map<std::string, std::string> app_infos_;
  std::unordered_map<std::string, std::vector<bool> >
      switch_persistent_default_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_DEVTOOL_DEVTOOL_ENV_EMBEDDER_H_
