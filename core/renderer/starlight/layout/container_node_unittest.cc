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

TEST(ContainerNodeTests, ContainerNodeFindIndexAndDefensiveBranches) {
  starlight::ContainerNode parent, child0, child1, child2, outsider;
  parent.AppendChild(&child0);
  parent.AppendChild(&child1);
  parent.AppendChild(&child2);

  EXPECT_EQ(&child0, parent.Find(0));
  EXPECT_EQ(&child1, parent.Find(1));
  EXPECT_EQ(&child2, parent.Find(2));
  EXPECT_EQ(nullptr, parent.Find(3));
  EXPECT_EQ(nullptr, parent.Find(-1));

  EXPECT_EQ(0, parent.IndexOf(&child0));
  EXPECT_EQ(1, parent.IndexOf(&child1));
  EXPECT_EQ(2, parent.IndexOf(&child2));
  EXPECT_EQ(-1, parent.IndexOf(&outsider));

  parent.RemoveChild(nullptr);
  EXPECT_EQ(3, parent.GetChildCount());

  starlight::ContainerNode empty_parent;
  empty_parent.RemoveChild(&outsider);
  EXPECT_EQ(0, empty_parent.GetChildCount());

  EXPECT_EQ(nullptr, outsider.parent());
}

TEST(ContainerNodeTests,
     ContainerNodeFindPastEndAndInsertBeforeOwnedReference) {
  starlight::ContainerNode empty_parent;
  EXPECT_EQ(nullptr, empty_parent.Find(1));

  starlight::ContainerNode parent, child0, child1, inserted;
  parent.AppendChild(&child0);
  parent.AppendChild(&child1);

  parent.InsertChildBefore(&inserted, &child1);
  EXPECT_EQ(&child0, parent.FirstChild());
  EXPECT_EQ(&child1, parent.LastChild());
  EXPECT_EQ(&child0, inserted.Previous());
  EXPECT_EQ(&child1, inserted.Next());
  EXPECT_EQ(&inserted, child1.Previous());
  EXPECT_EQ(3, parent.GetChildCount());
}

TEST(ContainerNodeTests, ContainerNodeDestructorDetachesChildren) {
  starlight::ContainerNode child0, child1, child2;
  {
    starlight::ContainerNode parent;
    parent.AppendChild(&child0);
    parent.AppendChild(&child1);
    parent.AppendChild(&child2);
    EXPECT_EQ(3, parent.GetChildCount());
    EXPECT_EQ(&parent, child0.parent());
    EXPECT_EQ(&parent, child1.parent());
    EXPECT_EQ(&parent, child2.parent());
  }

  EXPECT_EQ(nullptr, child0.parent());
  EXPECT_EQ(nullptr, child0.Next());
  EXPECT_EQ(nullptr, child0.Previous());
  EXPECT_EQ(nullptr, child1.parent());
  EXPECT_EQ(nullptr, child1.Next());
  EXPECT_EQ(nullptr, child1.Previous());
  EXPECT_EQ(nullptr, child2.parent());
  EXPECT_EQ(nullptr, child2.Next());
  EXPECT_EQ(nullptr, child2.Previous());
}

TEST(ContainerNodeTests, ContainerNodeInsertWithForeignReferenceDeath) {
  starlight::ContainerNode parent, other_parent, child, foreign_reference;
  other_parent.AppendChild(&foreign_reference);

  EXPECT_DEATH_IF_SUPPORTED(
      parent.InsertChildBefore(&child, &foreign_reference), "");
}

}  // namespace tasm
}  // namespace lynx
