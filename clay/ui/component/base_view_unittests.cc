// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/fml/logging.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/rendering/render_container.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

PointerEvent CreateDownPointer(float x, float y) {
  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.position = {x, y};
  return event;
}

class BaseViewTest : public UITest {};

TEST_F_UI(BaseViewTest, TreeManipulation) {
  int view_id = 0;
  std::unique_ptr<BaseView> root =
      std::make_unique<View>(view_id++, page_.get());
  View* childView1 = new View(view_id++, page_.get());
  root->AddChild(childView1);
  View* childView2 = new View(view_id++, page_.get());
  View* childView3 = new View(view_id++, page_.get());
  root->AddChild(childView3);
  root->AddChild(childView2, 1);
  EXPECT_EQ(root->child_count(), 3u);
  EXPECT_EQ(root->Parent(), nullptr);
  EXPECT_EQ(childView1->Parent(), root.get());
  EXPECT_EQ(childView2->Parent(), root.get());
  EXPECT_EQ(childView3->Parent(), root.get());

  root->RemoveChild(childView3);
  EXPECT_EQ(childView3->Parent(), nullptr);
  delete childView3;

  EXPECT_EQ(root->child_count(), 2u);
  root->DestroyAllChildren();
  root->Destroy();
  EXPECT_EQ(root->child_count(), 0u);
}

TEST_F_UI(BaseViewTest, HitTest) {
  //     0     100     200 250    450 600   800
  //     |---------------|
  //     |     View1     |
  // 200 |       |-------------------|------|
  // 300 |-------|    View3          |      |
  // 350         |         |--------||      |
  //             |         |  View4 ||      |
  //             |         |--------||      |
  //             |                   |      |
  // 700         |-------------------|      |
  //             |                          |
  //             |           View2          |
  //             |                          |
  // 1000        |--------------------------|
  //

  std::unique_ptr<BaseView> root = std::make_unique<View>(0, page_.get());
  // View type doesn't matter. All views in the region will be added in.
  BaseView* View1 = new View(1, page_.get());
  BaseView* View2 = new View(2, page_.get());
  BaseView* View3 = new View(3, page_.get());
  BaseView* View4 = new View(4, page_.get());
  root->AddChild(View1);
  root->AddChild(View2);
  View2->AddChild(View3);
  View3->AddChild(View4);
  EXPECT_EQ(root->child_count(), 2u);

  root->SetX(0.f);
  root->SetY(0.f);
  root->SetWidth(1000.f);
  root->SetHeight(1000.f);

  View1->SetX(0.f);
  View1->SetY(0.f);
  View1->SetWidth(200.f);
  View1->SetHeight(300.f);

  View2->SetX(100.f);
  View2->SetY(200.f);
  View2->SetWidth(800.f);
  View2->SetHeight(800.f);

  View3->SetX(0.f);
  View3->SetY(0.f);
  View3->SetWidth(500.f);
  View3->SetHeight(500.f);

  View4->SetX(150.f);
  View4->SetY(100.f);
  View4->SetWidth(200.f);
  View4->SetHeight(200.f);

  View3->OnLayoutUpdated();
  View4->OnLayoutUpdated();

  {
    HitTestResult hit_test_result;
    root->HitTest(CreateDownPointer(300, 100), hit_test_result);
    // root
    EXPECT_EQ(static_cast<int>(hit_test_result.size()), 1);
  }

  {
    HitTestResult hit_test_result;
    root->HitTest(CreateDownPointer(150, 350), hit_test_result);
    // root / view2 / view3
    EXPECT_EQ(static_cast<int>(hit_test_result.size()), 3);
  }

  {
    HitTestResult hit_test_result;
    root->HitTest(CreateDownPointer(650, 750), hit_test_result);
    EXPECT_EQ(static_cast<int>(hit_test_result.size()), 2);
    int list[] = {2, 0};
    int index = 0;
    for (auto it = hit_test_result.begin(); it != hit_test_result.end(); ++it) {
      EXPECT_EQ(static_cast<BaseView*>(it->get())->id(), list[index]);
      index++;
    }
  }

  root->DestroyAllChildren();
  root->Destroy();
}

class BaseViewWithChildrenTest : public UITest {
 protected:
  void UISetUp() override {
    for (int i = 0; i <= 6; i++) {
      nodeList.push_back(std::make_unique<View>(i, page_.get()));
    }

    nodeList[0]->AddChild(nodeList[1].get());
    nodeList[0]->AddChild(nodeList[2].get());
    nodeList[1]->AddChild(nodeList[3].get());
    nodeList[1]->AddChild(nodeList[4].get());
    nodeList[1]->AddChild(nodeList[5].get());
    nodeList[2]->AddChild(nodeList[6].get());
  }

  void UITearDown() override { nodeList.clear(); }

  bool ChildrenPaintingOrderIsDirtyForTesting(BaseView* view) {
    return !view->children_.empty() && view->sorted_children_.empty();
  }

  const std::vector<BaseView*>& GetSortedChildrenForTesting(BaseView* view) {
    view->RebuildSortedChildrenIfNeeded();
    return view->sorted_children_;
  }

  std::vector<std::unique_ptr<BaseView>> nodeList;
};

TEST_F_UI(BaseViewWithChildrenTest, PaintOrder) {
  // Initial state.
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted1 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), false);
  EXPECT_EQ(sorted1[0], nodeList[3].get());
  EXPECT_EQ(sorted1[1], nodeList[4].get());
  EXPECT_EQ(sorted1[2], nodeList[5].get());

  // Set z-index = 5 for node 3.
  nodeList[3]->SetPaintingOrder(5);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted2 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted2[0], nodeList[4].get());
  EXPECT_EQ(sorted2[1], nodeList[5].get());
  EXPECT_EQ(sorted2[2], nodeList[3].get());

  // Set z-index = 5 for node 3 again.
  nodeList[3]->SetPaintingOrder(5);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), false);

  // Set translate-z = 1 for node 4.
  TransformOperations transform3;
  transform3.AppendTranslate(0, 0, 1);
  nodeList[4]->SetProperty(ClayAnimationPropertyType::kTransform, transform3,
                           false);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted3 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted3[0], nodeList[5].get());
  EXPECT_EQ(sorted3[1], nodeList[3].get());
  EXPECT_EQ(sorted3[2], nodeList[4].get());

  // Insert new child to node 1.
  BaseView* obj = new View(7, page_.get());
  nodeList[1]->AddChild(obj);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted4 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted4[0], nodeList[5].get());
  EXPECT_EQ(sorted4[1], obj);
  EXPECT_EQ(sorted4[2], nodeList[3].get());
  EXPECT_EQ(sorted4[3], nodeList[4].get());

  // Set z-index = 10 for obj.
  obj->SetPaintingOrder(10);
  const auto& sorted5 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted5[0], nodeList[5].get());
  EXPECT_EQ(sorted5[1], nodeList[3].get());
  EXPECT_EQ(sorted5[2], obj);
  EXPECT_EQ(sorted5[3], nodeList[4].get());

  // Set translate-z = 1 for obj.
  TransformOperations transform6;
  transform6.AppendTranslate(0, 0, 1);
  obj->SetProperty(ClayAnimationPropertyType::kTransform, transform6, false);
  const auto& sorted6 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted6[0], nodeList[5].get());
  EXPECT_EQ(sorted6[1], nodeList[3].get());
  EXPECT_EQ(sorted6[2], nodeList[4].get());
  EXPECT_EQ(sorted6[3], obj);

  // Set translate-z = 0 for obj and node 4.
  TransformOperations transform7;
  transform7.AppendTranslate(0, 0, 0);
  nodeList[4]->SetProperty(ClayAnimationPropertyType::kTransform, transform7,
                           false);
  obj->SetProperty(ClayAnimationPropertyType::kTransform, transform7, false);
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted7 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted7[0], nodeList[4].get());
  EXPECT_EQ(sorted7[1], nodeList[5].get());
  EXPECT_EQ(sorted7[2], nodeList[3].get());
  EXPECT_EQ(sorted7[3], obj);

  // Remove node 3.
  nodeList[1]->RemoveChild(nodeList[3].get());
  EXPECT_EQ(ChildrenPaintingOrderIsDirtyForTesting(nodeList[1].get()), true);
  const auto& sorted8 = GetSortedChildrenForTesting(nodeList[1].get());
  EXPECT_EQ(sorted8[0], nodeList[4].get());
  EXPECT_EQ(sorted8[1], nodeList[5].get());
  EXPECT_EQ(sorted8[2], obj);

  // Update root node‘s painting order shouldn't trigger a crash.
  TransformOperations transform8;
  transform8.AppendTranslate(0, 0, 1);
  auto* root = nodeList[0].get();
  root->SetProperty(ClayAnimationPropertyType::kTransform, transform8, false);
  root->SetPaintingOrder(1);
}

TEST_F_UI(BaseViewTest, OnBoundChange) {
  class MockBaseView : public BaseView {
   public:
    using BaseView::BaseView;
    MOCK_METHOD(void, OnBoundsChanged,
                (const FloatRect& old_bounds, const FloatRect& new_bounds),
                (override));
  };

  MockBaseView mock_view(std::make_unique<RenderContainer>(), page_.get());

  EXPECT_CALL(mock_view, OnBoundsChanged(::testing::_, ::testing::_)).Times(1);
  mock_view.SetBound(0, 0, 100, 100);
  ::testing::Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(mock_view, OnBoundsChanged(FloatRect(0, 0, 100, 100),
                                         FloatRect(0, 0, 200, 300)))
      .Times(1);
  mock_view.SetBound(0, 0, 200, 300);
  ::testing::Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(mock_view, OnBoundsChanged(::testing::_, ::testing::_)).Times(1);
  mock_view.SetX(10);
  ::testing::Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(mock_view, OnBoundsChanged(::testing::_, ::testing::_)).Times(1);
  mock_view.SetY(10);
  ::testing::Mock::VerifyAndClearExpectations(this);

  EXPECT_CALL(mock_view, OnBoundsChanged(::testing::_, ::testing::_)).Times(0);
  mock_view.SetBound(10, 10, 200, 300);
}

}  // namespace clay
