// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/layout/layout_node.h"

#include <memory>
#include <unordered_map>

#include "core/renderer/lynx_env_config.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

class LayoutNodeTests : public ::testing::Test {
 protected:
  LayoutNodeTests() = default;
  ~LayoutNodeTests() override = default;

  int id_ = 0;
  std::unordered_map<int, LayoutNode> nodes_;

  std::unique_ptr<starlight::ComputedCSSStyle> layout_computed_css_;
  LayoutNode* root_;
  starlight::LayoutConfigs layout_configs_;
  LynxEnvConfig layout_env_config_{0, 0, 1.f, 1.f};

  LayoutNode* CommonNode() {
    auto id = id_++;
    auto result =
        &nodes_
             .emplace(
                 std::piecewise_construct, std::forward_as_tuple(id),
                 std::forward_as_tuple(id, layout_configs_, layout_env_config_,
                                       *layout_computed_css_))
             .first->second;
    result->set_type(LayoutNodeType::COMMON);
    return result;
  }

  LayoutNode* VirtualNode() {
    auto id = id_++;
    auto result =
        &nodes_
             .emplace(
                 std::piecewise_construct, std::forward_as_tuple(id),
                 std::forward_as_tuple(id, layout_configs_, layout_env_config_,
                                       *layout_computed_css_))
             .first->second;
    result->set_type(LayoutNodeType::VIRTUAL);
    return result;
  }

  void SetUp() override {
    layout_computed_css_ =
        std::make_unique<starlight::ComputedCSSStyle>(1.f, 1.f);
    root_ = CommonNode();
  }

  void AppendSomeCommonNodes(int number) {
    for (int i = 0; i < number; ++i) {
      root_->InsertNode(CommonNode());
    }
  }

  void AppendSomeVirtualNodes(int number) {
    for (int i = 0; i < number; ++i) {
      root_->InsertNode(VirtualNode());
    }
  }
};

TEST_F(LayoutNodeTests, InsertCommon) {
  auto child = CommonNode();
  root_->InsertNode(child);
  EXPECT_EQ(1, root_->slnode()->GetChildCount());
  EXPECT_EQ(child->slnode(), root_->slnode()->FirstChild());
}

TEST_F(LayoutNodeTests, InsertVirtual) {
  root_->InsertNode(VirtualNode());
  EXPECT_EQ(0, root_->slnode()->GetChildCount());
}

TEST_F(LayoutNodeTests, InsertCommonWithinCommon) {
  auto child0 = CommonNode();
  auto child1 = CommonNode();
  auto child2 = CommonNode();
  AppendSomeCommonNodes(10);
  root_->InsertNode(child0);
  root_->InsertNode(child2);
  AppendSomeCommonNodes(10);

  root_->InsertNode(child1, 11);
  EXPECT_EQ(child1->slnode(), child2->slnode()->Previous());
  EXPECT_EQ(child1->slnode(), child0->slnode()->Next());
}

TEST_F(LayoutNodeTests, InsertCommonBeforeVirtual) {
  auto child0 = CommonNode();
  auto child1 = CommonNode();
  auto child2 = CommonNode();
  AppendSomeCommonNodes(10);
  root_->InsertNode(child0);
  AppendSomeVirtualNodes(10);
  root_->InsertNode(child2);
  AppendSomeCommonNodes(10);

  root_->InsertNode(child1, 11);
  EXPECT_EQ(child1->slnode(), child2->slnode()->Previous());
  EXPECT_EQ(child1->slnode(), child0->slnode()->Next());
}

TEST_F(LayoutNodeTests, RemoveCommonBeforeVirtual) {
  auto child0 = CommonNode();
  auto child1 = CommonNode();
  auto child2 = CommonNode();
  AppendSomeCommonNodes(10);
  root_->InsertNode(child0);
  root_->InsertNode(child1);
  AppendSomeVirtualNodes(10);
  root_->InsertNode(child2);
  AppendSomeCommonNodes(10);

  root_->RemoveNodeAtIndex(11);
  EXPECT_EQ(child0->slnode(), child2->slnode()->Previous());
}

TEST_F(LayoutNodeTests, InsertCommonAfterVirtual) {
  auto child0 = CommonNode();
  auto child1 = CommonNode();
  auto child2 = CommonNode();
  AppendSomeCommonNodes(10);
  root_->InsertNode(child0);
  AppendSomeVirtualNodes(10);
  root_->InsertNode(child2);
  AppendSomeCommonNodes(10);

  root_->InsertNode(child1, 21);
  EXPECT_EQ(child1->slnode(), child2->slnode()->Previous());
  EXPECT_EQ(child1->slnode(), child0->slnode()->Next());
}

TEST_F(LayoutNodeTests, RemoveCommonAfterVirtual) {
  auto child0 = CommonNode();
  auto child1 = CommonNode();
  auto child2 = CommonNode();
  AppendSomeCommonNodes(10);
  root_->InsertNode(child0);
  AppendSomeVirtualNodes(10);
  root_->InsertNode(child1);
  root_->InsertNode(child2);
  AppendSomeCommonNodes(10);

  root_->RemoveNodeAtIndex(21);
  EXPECT_EQ(child0->slnode(), child2->slnode()->Previous());
}

TEST_F(LayoutNodeTests, InsertCommonWithinVirtual) {
  auto child0 = CommonNode();
  auto child1 = CommonNode();
  auto child2 = CommonNode();
  AppendSomeCommonNodes(10);
  root_->InsertNode(child0);
  AppendSomeVirtualNodes(10);
  root_->InsertNode(child2);
  AppendSomeCommonNodes(10);

  root_->InsertNode(child1, 15);
  EXPECT_EQ(child1->slnode(), child2->slnode()->Previous());
  EXPECT_EQ(child1->slnode(), child0->slnode()->Next());
}

TEST_F(LayoutNodeTests, RemoveCommonWithinVirtual) {
  auto child0 = CommonNode();
  auto child1 = CommonNode();
  auto child2 = CommonNode();
  AppendSomeCommonNodes(10);
  root_->InsertNode(child0);
  AppendSomeVirtualNodes(10);
  root_->InsertNode(child1);
  AppendSomeVirtualNodes(10);
  root_->InsertNode(child2);
  AppendSomeCommonNodes(10);

  root_->RemoveNodeAtIndex(21);
  EXPECT_EQ(child0->slnode(), child2->slnode()->Previous());
}

}  // namespace tasm
}  // namespace lynx
