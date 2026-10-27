// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/rendering/render_object.h"
#include "clay/ui/rendering/render_object_child_list.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace testing {

namespace {
class MockRenderObject : public RenderObject {
 public:
  MOCK_METHOD(const char*, GetName, (), (const, override));
  MOCK_METHOD(void, Paint, (PaintingContext & context, const FloatPoint&),
              (override));
  MOCK_METHOD(void, PaintChildren,
              (PaintingContext & context, const FloatPoint&), (override));
};
}  // namespace

TEST(RenderObjectChildListTest, Basic) {
  MockRenderObject rootNode;
  MockRenderObject childNode1;
  MockRenderObject childNode2;
  MockRenderObject childNode3;
  RenderObjectChildList childrenList;

  rootNode.AdoptChild(&childNode1);
  childrenList.AppendChild(&rootNode, &childNode1);
  EXPECT_EQ(childrenList.FirstChild(), childrenList.LastChild());
  EXPECT_EQ(childrenList.FirstChild(), &childNode1);
  EXPECT_EQ(childrenList.IndexOfChild(&childNode1), 0);

  rootNode.AdoptChild(&childNode3);
  childrenList.InsertChild(&rootNode, &childNode3, nullptr);
  rootNode.AdoptChild(&childNode2);
  childrenList.InsertChild(&rootNode, &childNode2, &childNode3);
  EXPECT_EQ(childrenList.FirstChild(), &childNode1);
  EXPECT_EQ(childrenList.LastChild(), &childNode3);
  EXPECT_EQ(childrenList.IndexOfChild(&childNode1), 0);
  EXPECT_EQ(childrenList.IndexOfChild(&childNode2), 1);
  EXPECT_EQ(childrenList.IndexOfChild(&childNode3), 2);

  childrenList.RemoveChild(&rootNode, &childNode2);
  rootNode.DropChild(&childNode2);
  EXPECT_EQ(childrenList.FirstChild(), &childNode1);
  EXPECT_EQ(childrenList.LastChild(), &childNode3);
  EXPECT_EQ(childNode1.PreviousSibling(), nullptr);
  EXPECT_EQ(childNode1.NextSibling(), &childNode3);
  EXPECT_EQ(childNode3.PreviousSibling(), &childNode1);
  EXPECT_EQ(childNode3.NextSibling(), nullptr);

  childrenList.RemoveChild(&rootNode, &childNode1);
  rootNode.DropChild(&childNode1);
  childrenList.RemoveChild(&rootNode, &childNode3);
  rootNode.DropChild(&childNode3);
  EXPECT_EQ(childrenList.FirstChild(), childrenList.LastChild());
  EXPECT_EQ(childrenList.FirstChild(), nullptr);
  EXPECT_EQ(childrenList.IndexOfChild(&childNode1), -1);
}

}  // namespace testing
}  // namespace clay
