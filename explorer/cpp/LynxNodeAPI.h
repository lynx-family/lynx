// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef EXPLORER_CPP_LYNXNODEAPI_H_
#define EXPLORER_CPP_LYNXNODEAPI_H_

#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace lynx {
namespace explorer {

class LynxNodeAPI {
 public:
  static LynxNodeAPI& GetInstance() {
    static LynxNodeAPI instance;
    return instance;
  }

  void PutEnvByToken(uint64_t token_id, void* napi_env_ptr);
  void RemoveEnvByToken(uint64_t token_id);
  void RegisterStaticNodeAddon(const std::string& addon_name,
                               void* register_func);
  void RequireNodeAddonByToken(uint64_t token_id,
                               const std::string& addon_name);
  void RequireNodeAddon(void* napi_env_ptr, const std::string& addon_name);

 private:
  LynxNodeAPI() = default;
  ~LynxNodeAPI() = default;

  struct NodeAddon {
    void* moduleHandle = nullptr;
    void* init = nullptr;
    std::string generatedName;
  };

  bool LoadNodeAddon(NodeAddon& addon, const std::string& libraryName) const;
  bool InitializeNodeModule(void* env_ptr, NodeAddon& addon);

  std::mutex env_mutex_;
  std::unordered_map<uint64_t, void*> tokenEnvMap_;
  std::unordered_map<std::string, NodeAddon> nodeAddons_;
  std::unordered_map<std::string, void*> staticNodeAddons_;
};

void RegisterStaticNodeAddon(const char* addon_name, void* register_func);

}  // namespace explorer
}  // namespace lynx

#endif  // EXPLORER_CPP_LYNXNODEAPI_H_
