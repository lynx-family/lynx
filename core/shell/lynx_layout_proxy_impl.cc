// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/shell/lynx_layout_proxy_impl.h"

#include <memory>
#include <utility>

namespace lynx {
namespace shell {

void LynxLayoutProxyImpl::DispatchTaskToLynxLayout(base::closure task) {
  layout_actor_->Act(
      [task = std::move(task)](auto& layout) mutable { task(); });
}

void LynxLayoutProxyImpl::TriggerLayout() {
  layout_actor_->Act([](auto& layout) { layout->Layout(); });
}
}  // namespace shell
}  // namespace lynx
