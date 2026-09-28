// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_LYNX_CALLBACK_TESTBENCH_H_
#define CORE_SERVICES_REPLAY_LYNX_CALLBACK_TESTBENCH_H_
#include <memory>
#include <string>

#include "core/runtime/js/bindings/modules/lynx_jsi_module_callback.h"
#include "core/runtime/js/jsi/jsi.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace runtime {
namespace js {
class ModuleCallbackTestBench : public ModuleCallback {
 public:
  ModuleCallbackTestBench(int64_t callback_id);
  ~ModuleCallbackTestBench() override = default;
  rapidjson::Document argument;
  std::string fixture_json;
  bool guarded = false;
  std::weak_ptr<void> lifetime;
  void Invoke(Runtime *runtime, ModuleCallbackFunctionHolder *holder) override;
};
}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_LYNX_CALLBACK_TESTBENCH_H_
