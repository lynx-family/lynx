// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_MODULE_GLOBAL_MODULE_REGISTRY_H_
#define PLATFORM_EMBEDDER_MODULE_GLOBAL_MODULE_REGISTRY_H_

#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "platform/embedder/public/capi/lynx_extension_module_capi.h"
#include "platform/embedder/public/capi/lynx_native_module_capi.h"

namespace lynx {
namespace embedder {

class GlobalModuleRegistry {
 public:
  static GlobalModuleRegistry& GetInstance();

  void RegisterNativeModule(const std::string& name,
                            napi_module_creator creator, void* opaque);
  void RegisterExtensionModule(const std::string& name,
                               extension_module_creator creator,
                               bool is_lazy_create, void* opaque);

  void MergeWithInstanceModuleMap(
      const std::unordered_map<std::string,
                               std::pair<napi_module_creator, void*>>&
          instance_module_map,
      std::unordered_map<std::string, std::pair<napi_module_creator, void*>>&
          module_map);

  void MergeWithInstanceExtensionModuleMap(
      const std::unordered_map<
          std::string, std::tuple<extension_module_creator, bool, void*>>&
          instance_module_map,
      std::unordered_map<std::string,
                         std::tuple<extension_module_creator, bool, void*>>&
          module_map);

  GlobalModuleRegistry(const GlobalModuleRegistry&) = delete;
  GlobalModuleRegistry& operator=(const GlobalModuleRegistry&) = delete;

 private:
  GlobalModuleRegistry() = default;

  std::unordered_map<std::string, std::pair<napi_module_creator, void*>>
      module_map_;
  std::unordered_map<std::string,
                     std::tuple<extension_module_creator, bool, void*>>
      extension_module_map_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_MODULE_GLOBAL_MODULE_REGISTRY_H_
