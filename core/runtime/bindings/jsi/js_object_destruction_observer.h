// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_JSI_JS_OBJECT_DESTRUCTION_OBSERVER_H_
#define CORE_RUNTIME_BINDINGS_JSI_JS_OBJECT_DESTRUCTION_OBSERVER_H_

#include <memory>
#include <utility>

#include "core/runtime/bindings/jsi/js_app.h"
#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

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
  explicit JSObjectDestructionObserver(std::weak_ptr<App> app,
                                       ApiCallBack destruction_callback)
      : native_app_(std::move(app)),
        destruction_callback_(std::move(destruction_callback)) {}

  ~JSObjectDestructionObserver() override { CallDestructionCallback(); }

 private:
  // Can only be called once at destruction.
  void CallDestructionCallback() {
    if (auto app = native_app_.lock()) {
      app->RunOnJSThreadWhenIdle(
          [destruction_callback = std::move(destruction_callback_),
           weak_app = std::move(native_app_)]() {
            if (auto app = weak_app.lock()) {
              app->InvokeApiCallBack(destruction_callback);
            }
          });
    }
  }

  std::weak_ptr<App> native_app_;
  ApiCallBack destruction_callback_;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_JSI_JS_OBJECT_DESTRUCTION_OBSERVER_H_
