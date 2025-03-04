// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_AIR_TESTING_AIR_LEPUS_CONTEXT_MOCK_H_
#define CORE_RENDERER_DOM_AIR_TESTING_AIR_LEPUS_CONTEXT_MOCK_H_

#include <vector>

#include "core/runtime/vm/lepus/quick_context.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"

namespace lynx {
namespace air {
namespace testing {
class AirMockLepusContext : public lepus::QuickContext {
 public:
  virtual lepus::Value CallClosureArgs(const lepus::Value& closure,
                                       const lepus::Value* args[],
                                       size_t args_count) override {
    return lepus::Value(1);
  }
};

}  // namespace testing
}  // namespace air
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_AIR_TESTING_AIR_LEPUS_CONTEXT_MOCK_H_
