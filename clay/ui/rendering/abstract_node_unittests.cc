// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/rendering/abstract_node.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

namespace {
class MockAbstractNode : public AbstractNode {
 public:
  MOCK_METHOD(void, RedepthChildren, (), (override));
};
}  // namespace

TEST(AbstractNodeTest, ModifyTree) {
  MockAbstractNode rootNode;
  MockAbstractNode childNode;
  MockAbstractNode grandchildNode;
  EXPECT_CALL(childNode, RedepthChildren()).Times(1);
  EXPECT_CALL(grandchildNode, RedepthChildren()).Times(1);

  rootNode.AdoptChild(&childNode);
  childNode.AdoptChild(&grandchildNode);
  EXPECT_EQ(childNode.Parent(), &rootNode);
  EXPECT_EQ(childNode.Depth(), 1u);
  EXPECT_EQ(grandchildNode.Parent(), &childNode);
  EXPECT_EQ(grandchildNode.Depth(), 2u);

  rootNode.DropChild(&childNode);
  childNode.DropChild(&grandchildNode);
  EXPECT_FALSE(childNode.Parent());
  EXPECT_FALSE(grandchildNode.Parent());
}

}  // namespace testing
}  // namespace clay
