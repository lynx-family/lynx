// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lazy_bundle/bundle_manager.h"

#include <utility>

namespace lynx {
namespace tasm {

void BundleManager::InsertTemplateBundle(const std::string& url,
                                         LynxTemplateBundle bundle) {
  std::lock_guard<std::mutex> lock(mutex_);
  loaded_bundles_.try_emplace(url, std::move(bundle));
}

std::optional<LynxTemplateBundle> BundleManager::GetTemplateBundle(
    const std::string& url) const {
  std::lock_guard<std::mutex> lock(mutex_);
  auto bundle_iter = loaded_bundles_.find(url);
  return bundle_iter == loaded_bundles_.end()
             ? std::nullopt
             : std::make_optional(bundle_iter->second);
}

void BundleManager::MergeFrom(const BundleManager& other) {
  if (this == &other) {
    return;
  }

  std::scoped_lock lock(mutex_, other.mutex_);
  for (const auto& [url, bundle] : other.loaded_bundles_) {
    loaded_bundles_.try_emplace(url, bundle);
  }
}

}  // namespace tasm
}  // namespace lynx
