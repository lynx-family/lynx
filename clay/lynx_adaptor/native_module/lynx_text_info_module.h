// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_TEXT_INFO_MODULE_H_
#define CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_TEXT_INFO_MODULE_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/fml/task_runner.h"
#include "core/public/jsb/lynx_native_module.h"

namespace lynx {

class LynxTextInfoModule : public runtime::LynxNativeModule {
 public:
  explicit LynxTextInfoModule(uint32_t view_context_id);
  ~LynxTextInfoModule() override = default;

  static std::shared_ptr<LynxNativeModule> Create(
      uint32_t view_context_id, fml::RefPtr<fml::TaskRunner> task_runner) {
    return std::make_shared<LynxTextInfoModule>(view_context_id);
  }

  static const std::string& GetName() { return name_; }

  base::expected<std::unique_ptr<pub::Value>, std::string> InvokeMethod(
      const std::string& method_name, std::unique_ptr<pub::Value> args,
      size_t count, const runtime::CallbackMap& callbacks) override;

  std::unique_ptr<pub::Value> GetTextInfo(const std::string& content,
                                          const pub::Value& info);

 private:
  static const std::string name_;
  uint32_t view_context_id_;
};

}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_NATIVE_MODULE_LYNX_TEXT_INFO_MODULE_H_
