// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LAZY_BUNDLE_BUNDLE_MANAGER_H_
#define CORE_RESOURCE_LAZY_BUNDLE_BUNDLE_MANAGER_H_

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>

#include "core/template_bundle/lynx_template_bundle.h"

namespace lynx {
namespace tasm {

// Thread-safe template-bundle cache shared by loaders on different threads.
// The first bundle registered for a URL remains authoritative.
class BundleManager {
 public:
  BundleManager() = default;
  ~BundleManager() = default;

  BundleManager(const BundleManager&) = delete;
  BundleManager& operator=(const BundleManager&) = delete;
  BundleManager(BundleManager&&) = delete;
  BundleManager& operator=(BundleManager&&) = delete;

  void InsertTemplateBundle(const std::string& url, LynxTemplateBundle bundle);

  std::optional<LynxTemplateBundle> GetTemplateBundle(
      const std::string& url) const;

  // Adds entries that are absent locally without replacing existing bundles.
  void MergeFrom(const BundleManager& other);

 private:
  std::unordered_map<std::string, LynxTemplateBundle> loaded_bundles_;
  mutable std::mutex mutex_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RESOURCE_LAZY_BUNDLE_BUNDLE_MANAGER_H_
