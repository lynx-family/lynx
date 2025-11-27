// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/module/global_module_registry.h"

#include <utility>

#include "platform/embedder/module/lynx_fetch_module.h"

namespace lynx {
namespace embedder {

GlobalModuleRegistry& GlobalModuleRegistry::GetInstance() {
  static GlobalModuleRegistry instance;
  return instance;
}

void GlobalModuleRegistry::RegisterNativeModule(const std::string& name,
                                                napi_module_creator creator,
                                                void* opaque) {
  module_map_[name] = {creator, opaque};
}

void GlobalModuleRegistry::RegisterExtensionModule(
    const std::string& name, extension_module_creator creator,
    bool is_lazy_create, void* opaque) {
  extension_module_map_.emplace(
      name, std::make_tuple(creator, is_lazy_create, opaque));
}

void GlobalModuleRegistry::MergeWithInstanceModuleMap(
    const std::unordered_map<std::string,
                             std::pair<napi_module_creator, void*>>&
        instance_module_map,
    std::unordered_map<std::string, std::pair<napi_module_creator, void*>>&
        module_map) {
  // Register internal fetch module.
  std::pair<napi_module_creator, void*> fetch_module{
      lynx::embedder::LynxFetchModuleCreator, nullptr};
  module_map["LynxFetchModule"] = fetch_module;
  // Append global modules.
  for (const auto& it : module_map_) {
    module_map[it.first] = it.second;
  }
  // Append instance modules.
  for (const auto& it : instance_module_map) {
    module_map[it.first] = it.second;
  }
}

void GlobalModuleRegistry::MergeWithInstanceExtensionModuleMap(
    const std::unordered_map<std::string,
                             std::tuple<extension_module_creator, bool, void*>>&
        instance_module_map,
    std::unordered_map<std::string,
                       std::tuple<extension_module_creator, bool, void*>>&
        module_map) {
  // Append global modules.
  for (const auto& it : extension_module_map_) {
    module_map[it.first] = it.second;
  }
  // Append instance modules.
  for (const auto& it : instance_module_map) {
    module_map[it.first] = it.second;
  }
}

}  // namespace embedder
}  // namespace lynx
