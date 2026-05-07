// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_JS_BINDINGS_JS_OBJECT_DESTRUCTION_OBSERVER_H_
#define CORE_RUNTIME_JS_BINDINGS_JS_OBJECT_DESTRUCTION_OBSERVER_H_

#include <memory>
#include <utility>

#include "core/base/memory/unsafe_owning_ptr.h"
#include "core/runtime/js/bindings/js_app.h"
#include "core/runtime/js/jsi/jsi.h"

namespace lynx {
namespace runtime {
namespace js {
/**
 * JSObjectDestructionObserver is used to monitor the destruction of JS
 * objects.
 *
 * After mounting the JSObjectDestructionObserver on the JS object, when the JS
 * object is destroyed, the JSObjectDestructionObserver will also be destructed,
 * and the passed JS callback function will be called asynchronously with low
 * priority.
 */
class JSObjectDestructionObserver final : public HostObject {
 public:
  explicit JSObjectDestructionObserver(base::UnsafeWeakPtr<App> app,
                                       ApiCallBack destruction_callback)
      : native_app_(std::move(app)),
        destruction_callback_(std::move(destruction_callback)) {}

  ~JSObjectDestructionObserver() override { CallDestructionCallback(); }

 private:
  // Can only be called once at destruction.
  void CallDestructionCallback() {
    if (auto* app = native_app_.Lock()) {
      app->RunOnJSThreadWhenIdle(
          [destruction_callback = std::move(destruction_callback_),
           weak_app = std::move(native_app_)]() {
            if (auto* app = weak_app.Lock()) {
              app->InvokeApiCallBack(destruction_callback);
            }
          });
    }
  }

  base::UnsafeWeakPtr<App> native_app_;
  ApiCallBack destruction_callback_;
};
}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_BINDINGS_JS_OBJECT_DESTRUCTION_OBSERVER_H_
