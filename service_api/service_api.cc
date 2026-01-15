// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define LYNX_SERVICE_API_NEED_EXPORTS

#include <service_api/service_api.h>

#include <utility>

namespace lynx {
namespace service {
void _BaseRegistry::set_creator(std::function<_BaseService*()> c) {
  std::lock_guard lock(mutex_);
  if (creator_ != nullptr || impl_.load(std::memory_order_relaxed)) {
    // TODO @dangyingkai introduce log_service
    // LOGE(base::FormatString("Duplicate implementations for service %s",
    //                         name.c_str()));
    return;
  }
  creator_ = std::move(c);
}

_BaseService* _BaseRegistry::get() {
  // fast-path
  auto* tmp = impl_.load(std::memory_order_acquire);
  if (tmp != nullptr) {
    return tmp;
  }

  std::lock_guard lock(mutex_);
  tmp = impl_.load(std::memory_order_relaxed);
  if (tmp != nullptr) {
    return tmp;
  }
  if (!creator_) {
    return nullptr;
  }
  tmp = creator_();
  impl_.store(tmp, std::memory_order_release);
  return tmp;
}
}  // namespace service
}  // namespace lynx
