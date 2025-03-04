// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_TESTING_MOCK_LAYOUT_DELEGATE_H_
#define CORE_SHELL_TESTING_MOCK_LAYOUT_DELEGATE_H_

#include <memory>
#include <vector>

#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

using lynx::tasm::LayoutContext;

namespace lynx {
namespace tasm {
namespace test {

class MockLayoutDelegate : public LayoutContext::Delegate {
 public:
  void OnLayoutUpdate(int tag, float x, float y, float width, float height,
                      const std::array<float, 4>& paddings,
                      const std::array<float, 4>& margins,
                      const std::array<float, 4>& borders,
                      const std::array<float, 4>* sticky_positions,
                      float max_height) override {}
  void OnNodeLayoutAfter(int32_t id) override{};
  void PostPlatformExtraBundle(
      int32_t id, std::unique_ptr<tasm::PlatformExtraBundle> bundle) override {}
  void OnCalculatedViewportChanged(const CalculatedViewport& viewport,
                                   int tag) override {}
  void SetTiming(tasm::Timing timing) override {}
  void SetEnableAirStrictMode(bool enable_air_strict_mode) override {}
  MOCK_METHOD(void, OnFirstMeaningfulLayout, (), (override));
  MOCK_METHOD(void, OnLayoutAfter,
              (const PipelineOptions&,
               std::unique_ptr<PlatformExtraBundleHolder>, bool),
              (override));
};

}  // namespace test

}  // namespace tasm

}  // namespace lynx

#endif  // CORE_SHELL_TESTING_MOCK_LAYOUT_DELEGATE_H_
