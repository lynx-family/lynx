// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/starlight/layout/container_node.h"

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {

TEST(ContainerNodeTests, ContainerNodeEmptyInit) {
  starlight::ContainerNode node;
  EXPECT_EQ(nullptr, node.Next());
  EXPECT_EQ(nullptr, node.Previous());
  EXPECT_EQ(nullptr, node.parent());
  EXPECT_EQ(nullptr, node.FirstChild());
  EXPECT_EQ(nullptr, node.LastChild());
  EXPECT_EQ(0, node.GetChildCount());
}

TEST(ContainerNodeTests, ContainerNodeEmptyInsert) {
  starlight::ContainerNode parent, child;
  parent.InsertChildBefore(&child, nullptr);
  EXPECT_EQ(&parent, child.parent());
  EXPECT_EQ(&child, parent.FirstChild());
  EXPECT_EQ(&child, parent.LastChild());
  EXPECT_EQ(1, parent.GetChildCount());
}

TEST(ContainerNodeTests, ContainerNodeEmptyAppend) {
  starlight::ContainerNode parent, child;
  parent.AppendChild(&child);
  EXPECT_EQ(&parent, child.parent());
  EXPECT_EQ(&child, parent.FirstChild());
  EXPECT_EQ(&child, parent.LastChild());
  EXPECT_EQ(1, parent.GetChildCount());
}

TEST(ContainerNodeTests, ContainerNodeAppend) {
  starlight::ContainerNode parent, child0, child1;
  parent.AppendChild(&child0);
  parent.AppendChild(&child1);
  EXPECT_EQ(&child0, parent.FirstChild());
  EXPECT_EQ(&child1, parent.LastChild());
  EXPECT_EQ(&child0, child1.Previous());
  EXPECT_EQ(&child1, child0.Next());
  EXPECT_EQ(nullptr, child1.Next());
  EXPECT_EQ(nullptr, child0.Previous());
  EXPECT_EQ(2, parent.GetChildCount());
}

TEST(ContainerNodeTests, ContainerNodeInsertFront) {
  starlight::ContainerNode parent, child0, child1, child2;
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);

  parent.InsertChildBefore(&child0, &child1);
  EXPECT_EQ(&child0, parent.FirstChild());
  EXPECT_EQ(&child2, parent.LastChild());
  EXPECT_EQ(&child0, child1.Previous());
  EXPECT_EQ(&child1, child0.Next());
  EXPECT_EQ(nullptr, child0.Previous());
  EXPECT_EQ(3, parent.GetChildCount());
}

TEST(ContainerNodeTests, ContainerNodeInsertMiddle) {
  starlight::ContainerNode parent, child0, child1, child2;
  parent.AppendChild(&child0);
  parent.AppendChild(&child2);

  parent.InsertChildBefore(&child1, &child2);
  EXPECT_EQ(&child0, parent.FirstChild());
  EXPECT_EQ(&child2, parent.LastChild());
  EXPECT_EQ(&child0, child1.Previous());
  EXPECT_EQ(&child1, child0.Next());
  EXPECT_EQ(&child2, child1.Next());
  EXPECT_EQ(&child1, child2.Previous());
  EXPECT_EQ(3, parent.GetChildCount());
}

TEST(ContainerNodeTests, ContainerNodeInsertEnd) {
  starlight::ContainerNode parent, child0, child1, child2;
  parent.AppendChild(&child0);
  parent.AppendChild(&child1);

  parent.InsertChildBefore(&child2, nullptr);
  EXPECT_EQ(&child0, parent.FirstChild());
  EXPECT_EQ(&child2, parent.LastChild());
  EXPECT_EQ(&child1, child2.Previous());
  EXPECT_EQ(nullptr, child2.Next());
  EXPECT_EQ(&child2, child1.Next());
  EXPECT_EQ(3, parent.GetChildCount());
}

TEST(ContainerNodeTests, ContainerNodeRemoveFirst) {
  starlight::ContainerNode parent, child0, child1, child2;
  parent.AppendChild(&child0);
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);

  parent.RemoveChild(&child0);
  EXPECT_EQ(&child1, parent.FirstChild());
  EXPECT_EQ(&child2, parent.LastChild());
  EXPECT_EQ(nullptr, child1.Previous());
  EXPECT_EQ(nullptr, child0.parent());
  EXPECT_EQ(nullptr, child0.Next());
  EXPECT_EQ(nullptr, child0.Previous());
  EXPECT_EQ(2, parent.GetChildCount());
}

TEST(ContainerNodeTests, ContainerNodeRemoveMiddle) {
  starlight::ContainerNode parent, child0, child1, child2;
  parent.AppendChild(&child0);
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);

  parent.RemoveChild(&child1);
  EXPECT_EQ(&child0, parent.FirstChild());
  EXPECT_EQ(&child2, parent.LastChild());
  EXPECT_EQ(&child0, child2.Previous());
  EXPECT_EQ(&child2, child0.Next());
}

TEST(ContainerNodeTests, ContainerNodeRemoveLast) {
  starlight::ContainerNode parent, child0, child1, child2;
  parent.AppendChild(&child0);
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);

  parent.RemoveChild(&child2);
  EXPECT_EQ(&child0, parent.FirstChild());
  EXPECT_EQ(&child1, parent.LastChild());
  EXPECT_EQ(nullptr, child1.Next());
}

TEST(ContainerNodeTests, ContainerNodeRemoveOnlyNode) {
  starlight::ContainerNode parent, child;
  parent.AppendChild(&child);

  parent.RemoveChild(&child);
  EXPECT_EQ(nullptr, parent.FirstChild());
  EXPECT_EQ(nullptr, parent.LastChild());
}

}  // namespace tasm
}  // namespace lynx
