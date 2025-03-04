// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_CONSOLE_MESSAGE_POSTMAN_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_CONSOLE_MESSAGE_POSTMAN_IMPL_H_

#include <memory>
#include <vector>

#include "core/inspector/console_message_postman.h"
#include "devtool/lynx_devtool/js_debug/js/inspector_runtime_observer_impl.h"

namespace lynx {
namespace devtool {

class ConsoleMessagePostManImpl : public piper::ConsoleMessagePostMan {
 public:
  ConsoleMessagePostManImpl() = default;
  ~ConsoleMessagePostManImpl() override = default;

  void OnMessagePosted(const piper::ConsoleMessage& message) override;
  void InsertRuntimeObserver(
      const std::shared_ptr<piper::InspectorRuntimeObserverNG>& observer)
      override;

 private:
  std::vector<std::weak_ptr<InspectorRuntimeObserverImpl>> observer_vec_;
};
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_CONSOLE_MESSAGE_POSTMAN_IMPL_H_
