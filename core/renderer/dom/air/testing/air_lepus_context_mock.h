// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_TESTING_AIR_LEPUS_CONTEXT_MOCK_H_
#define CORE_RENDERER_DOM_AIR_TESTING_AIR_LEPUS_CONTEXT_MOCK_H_

#include <vector>

#include "core/shell/runtime/mts/mts_runtime.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"

namespace lynx {
namespace air {
namespace testing {
class AirMockLepusContext : public runtime::MTSRuntime {
 public:
  AirMockLepusContext()
      : runtime::MTSRuntime(runtime::ContextType::LepusNGContextType) {}

  lepus::Value CallClosureArgs(const lepus::Value& closure,
                               const std::vector<lepus::Value>& args) override {
    return lepus::Value(1);
  }
};

}  // namespace testing
}  // namespace air
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_TESTING_AIR_LEPUS_CONTEXT_MOCK_H_
