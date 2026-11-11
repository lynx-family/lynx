// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/module_interceptor_registry.h"

#include <algorithm>
#include <mutex>
#include <utility>
#include <vector>

#include "base/include/no_destructor.h"
#include "core/runtime/js/bindings/modules/module_interceptor.h"

namespace lynx {
namespace runtime {
namespace js {

namespace {
struct Registry {
  std::mutex mutex;
  std::vector<ModuleInterceptorRegistry::Creator> creators;
};
Registry& GetRegistry() {
  static base::NoDestructor<Registry> registry;
  return *registry;
}
}  // namespace

void ModuleInterceptorRegistry::RegisterLeading(Creator creator) {
  if (!creator) {
    return;
  }
  auto& registry = GetRegistry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  auto& creators = registry.creators;
  if (std::find(creators.begin(), creators.end(), creator) == creators.end()) {
    creators.push_back(creator);
  }
}

std::shared_ptr<GroupInterceptor> ModuleInterceptorRegistry::Install(
    std::shared_ptr<GroupInterceptor> group) {
  auto& registry = GetRegistry();
  std::lock_guard<std::mutex> lock(registry.mutex);
  // Prepend in reverse so the registration order is kept.
  for (auto it = registry.creators.rbegin(); it != registry.creators.rend();
       ++it) {
    auto interceptor = (*it)();
    if (!interceptor) {
      continue;
    }
    if (!group) {
      group = std::make_shared<GroupInterceptor>();
    }
    group->AddLeadingInterceptor(std::move(interceptor));
  }
  if (group && group->empty()) {
    return nullptr;
  }
  return group;
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
