// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/services/replay/layout_tree_testbench.h"

#include "core/renderer/css/computed_css_style.h"
#include "core/renderer/lynx_env_config.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace replay {
class ReplayLayoutTest : public ::testing::Test {
 protected:
  ReplayLayoutTest() = default;
  ~ReplayLayoutTest() override = default;

  std::unique_ptr<starlight::ComputedCSSStyle> layout_computed_css_;
  std::shared_ptr<SLNode> root_;

  std::shared_ptr<SLNode> CreateNode() {
    auto result = std::make_shared<SLNode>(
        starlight::LayoutConfigs(),
        layout_computed_css_->GetLayoutComputedStyle());
    return result;
  }
  void SetUp() override {
    layout_computed_css_ =
        std::make_unique<starlight::ComputedCSSStyle>(1.f, 1.f);
    root_ = CreateNode();
  }
};

TEST_F(ReplayLayoutTest, GetLayoutTreeEmpty) {
  ASSERT_EQ(
      "{\"width\":0.0,\"height\":0.0,\"offset_top\":0.0,\"offset_left\":0.0,"
      "\"content\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],\"padding\":[0.0,0.0,0.0,"
      "0.0,0.0,0.0,0.0,0.0],\"border\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],"
      "\"margin\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]}",
      LayoutTreeTestBench::GetLayoutTree(root_.get()));
}

TEST_F(ReplayLayoutTest, GetLayoutTreeWithSetValue) {
  root_->SetBorderBoundTopFromParentPaddingBound(1.0f);
  root_->SetBorderBoundLeftFromParentPaddingBound(-0.0f);
  root_->SetBorderBoundWidth(3.3f);
  root_->SetBorderBoundHeight(4.44444f);
  ASSERT_EQ(
      "{\"width\":3.3,\"height\":4.44,\"offset_top\":1.0,\"offset_left\":0.0,"
      "\"content\":[0.0,1.0,3.3,1.0,3.3,5.44,0.0,5.44],\"padding\":[0.0,1.0,3."
      "3,1.0,3.3,5.44,0.0,5.44],\"border\":[0.0,1.0,3.3,1.0,3.3,5.44,0.0,5.44],"
      "\"margin\":[0.0,1.0,3.3,1.0,3.3,5.44,0.0,5.44]}",
      LayoutTreeTestBench::GetLayoutTree(root_.get()));
}

TEST_F(ReplayLayoutTest, GetLayoutTreeWithChild) {
  auto child0 = CreateNode();
  auto child1 = CreateNode();
  root_->AppendChild(child0.get());
  root_->AppendChild(child1.get());
  ASSERT_EQ(
      "{\"width\":0.0,\"height\":0.0,\"offset_top\":0.0,\"offset_left\":0.0,"
      "\"content\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],\"padding\":[0.0,0.0,0.0,"
      "0.0,0.0,0.0,0.0,0.0],\"border\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],"
      "\"margin\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],\"children\":[{\"width\":0."
      "0,\"height\":0.0,\"offset_top\":0.0,\"offset_left\":0.0,\"content\":[0."
      "0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],\"padding\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,"
      "0.0],\"border\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],\"margin\":[0.0,0.0,0."
      "0,0.0,0.0,0.0,0.0,0.0]},{\"width\":0.0,\"height\":0.0,\"offset_top\":0."
      "0,\"offset_left\":0.0,\"content\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],"
      "\"padding\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],\"border\":[0.0,0.0,0.0,0."
      "0,0.0,0.0,0.0,0.0],\"margin\":[0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]}]}",
      LayoutTreeTestBench::GetLayoutTree(root_.get()));
}
}  // namespace replay
}  // namespace tasm
}  // namespace lynx
