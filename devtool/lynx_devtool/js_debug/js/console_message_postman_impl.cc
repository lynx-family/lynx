// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/js/console_message_postman_impl.h"

namespace lynx {
namespace devtool {

void ConsoleMessagePostManImpl::OnMessagePosted(
    const piper::ConsoleMessage& message) {
  auto iter = observer_vec_.begin();
  while (iter != observer_vec_.end()) {
    auto sp = (*iter).lock();
    if (sp) {
      sp->OnConsoleMessagePosted(message);
      ++iter;
    } else {
      iter = observer_vec_.erase(iter);
    }
  }
}

void ConsoleMessagePostManImpl::InsertRuntimeObserver(
    const std::shared_ptr<piper::InspectorRuntimeObserverNG>& observer) {
  if (observer) {
    observer_vec_.emplace_back(
        std::static_pointer_cast<InspectorRuntimeObserverImpl>(observer));
  }
}

}  // namespace devtool
}  // namespace lynx
