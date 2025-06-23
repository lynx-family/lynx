// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_LYNX_TEXT_INFO_MODULE_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_LYNX_TEXT_INFO_MODULE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_context.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/module/native_module_capi.h"

namespace lynx {
namespace harmony {

class LynxTextInfoModule : public NativeModuleCAPI {
 public:
  static std::shared_ptr<piper::LynxNativeModule> Create(
      const std::shared_ptr<tasm::harmony::LynxContext>& context) {
    return std::make_shared<LynxTextInfoModule>(context);
  }

  static const std::string& GetName() { return name_; }

  explicit LynxTextInfoModule(
      const std::shared_ptr<tasm::harmony::LynxContext>& context);

  ~LynxTextInfoModule() override = default;

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const piper::CallbackMap& callbacks) override;

  std::unique_ptr<pub::Value> GetTextInfo(std::unique_ptr<pub::Value> args,
                                          const piper::CallbackMap& callbacks);

  void Destroy() override;

 private:
  static const std::string name_;
  std::weak_ptr<tasm::harmony::LynxContext> context_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_MODULE_LYNX_TEXT_INFO_MODULE_H_
