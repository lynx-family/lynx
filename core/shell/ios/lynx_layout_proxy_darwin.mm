// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/shell/ios/lynx_layout_proxy_darwin.h"

namespace lynx {

namespace shell {

void LynxLayoutProxyDarwin::RunOnLayoutThread(dispatch_block_t task) {
  layout_proxy_->DispatchTaskToLynxLayout([task]() { task(); });
}

void LynxLayoutProxyDarwin::TriggerLayout() { layout_proxy_->TriggerLayout(); }

}  // namespace shell
}  // namespace lynx
