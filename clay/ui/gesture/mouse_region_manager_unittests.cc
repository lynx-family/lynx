// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/thread.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/gesture/mouse_region_manager.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

using ::testing::ElementsAre;

namespace clay {
namespace testing {

class MouseRegionManagerTest : public UITest {
 protected:
  std::vector<PointerEvent> CreateHoverPointer(float x, float y) {
    return {
        CreatePointer(-1, clay::PointerEvent::EventType::kHoverEvent, {x, y})};
  }
};

TEST_F_UI(MouseRegionManagerTest, EnterLeaveMouseRegion) {
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

  // the parent-child relation of views:
  //        view1
  //     ↙
  // root
  //     ↖
  //        view2 <- view3 <- view4

  auto& root = page_;
  // View type doesn't matter. All views in the region will be added in.
  BaseView* View1 = new View(1, root.get());
  BaseView* View2 = new View(2, root.get());
  BaseView* View3 = new View(3, root.get());
  BaseView* View4 = new View(4, root.get());
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

  std::vector<int> views_enter, views_leave;
  auto on_enter = [&views_enter](int view_id) {
    views_enter.push_back(view_id);
  };
  auto on_leave = [&views_leave](int view_id) {
    views_leave.push_back(view_id);
  };
  auto clear = [&views_enter, &views_leave] {
    views_enter.clear();
    views_leave.clear();
  };

  // register callbacks for mouse entering and leaving mouse regions
  auto* manager = root->mouse_region_manager();
  if (manager) {
    std::vector<BaseView*> views = {root.get(), View1, View2, View3, View4};
    for (size_t view_id = 0; view_id < views.size(); ++view_id) {
      auto* view = views[view_id];
      manager->RegisterEnterCallback(view, std::bind(on_enter, view_id));
      manager->RegisterLeaveCallback(view, std::bind(on_leave, view_id));
    }
  }

  // hover to view4
  root->DispatchPointerEvent(CreateHoverPointer(400, 400));
  EXPECT_THAT(views_leave, ElementsAre());
  EXPECT_THAT(views_enter, ElementsAre(0, 2, 3, 4));
  clear();

  // hover to view1
  root->DispatchPointerEvent(CreateHoverPointer(50, 100));
  EXPECT_THAT(views_leave, ElementsAre(4, 3, 2));
  EXPECT_THAT(views_enter, ElementsAre(1));
  clear();

  // hover to view3
  root->DispatchPointerEvent(CreateHoverPointer(150, 500));
  EXPECT_THAT(views_leave, ElementsAre(1));
  EXPECT_THAT(views_enter, ElementsAre(2, 3));
  clear();

  // hover to view4
  root->DispatchPointerEvent(CreateHoverPointer(400, 400));
  EXPECT_THAT(views_leave, ElementsAre());
  EXPECT_THAT(views_enter, ElementsAre(4));
  clear();

  // hover to view4
  root->DispatchPointerEvent(CreateHoverPointer(400, 450));
  EXPECT_THAT(views_leave, ElementsAre());
  EXPECT_THAT(views_enter, ElementsAre());
  clear();

  // hover to view3
  root->DispatchPointerEvent(CreateHoverPointer(200, 600));
  EXPECT_THAT(views_leave, ElementsAre(4));
  EXPECT_THAT(views_enter, ElementsAre());
  clear();
}
}  // namespace testing
}  // namespace clay
