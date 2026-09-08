// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspected_context_provider.h"

#include <gtest/gtest.h>

namespace lepus_inspector {
namespace {

std::shared_ptr<LepusInspectedContext> ReturnNullFactory(
    lynx::runtime::MTSContext* context, LepusInspectorNGImpl* inspector,
    const std::string& name) {
  (void)context;
  (void)inspector;
  (void)name;
  return nullptr;
}

class LepusInspectedContextProviderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Save the previously registered factory (e.g. the classic-Lepus factory
    // installed by the internal registrar) so TearDown can restore it. The
    // provider's global slot is shared with other debugger tests in this
    // process, and leaving it cleared would break them depending on order.
    saved_factory_ =
        LepusInspectedContextProvider::RegisterFactory(&ReturnNullFactory);
  }

  void TearDown() override {
    LepusInspectedContextProvider::RegisterFactory(saved_factory_);
  }

  LepusInspectedContextProvider::Factory saved_factory_{nullptr};
};

TEST_F(LepusInspectedContextProviderTest,
       RegisterFactoryRoutesGetInspectedContext) {
  auto result = LepusInspectedContextProvider::GetInspectedContext(
      nullptr, nullptr, "test");
  EXPECT_EQ(result, nullptr);
}

}  // namespace
}  // namespace lepus_inspector
