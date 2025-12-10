// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_TESTING_MOCK_LAYOUT_PLATFORM_H_
#define CORE_SHELL_TESTING_MOCK_LAYOUT_PLATFORM_H_

#include <string>
#include <unordered_set>
#include <vector>

#include "core/public/layout_ctx_platform_impl.h"
#include "core/public/layout_node_manager.h"
#include "core/public/layout_node_value.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

using lynx::tasm::LayoutContext;

namespace lynx {
namespace tasm {
namespace test {
class MockPlatformImpl : public LayoutCtxPlatformImpl {
 public:
  MockPlatformImpl() {}
  virtual ~MockPlatformImpl() override {}
  void SetLayoutNodeManager(LayoutNodeManager* layout_node_manager) override{};
  int CreateLayoutNode(int id, const std::string& tag, PropBundle* props,
                       bool allow_inline) override {
    int result = LayoutNodeType::UNKNOWN;
    if (tag == "raw-text" || tag == "inline-text") {
      result |= LayoutNodeType::VIRTUAL;
    }
    if (tag == "view") {
      result |= LayoutNodeType::COMMON;
    }
    if (tag == "inline-view") {
      result |= LayoutNodeType::INLINE;
    }
    if (tag == "text") {
      result |= LayoutNodeType::CUSTOM;
    }
    return result;
  };
  void InsertLayoutNode(int parent, int child, int index) override {}
  void RemoveLayoutNode(int parent, int child, int index) override {}
  void DestroyLayoutNodes(const std::unordered_set<int>& ids) override {}
  void ScheduleLayout() override {}
  void OnLayoutBefore(int id) override { before_ids.push_back(id); }
  void OnLayout(int id, float left, float top, float width, float height,
                const std::array<float, 4>& paddings,
                const std::array<float, 4>& borders) override {}
  void Destroy() override {}

  void SetFontFaces(const CSSFontFaceRuleMap&) override {}
  MOCK_METHOD(void, UpdateLayoutNode, (int, PropBundle*), (override));

  std::vector<int32_t> before_ids;
};
}  // namespace test
}  // namespace tasm
}  // namespace lynx
#endif  // CORE_SHELL_TESTING_MOCK_LAYOUT_PLATFORM_H_
