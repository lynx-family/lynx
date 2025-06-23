// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/module/native_module_capi.h"

#include <utility>

#include "core/base/js_constants.h"
#include "core/public/jsb/lynx_module_callback.h"

namespace lynx {
namespace harmony {

base::expected<std::unique_ptr<pub::Value>, std::string>
NativeModuleCAPI::InvokeMethod(const std::string& method_name,
                               std::unique_ptr<pub::Value> args, size_t count,
                               const piper::CallbackMap& callbacks) {
  auto delegate = delegate_.lock();
  if (!delegate) {
    return std::unique_ptr<pub::Value>(nullptr);
  }
  delegate->RunOnPlatformThread([this, method_name, args = std::move(args),
                                 callbacks = std::move(callbacks)]() mutable {
    auto it = invocations_.find(method_name);
    if (it != invocations_.end()) {
      (this->*(it->second))(std::move(args), callbacks);
    }
  });
  return std::unique_ptr<pub::Value>(nullptr);
}

}  // namespace harmony
}  // namespace lynx
