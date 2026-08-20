// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/replay_resource_cache.h"

#include <utility>

namespace lynx {
namespace tasm {
namespace replay {

ReplayResourceCache& ReplayResourceCache::GetGlobalInstance() {
  static ReplayResourceCache instance;
  return instance;
}

void ReplayResourceCache::AddExternalScript(const std::string& url,
                                            std::vector<uint8_t> data) {
  if (url.empty() || data.empty()) {
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  external_scripts_[url] = std::move(data);
  std::string path = NormalizePath(url);
  if (!path.empty() && path != url) {
    external_scripts_[path] = external_scripts_[url];
  }
}

void ReplayResourceCache::Clear() {
  std::lock_guard<std::mutex> lock(mutex_);
  external_scripts_.clear();
}

bool ReplayResourceCache::IsEmpty() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return external_scripts_.empty();
}

bool ReplayResourceCache::ResolveResource(
    const pub::LynxResourceRequest& request,
    pub::LynxResourceResponse* response) const {
  if (response == nullptr) {
    return false;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (request.type != pub::LynxResourceType::kExternalJs) {
    return false;
  }
  std::vector<uint8_t> out;
  if (LookupByUrl(external_scripts_, request.url, &out)) {
    response->data = std::move(out);
    return true;
  }
  return false;
}

bool ReplayResourceCache::LookupByUrl(
    const std::unordered_map<std::string, std::vector<uint8_t>>& store,
    const std::string& url, std::vector<uint8_t>* out) const {
  auto it = store.find(url);
  if (it != store.end() && !it->second.empty()) {
    *out = it->second;
    return true;
  }

  std::string path = NormalizePath(url);
  if (!path.empty() && path != url) {
    it = store.find(path);
    if (it != store.end() && !it->second.empty()) {
      *out = it->second;
      return true;
    }
  }

  return false;
}

std::string ReplayResourceCache::NormalizePath(const std::string& url) {
  if (url.empty()) {
    return url;
  }
  if (url[0] == '/') {
    auto pos = url.find('?');
    return pos != std::string::npos ? url.substr(0, pos) : url;
  }
  auto scheme_end = url.find("://");
  if (scheme_end == std::string::npos) {
    auto pos = url.find('?');
    return pos != std::string::npos ? url.substr(0, pos) : url;
  }
  auto path_start = url.find('/', scheme_end + 3);
  if (path_start == std::string::npos) {
    return "";
  }
  auto query_pos = url.find('?', path_start);
  if (query_pos != std::string::npos) {
    return url.substr(path_start, query_pos - path_start);
  }
  return url.substr(path_start);
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
