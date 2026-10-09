// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_REPLAY_LYNX_MODULE_FIXTURE_REPLAY_H_
#define CORE_SERVICES_REPLAY_LYNX_MODULE_FIXTURE_REPLAY_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/services/replay/fixture_context.h"
#include "core/services/replay/lynx_module_testbench.h"

namespace lynx {
namespace runtime {
namespace js {

// NativeModule adapter backed by handlers registered in fixture.js.
// Arguments use recorder JSON conventions for undefined and NaN; callbacks
// retain function placeholders through FixtureContext.

class ModuleFixtureReplay : public ModuleTestBench {
 public:
  ModuleFixtureReplay(
      const std::string& name, const std::shared_ptr<ModuleDelegate>& delegate,
      std::shared_ptr<tasm::replay::FixtureContext> fixture_context)
      : ModuleTestBench(name, delegate),
        fixture_context_(std::move(fixture_context)) {}
  ~ModuleFixtureReplay() override = default;

  void Destroy() override {
    lifetime_.reset();
    fixture_context_.reset();
  }

 protected:
  base::expected<Value, JSINativeException> invokeMethod(
      const MethodMetadata& method, Runtime* rt, const Value* args,
      size_t count) override;

  Value invokeMethodKernel(const MethodMetadata& method, Runtime* rt,
                           const Value* args, size_t count);

 private:
  // Serializes the non-function arguments of a JSB call to a JSON array string
  // and collects callbacks plus their positions in the original argument list.
  std::string SerializeArgs(Runtime* rt, const Value* args, size_t count,
                            std::vector<Function>& out_callbacks,
                            std::vector<size_t>& out_callback_positions);

  std::weak_ptr<tasm::replay::FixtureContext> fixture_context_;
  std::shared_ptr<int> lifetime_ = std::make_shared<int>(0);
};

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_SERVICES_REPLAY_LYNX_MODULE_FIXTURE_REPLAY_H_
