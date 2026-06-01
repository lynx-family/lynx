// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/rendering/render_box.h"
#include "clay/ui/rendering/render_image.h"
#include "clay/ui/rendering/render_object.h"
#include "clay/ui/rendering/renderer.h"
#include "clay/ui/rendering/text/render_text.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

class MockRenderObject : public RenderObject {
 public:
  MOCK_METHOD(const char*, GetName, (), (const, override));
  MOCK_METHOD(void, Paint, (PaintingContext & context, const FloatPoint&),
              (override));
  MOCK_METHOD(void, PaintChildren,
              (PaintingContext & context, const FloatPoint&), (override));

  bool ChildrenPaintingOrderIsDirtyForTesting() const {
    return children_.FirstChild() && sorted_children_.empty();
  }

  const std::vector<RenderObject*>& GetSortedChildrenForTesting() {
    RebuildSortedChildrenIfNeeded();
    return sorted_children_;
  }
};

class TestRendererClient : public RendererClient {
 public:
  MOCK_METHOD(void, RequestNewFrame, (), (override));

  RenderPhase GetRenderPhase() const override { return RenderPhase::kIdle; }

  fml::RefPtr<PaintImage> MakeRasterSnapshot(GrPicturePtr,
                                             skity::Vec2) override {
    return nullptr;
  }

  void RegisterUploadTask(OneShotCallback<>&&, int) override {}
};

class MockWillPaintRenderObject : public MockRenderObject {
 public:
  MOCK_METHOD(void, WillPaint, (), (override));
};

class RenderObjectTest : public ::testing::Test {
 protected:
  void SetUp() override {
    //             0
    //          /    \
    //         1       2
    //       / | \      \
    //      3  4  5      6
    // set up a tree as above
    for (int i = 0; i <= 6; i++)
      nodeList.push_back(std::make_unique<MockRenderObject>());

    nodeList[0]->AddChild(nodeList[1].get());
    nodeList[0]->AddChild(nodeList[2].get());
    nodeList[1]->AddChild(nodeList[3].get());
    nodeList[1]->AddChild(nodeList[4].get());
    nodeList[1]->AddChild(nodeList[5].get());
    nodeList[2]->AddChild(nodeList[6].get());
  }

  std::vector<std::unique_ptr<MockRenderObject>> nodeList;
};

TEST_F(RenderObjectTest, TreeTraversal) {
  EXPECT_EQ(nodeList[0]->PreviousInPreOrder(), nullptr);
  EXPECT_EQ(nodeList[6]->NextInPreOrder(), nullptr);

  RenderObject* currentNode = nodeList[0].get();
  for (size_t i = 0; i < 6; i++) {
    currentNode = currentNode->NextInPreOrder();
  }
  EXPECT_EQ(currentNode, nodeList[6].get());

  for (size_t i = 0; i < 6; i++) {
    currentNode = currentNode->PreviousInPreOrder();
  }
  EXPECT_EQ(currentNode, nodeList[0].get());
}

TEST_F(RenderObjectTest, MiscOperation) {
  EXPECT_EQ(nodeList[0]->ChildAt(0), nodeList[1].get());
  EXPECT_EQ(nodeList[0]->ChildAt(1), nodeList[2].get());
  EXPECT_EQ(nodeList[1]->ChildAt(1), nodeList[4].get());
  EXPECT_EQ(nodeList[1]->ChildAt(2), nodeList[5].get());
  EXPECT_EQ(nodeList[2]->ChildAt(0), nodeList[6].get());
  EXPECT_EQ(nodeList[2]->ChildAt(1), nullptr);

  EXPECT_EQ(nodeList[0]->LastLeafChild(), nodeList[6].get());
  EXPECT_EQ(nodeList[1]->LastLeafChild(), nodeList[5].get());

  EXPECT_TRUE(nodeList[1]->IsDescendantOf(nodeList[0].get()));
  EXPECT_TRUE(nodeList[3]->IsDescendantOf(nodeList[0].get()));
  EXPECT_TRUE(nodeList[3]->IsDescendantOf(nodeList[1].get()));
  EXPECT_TRUE(nodeList[6]->IsDescendantOf(nodeList[0].get()));
  EXPECT_FALSE(nodeList[3]->IsDescendantOf(nodeList[2].get()));
  EXPECT_FALSE(nodeList[3]->IsDescendantOf(nodeList[4].get()));
  EXPECT_FALSE(nodeList[6]->IsDescendantOf(nodeList[1].get()));
}

TEST_F(RenderObjectTest, TreeManipulation) {
  std::unique_ptr<MockRenderObject> obj = std::make_unique<MockRenderObject>();
  nodeList[2]->AddChild(obj.get(), nodeList[6].get());
  EXPECT_EQ(nodeList[6]->PreviousSibling(), obj.get());

  EXPECT_EQ(nodeList[0]->LastLeafChild(), nodeList[6].get());
  nodeList[2]->RemoveAllChildren();
  EXPECT_EQ(nodeList[0]->LastLeafChild(), nodeList[2].get());
}

TEST_F(RenderObjectTest, BoxModel) {
  std::unique_ptr<MockRenderObject> obj = std::make_unique<MockRenderObject>();
  EXPECT_FALSE(obj->HasBackground());
  EXPECT_FALSE(obj->HasBorder());
  EXPECT_EQ(obj->location(), FloatPoint());
  EXPECT_EQ(obj->Width(), 0.f);
  EXPECT_EQ(obj->Height(), 0.f);

  obj->SetLeft(10.f);
  obj->SetTop(10.f);
  obj->SetWidth(100.f);
  obj->SetHeight(100.f);
  obj->SetPaddingLeft(10.f);
  obj->SetPaddingTop(10.f);
  obj->SetPaddingRight(20.f);
  obj->SetPaddingBottom(20.f);
  EXPECT_EQ(obj->location(), FloatPoint(10.f, 10.f));
  EXPECT_EQ(obj->Width(), 100.f);
  EXPECT_EQ(obj->Height(), 100.f);
  EXPECT_EQ(obj->PaddingLeft(), 10.f);
  EXPECT_EQ(obj->PaddingTop(), 10.f);
  EXPECT_EQ(obj->PaddingRight(), 20.f);
  EXPECT_EQ(obj->PaddingBottom(), 20.f);
}

TEST_F(RenderObjectTest, PaintOrder) {
  // Initial state.
  EXPECT_EQ(nodeList[1]->ChildrenPaintingOrderIsDirtyForTesting(), true);
  const auto& sorted1 = nodeList[1]->GetSortedChildrenForTesting();
  EXPECT_EQ(nodeList[1]->ChildrenPaintingOrderIsDirtyForTesting(), false);
  EXPECT_EQ(sorted1[0], nodeList[3].get());
  EXPECT_EQ(sorted1[1], nodeList[4].get());
  EXPECT_EQ(sorted1[2], nodeList[5].get());

  // Set z-index = 5 for node 3.
  nodeList[3]->SetPaintingOrder(5);
  EXPECT_EQ(nodeList[1]->ChildrenPaintingOrderIsDirtyForTesting(), true);
  const auto& sorted2 = nodeList[1]->GetSortedChildrenForTesting();
  EXPECT_EQ(sorted2[0], nodeList[4].get());
  EXPECT_EQ(sorted2[1], nodeList[5].get());
  EXPECT_EQ(sorted2[2], nodeList[3].get());

  // Set z-index = 5 for node 3 again.
  nodeList[3]->SetPaintingOrder(5);
  EXPECT_EQ(nodeList[1]->ChildrenPaintingOrderIsDirtyForTesting(), false);

  // Set translate-z = 1 for node 4.
  TransformOperations transform3;
  transform3.AppendTranslate(0, 0, 1);
  nodeList[4]->SetTransformOperations(transform3);
  EXPECT_EQ(nodeList[1]->ChildrenPaintingOrderIsDirtyForTesting(), true);
  const auto& sorted3 = nodeList[1]->GetSortedChildrenForTesting();
  EXPECT_EQ(sorted3[0], nodeList[5].get());
  EXPECT_EQ(sorted3[1], nodeList[3].get());
  EXPECT_EQ(sorted3[2], nodeList[4].get());

  // Insert new child to node 1.
  std::unique_ptr<MockRenderObject> obj = std::make_unique<MockRenderObject>();
  nodeList[1]->AddChild(obj.get());
  EXPECT_EQ(nodeList[1]->ChildrenPaintingOrderIsDirtyForTesting(), true);
  const auto& sorted4 = nodeList[1]->GetSortedChildrenForTesting();
  EXPECT_EQ(sorted4[0], nodeList[5].get());
  EXPECT_EQ(sorted4[1], obj.get());
  EXPECT_EQ(sorted4[2], nodeList[3].get());
  EXPECT_EQ(sorted4[3], nodeList[4].get());

  // Set z-index = 10 for obj.
  obj->SetPaintingOrder(10);
  const auto& sorted5 = nodeList[1]->GetSortedChildrenForTesting();
  EXPECT_EQ(sorted5[0], nodeList[5].get());
  EXPECT_EQ(sorted5[1], nodeList[3].get());
  EXPECT_EQ(sorted5[2], obj.get());
  EXPECT_EQ(sorted5[3], nodeList[4].get());

  // Set translate-z = 1 for obj.
  TransformOperations transform6;
  transform6.AppendTranslate(0, 0, 1);
  obj->SetTransformOperations(transform6);
  const auto& sorted6 = nodeList[1]->GetSortedChildrenForTesting();
  EXPECT_EQ(sorted6[0], nodeList[5].get());
  EXPECT_EQ(sorted6[1], nodeList[3].get());
  EXPECT_EQ(sorted6[2], nodeList[4].get());
  EXPECT_EQ(sorted6[3], obj.get());

  // Set translate-z = 0 for obj and node 4.
  TransformOperations transform7;
  transform7.AppendTranslate(0, 0, 0);
  nodeList[4]->SetTransformOperations(transform7);
  obj->SetTransformOperations(transform7);
  EXPECT_EQ(nodeList[1]->ChildrenPaintingOrderIsDirtyForTesting(), true);
  const auto& sorted7 = nodeList[1]->GetSortedChildrenForTesting();
  EXPECT_EQ(sorted7[0], nodeList[4].get());
  EXPECT_EQ(sorted7[1], nodeList[5].get());
  EXPECT_EQ(sorted7[2], nodeList[3].get());
  EXPECT_EQ(sorted7[3], obj.get());

  // Remove node 3.
  nodeList[1]->RemoveChild(nodeList[3].get());
  EXPECT_EQ(nodeList[1]->ChildrenPaintingOrderIsDirtyForTesting(), true);
  const auto& sorted8 = nodeList[1]->GetSortedChildrenForTesting();
  EXPECT_EQ(sorted8[0], nodeList[4].get());
  EXPECT_EQ(sorted8[1], nodeList[5].get());
  EXPECT_EQ(sorted8[2], obj.get());

  // Update root node‘s painting order shouldn't trigger a crash.
  TransformOperations transform8;
  transform8.AppendTranslate(0, 0, 1);
  auto* root = nodeList[0].get();
  root->SetTransformOperations(transform8);
  root->SetPaintingOrder(1);
}

TEST(RenderObjectStandaloneTest, SkipDetachedChildDuringPrePaintTraversal) {
  TestRendererClient client;
  EXPECT_CALL(client, RequestNewFrame()).Times(::testing::AnyNumber());

  Renderer renderer(&client, nullptr);
  auto root = std::make_unique<MockRenderObject>();
  auto child = std::make_unique<MockWillPaintRenderObject>();
  root->AddChild(child.get());
  renderer.SetRoot(root.get());

  child->SetRenderer(nullptr);
  child->SetVisible(false);

  root->ValidateForPaint(true);
  EXPECT_CALL(*child, WillPaint()).Times(0);
  root->WillPaint();
}

}  // namespace clay
