// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_LYNX_INTERSECTION_OBSERVER_MODULE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_LYNX_INTERSECTION_OBSERVER_MODULE_H_

#include <memory>
#include <string>

#include "platform/harmony/lynx_harmony/src/main/cpp/module/native_module_capi.h"

namespace lynx {
namespace harmony {

class LynxIntersectionObserverModule : public NativeModuleCAPI {
 public:
  static std::shared_ptr<piper::LynxNativeModule> Create(
      const std::shared_ptr<tasm::harmony::LynxContext>& context) {
    return std::make_shared<LynxIntersectionObserverModule>(context);
  }

  static const std::string& GetName() { return name_; }

  LynxIntersectionObserverModule(
      const std::shared_ptr<tasm::harmony::LynxContext>& context);
  ~LynxIntersectionObserverModule() override = default;

  void Destroy() override;

  std::unique_ptr<pub::Value> CreateIntersectionObserver(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks);

  std::unique_ptr<pub::Value> RelativeTo(std::unique_ptr<pub::Value> args,
                                         const piper::CallbackMap& callbacks);

  std::unique_ptr<pub::Value> RelativeToViewport(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks);

  std::unique_ptr<pub::Value> RelativeToScreen(
      std::unique_ptr<pub::Value> args, const piper::CallbackMap& callbacks);

  std::unique_ptr<pub::Value> Observe(std::unique_ptr<pub::Value> args,
                                      const piper::CallbackMap& callbacks);

  std::unique_ptr<pub::Value> Disconnect(std::unique_ptr<pub::Value> args,
                                         const piper::CallbackMap& callbacks);

 private:
  static const std::string name_;
  std::weak_ptr<tasm::harmony::LynxContext> context_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_LYNX_INTERSECTION_OBSERVER_MODULE_H_
