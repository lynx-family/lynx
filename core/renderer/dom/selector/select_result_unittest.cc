// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/selector/select_result.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
struct NodeTestResult {
  int id;
};
}  // namespace testing

template <>
inline int NodeSelectResult<testing::NodeTestResult>::GetImplId(
    testing::NodeTestResult *node) {
  return node->id;
}

namespace testing {

TEST(SelectResultTest, SuccessFunctionSuccess) {
  std::vector<NodeTestResult *> nodes;
  std::unique_ptr<NodeTestResult> node = std::make_unique<NodeTestResult>();
  node->id = 100;
  nodes.push_back(node.get());
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            "id");
  NodeSelectResult<NodeTestResult> result(nodes, options, true);
  EXPECT_TRUE(result.Success());
}

TEST(SelectResultTest, SuccessFunctionFail1) {
  std::vector<NodeTestResult *> nodes;
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            "id");
  NodeSelectResult<NodeTestResult> result(nodes, options, true);
  EXPECT_FALSE(result.Success());
}

TEST(SelectResultTest, SuccessFunctionFail2) {
  std::vector<NodeTestResult *> nodes;
  std::unique_ptr<NodeTestResult> node = std::make_unique<NodeTestResult>();
  node->id = 100;
  nodes.push_back(node.get());
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            "id");
  NodeSelectResult<NodeTestResult> result(nodes, options, false);
  EXPECT_FALSE(result.Success());
}

TEST(SelectResultTest, GetOneNodeNullptr) {
  std::vector<NodeTestResult *> nodes;
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            "id");
  NodeSelectResult<NodeTestResult> result(nodes, options, true);
  EXPECT_EQ(result.GetOneNode(), nullptr);
}

TEST(SelectResultTest, GetOneNode) {
  std::vector<NodeTestResult *> nodes;
  std::unique_ptr<NodeTestResult> node = std::make_unique<NodeTestResult>();
  node->id = 100;
  nodes.push_back(node.get());
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            "id");
  NodeSelectResult<NodeTestResult> result(nodes, options, true);
  EXPECT_EQ(result.GetOneNode()->id, 100);
}

TEST(SelectResultTest, PackageLynxGetUIResultSuccess) {
  std::vector<NodeTestResult *> nodes;
  std::unique_ptr<NodeTestResult> node = std::make_unique<NodeTestResult>();
  node->id = 100;
  nodes.push_back(node.get());
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            "id");
  NodeSelectResult<NodeTestResult> result(nodes, options, true);
  EXPECT_TRUE(result.PackageLynxGetUIResult().Success());
}

TEST(SelectResultTest, PackageLynxGetUIResultFail1) {
  std::vector<NodeTestResult *> nodes;

  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            "id");
  NodeSelectResult<NodeTestResult> result(nodes, options, false);
  EXPECT_EQ(result.PackageLynxGetUIResult().ErrCode(),
            LynxGetUIResult::SELECTOR_NOT_SUPPORTED);
}

TEST(SelectResultTest, PackageLynxGetUIResultFail2) {
  std::vector<NodeTestResult *> nodes;
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            "id");
  NodeSelectResult<NodeTestResult> result(nodes, options, true);
  EXPECT_EQ(result.PackageLynxGetUIResult().ErrCode(),
            LynxGetUIResult::NODE_NOT_FOUND);
}

TEST(SelectResultTest, PackageLynxGetUIResultFail3) {
  std::vector<NodeTestResult *> nodes;
  NodeSelectOptions options(NodeSelectOptions::IdentifierType::CSS_SELECTOR,
                            "id");
  std::unique_ptr<NodeTestResult> node = std::make_unique<NodeTestResult>();
  node->id = 0;  // kInvalidImplId
  nodes.push_back(node.get());
  NodeSelectResult<NodeTestResult> result(nodes, options, true);
  EXPECT_EQ(result.PackageLynxGetUIResult().ErrCode(),
            LynxGetUIResult::NO_UI_FOR_NODE);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
