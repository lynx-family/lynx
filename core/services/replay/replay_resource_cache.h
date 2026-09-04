// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_REPLAY_RESOURCE_CACHE_H_
#define CORE_SERVICES_REPLAY_REPLAY_RESOURCE_CACHE_H_

#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/public/lynx_resource_loader.h"

namespace lynx {
namespace tasm {
namespace replay {

class ReplayResourceCache {
 public:
  ReplayResourceCache() = default;
  ~ReplayResourceCache() = default;

  static ReplayResourceCache& GetGlobalInstance();

  void AddExternalScript(const std::string& url, std::vector<uint8_t> data);
  void Clear();

  bool ResolveResource(const pub::LynxResourceRequest& request,
                       pub::LynxResourceResponse* response) const;
  bool IsEmpty() const;

 private:
  bool LookupByUrl(
      const std::unordered_map<std::string, std::vector<uint8_t>>& store,
      const std::string& url, std::vector<uint8_t>* out) const;

  static std::string NormalizePath(const std::string& url);

  mutable std::mutex mutex_;
  std::unordered_map<std::string, std::vector<uint8_t>> external_scripts_;
  std::unordered_map<std::string, std::vector<uint8_t>>
      external_script_aliases_;
};

}  // namespace replay
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_REPLAY_RESOURCE_CACHE_H_
