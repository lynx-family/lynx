// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cmath>

#include "base/include/fml/thread.h"
#include "build/build_config.h"
#include "clay/shell/common/pointer_data_to_event.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/text/text_view.h"
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

TEST_F_UI(MouseRegionManagerTest, EventThroughTextRevealsUnderlyingRegion) {
  auto* background = new View(1, page_.get());
  auto* text = new TextView(2, page_.get());
  page_->AddChild(background);
  page_->AddChild(text);
  page_->SetBound(0, 0, 100, 100);
  background->SetBound(0, 0, 100, 100);
  text->SetBound(0, 0, 100, 100);
  text->SetEventThrough(true);

  bool entered_background = false;
  bool entered_text = false;
  auto* manager = page_->mouse_region_manager();
  ASSERT_NE(manager, nullptr);
  manager->RegisterEnterCallback(background,
                                 [&entered_background](const PointerEvent&) {
                                   entered_background = true;
                                 });
  manager->RegisterEnterCallback(
      text, [&entered_text](const PointerEvent&) { entered_text = true; });

  page_->DispatchPointerEvent(CreateHoverPointer(50, 50));

  EXPECT_TRUE(entered_background);
  EXPECT_FALSE(entered_text);
}

#if defined(OS_WIN) || defined(OS_MAC)
std::string BoundaryRecord(const std::string& event_name, int target,
                           int related_target) {
  return event_name + ":" + std::to_string(target) + ":" +
         std::to_string(related_target);
}
#endif

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

#if defined(OS_WIN) || defined(OS_MAC)
  PointerData data{};
  data.kind = PointerData::DeviceKind::kMouse;
  data.change = PointerData::Change::kRemove;
  data.physical_x = 200;
  data.physical_y = 600;
  PointerDataPacket packet(1);
  packet.SetPointerData(0, data);
  root->DispatchPointerEvent(GetEventsFromPointerDataPacket(&packet));
  EXPECT_THAT(views_leave, ElementsAre(3, 2, 0));
  EXPECT_THAT(views_enter, ElementsAre());
  clear();

  root->DispatchPointerEvent(GetEventsFromPointerDataPacket(&packet));
  EXPECT_THAT(views_leave, ElementsAre());
  EXPECT_THAT(views_enter, ElementsAre());

  root->DispatchPointerEvent(CreateHoverPointer(200, 600));
  EXPECT_THAT(views_leave, ElementsAre());
  EXPECT_THAT(views_enter, ElementsAre(0, 2, 3));
#endif
}

#if defined(OS_WIN) || defined(OS_MAC)
TEST_F_UI(MouseRegionManagerTest,
          DispatchesPointerBoundariesUsingLowestCommonAncestor) {
  auto& root = page_;
  auto* parent = new View(1, root.get());
  auto* child = new View(2, root.get());
  auto* sibling = new View(3, root.get());
  root->AddChild(parent);
  parent->AddChild(child);
  parent->AddChild(sibling);

  root->SetBound(0, 0, 1000, 1000);
  parent->SetBound(0, 0, 500, 500);
  child->SetBound(0, 0, 200, 200);
  sibling->SetBound(250, 0, 200, 200);
  parent->OnLayoutUpdated();
  child->OnLayoutUpdated();
  sibling->OnLayoutUpdated();

  std::vector<std::string> records;
  auto* manager = root->mouse_region_manager();
  pointer_event_callback_ = [&records](const std::string& event_name,
                                       int view_id, int, ClayPointerDeviceKind,
                                       bool, int, int, float, float, float,
                                       int64_t, int related_target_sign) {
    records.push_back(BoundaryRecord(event_name, view_id, related_target_sign));
  };

  PointerEvent event(PointerEvent::EventType::kHoverEvent);
  event.device = PointerEvent::DeviceType::kMouse;
  event.pointer_id = 10;
  event.device_id = 10;
  event.position = {50, 50};
  manager->HandlePointerEventBefore(root.get(), event);
  EXPECT_THAT(records, ElementsAre("pointerover:2:-1", "pointerenter:0:-1",
                                   "pointerenter:1:-1", "pointerenter:2:-1"));

  records.clear();
  event.position = {300, 50};
  manager->HandlePointerEventBefore(root.get(), event);
  EXPECT_THAT(records, ElementsAre("pointerout:2:3", "pointerleave:2:3",
                                   "pointerover:3:2", "pointerenter:3:2"));
}

TEST_F_UI(MouseRegionManagerTest,
          RetargetsTouchLikePointerAfterCapturedViewIsDetached) {
  auto& root = page_;
  auto* fallback = new View(2, root.get());
  auto* captured = new View(1, root.get());
  root->AddChild(fallback);
  root->AddChild(captured);

  root->SetBound(0, 0, 1000, 1000);
  fallback->SetBound(0, 0, 400, 400);
  captured->SetBound(0, 0, 400, 400);
  fallback->OnLayoutUpdated();
  captured->OnLayoutUpdated();

  std::vector<std::string> records;
  pointer_event_callback_ = [&records](const std::string& event_name,
                                       int view_id, int, ClayPointerDeviceKind,
                                       bool, int, int, float, float, float,
                                       int64_t, int) {
    records.push_back(event_name + ":" + std::to_string(view_id));
  };

  PointerEvent touch(PointerEvent::EventType::kDownEvent);
  touch.device = PointerEvent::DeviceType::kTouch;
  touch.pointer_id = 10;
  touch.device_id = 10;
  touch.position = {50, 50};
  touch.buttons = PointerEvent::MouseButton::kPrimary;
  root->DispatchPointerEvent({touch});
  EXPECT_THAT(records, ElementsAre("pointerover:1", "pointerenter:0",
                                   "pointerenter:1", "pointerdown:1"));

  records.clear();
  root->RemoveChild(captured);

  touch.type = PointerEvent::EventType::kMoveEvent;
  touch.position = {800, 800};
  root->DispatchPointerEvent({touch});
  EXPECT_THAT(records, ElementsAre("pointerover:0", "pointermove:0"));

  records.clear();
  touch.position = {50, 50};
  root->DispatchPointerEvent({touch});
  EXPECT_THAT(records, ElementsAre("pointerout:0", "pointerover:2",
                                   "pointerenter:2", "pointermove:2"));

  records.clear();
  touch.type = PointerEvent::EventType::kUpEvent;
  touch.buttons = 0;
  root->DispatchPointerEvent({touch});
  EXPECT_THAT(records, ElementsAre("pointerup:2", "pointerout:2",
                                   "pointerleave:2", "pointerleave:0"));

  captured->Destroy();
  delete captured;
}

TEST_F_UI(MouseRegionManagerTest, RefreshesStationaryPointerAfterLayout) {
  auto& root = page_;
  auto* fallback = new View(1, root.get());
  auto* top = new View(2, root.get());
  root->AddChild(fallback);
  root->AddChild(top);
  root->SetBound(0, 0, 1000, 1000);
  fallback->SetBound(0, 0, 200, 200);
  top->SetBound(0, 0, 200, 200);
  fallback->OnLayoutUpdated();
  top->OnLayoutUpdated();

  std::vector<std::string> records;
  pointer_event_callback_ = [&records](const std::string& event_name,
                                       int view_id, int, ClayPointerDeviceKind,
                                       bool, int, int, float, float, float,
                                       int64_t, int related_target_sign) {
    records.push_back(BoundaryRecord(event_name, view_id, related_target_sign));
  };

  PointerEvent event(PointerEvent::EventType::kHoverEvent);
  event.device = PointerEvent::DeviceType::kMouse;
  event.pointer_id = 10;
  event.position = {50, 50};
  auto* manager = root->mouse_region_manager();
  manager->HandlePointerEventBefore(root.get(), event);
  records.clear();

  top->SetBound(300, 300, 200, 200);
  Layout();
  top->OnLayoutUpdated();
  DoAnimation(32);

  EXPECT_THAT(records, ElementsAre("pointerout:2:1", "pointerleave:2:1",
                                   "pointerover:1:2", "pointerenter:1:2"));
}

TEST_F_UI(MouseRegionManagerTest,
          DoesNotRefreshImplicitlyCapturedStylusAfterLayout) {
  auto& root = page_;
  auto* fallback = new View(1, root.get());
  auto* captured = new View(2, root.get());
  root->AddChild(fallback);
  root->AddChild(captured);
  root->SetBound(0, 0, 1000, 1000);
  fallback->SetBound(0, 0, 200, 200);
  captured->SetBound(0, 0, 200, 200);
  fallback->OnLayoutUpdated();
  captured->OnLayoutUpdated();

  std::vector<std::string> records;
  pointer_event_callback_ = [&records](const std::string& event_name,
                                       int view_id, int, ClayPointerDeviceKind,
                                       bool, int, int, float, float, float,
                                       int64_t, int related_target_sign) {
    records.push_back(BoundaryRecord(event_name, view_id, related_target_sign));
  };

  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.device = PointerEvent::DeviceType::kStylus;
  event.pointer_id = 10;
  event.position = {50, 50};
  event.buttons = PointerEvent::MouseButton::kPrimary;
  auto* manager = root->mouse_region_manager();
  manager->HandlePointerEventBefore(root.get(), event);
  records.clear();

  captured->SetBound(300, 300, 200, 200);
  Layout();
  captured->OnLayoutUpdated();
  DoAnimation(32);
  EXPECT_THAT(records, ElementsAre());

  event.type = PointerEvent::EventType::kUpEvent;
  event.buttons = 0;
  manager->HandlePointerEventBefore(root.get(), event);
  manager->HandlePointerEventAfter(root.get(), event);
  EXPECT_THAT(records, ElementsAre("pointerout:2:1", "pointerleave:2:1",
                                   "pointerover:1:2", "pointerenter:1:2"));
}

TEST_F_UI(MouseRegionManagerTest, OrdersPointerLifecycleBeforeLegacyMouse) {
  auto& root = page_;
  auto* child = new View(1, root.get());
  root->AddChild(child);
  root->SetBound(0, 0, 1000, 1000);
  child->SetBound(0, 0, 400, 400);
  child->OnLayoutUpdated();
  root->SetAlignMouseEventWithW3C(true);

  std::vector<std::string> records;
  pointer_event_callback_ = [&records](const std::string& event_name,
                                       int view_id, int, ClayPointerDeviceKind,
                                       bool is_primary, int button, int buttons,
                                       float, float, float, int64_t, int) {
    records.push_back("pointer:" + event_name + ":" + std::to_string(view_id) +
                      ":" + std::to_string(button) + ":" +
                      std::to_string(buttons) + ":" +
                      (is_primary ? "primary" : "secondary"));
  };
  mouse_event_callback_ = [&records](const std::string& event_name,
                                     int view_id) {
    records.push_back("mouse:" + event_name + ":" + std::to_string(view_id));
  };

  PointerEvent event(PointerEvent::EventType::kAddEvent);
  event.device = PointerEvent::DeviceType::kMouse;
  event.pointer_id = 10;
  event.device_id = 10;
  event.position = {50, 50};
  root->DispatchPointerEvent({event});
  EXPECT_THAT(records, ElementsAre("pointer:pointerover:1:-1:0:primary",
                                   "pointer:pointerenter:0:-1:0:primary",
                                   "pointer:pointerenter:1:-1:0:primary"));

  records.clear();
  event.type = PointerEvent::EventType::kHoverEvent;
  root->DispatchPointerEvent({event});
  EXPECT_THAT(records, ElementsAre("pointer:pointermove:1:-1:0:primary",
                                   "mouse:mouseenter:1", "mouse:mousemove:1"));

  records.clear();
  event.type = PointerEvent::EventType::kDownEvent;
  event.buttons = PointerEvent::MouseButton::kPrimary;
  root->DispatchPointerEvent({event});
  EXPECT_THAT(records, ElementsAre("pointer:pointerdown:1:0:1:primary",
                                   "mouse:mousedown:1"));

  records.clear();
  event.type = PointerEvent::EventType::kMoveEvent;
  event.buttons = PointerEvent::MouseButton::kPrimary |
                  PointerEvent::MouseButton::kSecondary;
  root->DispatchPointerEvent({event});
  EXPECT_THAT(records, ElementsAre("pointer:pointermove:1:2:3:primary",
                                   "mouse:mousemove:1"));

  records.clear();
  event.type = PointerEvent::EventType::kUpEvent;
  event.buttons = 0;
  root->DispatchPointerEvent({event});
  EXPECT_THAT(records, ElementsAre("pointer:pointerup:1:0:0:primary",
                                   "mouse:mouseup:1", "mouse:mouseclick:1"));

  records.clear();
  event.type = PointerEvent::EventType::kRemoveEvent;
  root->DispatchPointerEvent({event});
  EXPECT_THAT(records, ElementsAre("pointer:pointerout:1:-1:0:primary",
                                   "pointer:pointerleave:1:-1:0:primary",
                                   "pointer:pointerleave:0:-1:0:primary",
                                   "mouse:mouseleave:1"));
}

TEST_F_UI(MouseRegionManagerTest, StylusPreservesLegacyTouchEvents) {
  auto& root = page_;
  auto* child = new View(1, root.get());
  root->AddChild(child);
  root->SetBound(0, 0, 1000, 1000);
  child->SetBound(0, 0, 400, 400);
  child->OnLayoutUpdated();

  find_view_by_id_callback_ = [child](int view_id) {
    return view_id == child->id() ? child : nullptr;
  };

  std::vector<std::string> records;
  touch_event_callback_ = [&records](const std::string& event_name,
                                     int view_id) {
    records.push_back(event_name + ":" + std::to_string(view_id));
  };

  const int button_combinations[] = {
      PointerEvent::kPrimary, PointerEvent::kSecondary,
      PointerEvent::kPrimary | PointerEvent::kSecondary};
  for (auto device : {PointerEvent::kStylus, PointerEvent::kInvertedStylus}) {
    for (int buttons : button_combinations) {
      SCOPED_TRACE(device);
      SCOPED_TRACE(buttons);
      records.clear();
      PointerEvent event(PointerEvent::EventType::kDownEvent);
      event.device = device;
      event.pointer_id = 10;
      event.position = {50, 50};
      event.buttons = buttons;
      root->DispatchPointerEvent({event});
      event.type = PointerEvent::EventType::kUpEvent;
      event.buttons = 0;
      root->DispatchPointerEvent({event});
      EXPECT_THAT(records, ElementsAre("touchstart:1", "touchend:1", "tap:1"));
      event.type = PointerEvent::EventType::kRemoveEvent;
      root->DispatchPointerEvent({event});
    }
  }
}

TEST_F_UI(MouseRegionManagerTest, SharesPrimaryStateAcrossPenDeviceKinds) {
  auto& root = page_;
  root->SetBound(0, 0, 1000, 1000);

  std::vector<std::pair<int, bool>> records;
  pointer_event_callback_ = [&records](const std::string& event_name, int,
                                       int pointer_id, ClayPointerDeviceKind,
                                       bool is_primary, int, int, float, float,
                                       float, int64_t, int) {
    if (event_name == "pointerover") {
      records.emplace_back(pointer_id, is_primary);
    }
  };

  PointerEvent stylus(PointerEvent::EventType::kAddEvent);
  stylus.device = PointerEvent::DeviceType::kStylus;
  stylus.pointer_id = 101;
  stylus.device_id = 1;
  stylus.position = {50, 50};
  root->DispatchPointerEvent({stylus});

  PointerEvent inverted = stylus;
  inverted.device = PointerEvent::DeviceType::kInvertedStylus;
  inverted.pointer_id = 102;
  inverted.device_id = 2;
  root->DispatchPointerEvent({inverted});

  stylus.type = PointerEvent::EventType::kRemoveEvent;
  root->DispatchPointerEvent({stylus});
  stylus.type = PointerEvent::EventType::kAddEvent;
  stylus.pointer_id = 103;
  stylus.device_id = 3;
  root->DispatchPointerEvent({stylus});

  inverted.type = PointerEvent::EventType::kRemoveEvent;
  root->DispatchPointerEvent({inverted});
  stylus.type = PointerEvent::EventType::kRemoveEvent;
  root->DispatchPointerEvent({stylus});
  stylus.type = PointerEvent::EventType::kAddEvent;
  stylus.pointer_id = 104;
  stylus.device_id = 4;
  root->DispatchPointerEvent({stylus});

  EXPECT_THAT(records, ElementsAre(std::pair<int, bool>{1, true},
                                   std::pair<int, bool>{2, false},
                                   std::pair<int, bool>{3, false},
                                   std::pair<int, bool>{4, true}));
}

TEST_F_UI(MouseRegionManagerTest, PenHoverDoesNotChangeLegacyMouseRegion) {
  auto* left = new View(1, page_.get());
  auto* right = new View(2, page_.get());
  page_->AddChild(left);
  page_->AddChild(right);
  page_->SetBound(0, 0, 400, 200);
  left->SetBound(0, 0, 200, 200);
  right->SetBound(200, 0, 200, 200);
  find_view_by_id_callback_ = [left, right](int id) {
    return id == 1 ? left : id == 2 ? right : nullptr;
  };
  std::vector<std::string> legacy;
  mouse_event_callback_ = [&legacy](const std::string& name, int id) {
    legacy.push_back(name + ":" + std::to_string(id));
  };
  touch_event_callback_ = mouse_event_callback_;
  std::vector<std::string> pointers;
  pointer_event_callback_ = [&pointers](const std::string& name, int id, int,
                                        ClayPointerDeviceKind, bool, int, int,
                                        float, float, float, int64_t, int) {
    if (id == 2) pointers.push_back(name);
  };
  PointerEvent mouse(PointerEvent::EventType::kHoverEvent);
  mouse.device = PointerEvent::DeviceType::kMouse;
  mouse.position = {50, 50};
  page_->DispatchPointerEvent({mouse});
  legacy.clear();
  pointers.clear();

  auto pen = mouse;
  pen.device = PointerEvent::DeviceType::kStylus;
  pen.dispatch_mode = PointerEvent::DispatchMode::kPenWithTouchCompatibility;
  pen.device_id = 7;
  pen.position = {250, 50};
  page_->DispatchPointerEvent({pen});
  EXPECT_THAT(pointers,
              ElementsAre("pointerover", "pointerenter", "pointermove"));
  EXPECT_TRUE(legacy.empty());
  pen.type = PointerEvent::EventType::kRemoveEvent;
  page_->DispatchPointerEvent({pen});
  EXPECT_TRUE(legacy.empty());
  page_->DispatchPointerEvent({mouse});
  for (const auto& record : legacy) {
    EXPECT_NE(record, "mouseenter:1");
    EXPECT_NE(record, "mouseleave:1");
  }
}

TEST_F_UI(MouseRegionManagerTest, PenCancellationEndsLegacyTouchOnlyOnce) {
  auto* child = new View(1, page_.get());
  page_->AddChild(child);
  page_->SetBound(0, 0, 400, 400);
  child->SetBound(0, 0, 400, 400);
  find_view_by_id_callback_ = [child](int id) {
    return id == 1 ? child : nullptr;
  };
  std::vector<std::string> touches;
  touch_event_callback_ = [&touches](const std::string& name, int) {
    touches.push_back(name);
  };
  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.device = PointerEvent::DeviceType::kStylus;
  event.dispatch_mode = PointerEvent::DispatchMode::kPenWithTouchCompatibility;
  event.pointer_id = 10;
  event.device_id = 7;
  event.position = {50, 50};
  event.buttons = PointerEvent::kPrimary;
  page_->DispatchPointerEvent({event});
  event.type = PointerEvent::EventType::kCancel;
  event.buttons = 0;
  page_->DispatchPointerEvent({event});
  event.type = PointerEvent::EventType::kRemoveEvent;
  page_->DispatchPointerEvent({event});
  EXPECT_THAT(touches, ElementsAre("touchstart", "touchcancel"));
}

TEST_F_UI(MouseRegionManagerTest, PenContactMatchesLegacyTouchLifecycle) {
  auto* child = new View(1, page_.get());
  page_->AddChild(child);
  page_->SetBound(0, 0, 400, 400);
  child->SetBound(0, 0, 400, 400);
  find_view_by_id_callback_ = [child](int id) {
    return id == 1 ? child : nullptr;
  };
  std::vector<std::string> legacy;
  mouse_event_callback_ = [&legacy](const std::string& name, int id) {
    legacy.push_back(name + ":" + std::to_string(id));
  };
  touch_event_callback_ = mouse_event_callback_;

  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.device = PointerEvent::DeviceType::kTouch;
  event.pointer_id = 10;
  event.position = {50, 50};
  event.buttons = PointerEvent::MouseButton::kPrimary;
  page_->DispatchPointerEvent({event});
  event.type = PointerEvent::EventType::kUpEvent;
  event.buttons = 0;
  page_->DispatchPointerEvent({event});
  event.type = PointerEvent::EventType::kCancel;
  event.position = {};
  page_->DispatchPointerEvent({event});
  const auto baseline = legacy;
  ASSERT_THAT(baseline, ::testing::Contains("tap:1"));

  const int button_combinations[] = {
      PointerEvent::kPrimary, PointerEvent::kSecondary,
      PointerEvent::kPrimary | PointerEvent::kSecondary};
  for (int buttons : button_combinations) {
    SCOPED_TRACE(buttons);
    legacy.clear();
    std::vector<std::string> pointers;
    pointer_event_callback_ = [&pointers](const std::string& name, int id, int,
                                          ClayPointerDeviceKind, bool, int, int,
                                          float, float, float, int64_t, int) {
      if (id == 1) pointers.push_back(name);
    };
    event.device = PointerEvent::DeviceType::kStylus;
    event.dispatch_mode =
        PointerEvent::DispatchMode::kPenWithTouchCompatibility;
    event.device_id = 7;
    event.type = PointerEvent::EventType::kDownEvent;
    event.position = {50, 50};
    event.buttons = buttons;
    page_->DispatchPointerEvent({event});
    event.type = PointerEvent::EventType::kUpEvent;
    event.buttons = 0;
    page_->DispatchPointerEvent({event});
    EXPECT_EQ(legacy, baseline);
    EXPECT_THAT(pointers, ::testing::Contains("pointerup"));
    EXPECT_THAT(pointers, ::testing::Not(::testing::Contains("pointerleave")));
    pointers.clear();
    event.type = PointerEvent::EventType::kHoverEvent;
    page_->DispatchPointerEvent({event});
    EXPECT_THAT(pointers, ElementsAre("pointermove"));
    EXPECT_EQ(legacy, baseline);
    event.type = PointerEvent::EventType::kRemoveEvent;
    page_->DispatchPointerEvent({event});
    EXPECT_THAT(pointers, ::testing::Contains("pointerleave"));
    EXPECT_EQ(legacy, baseline);
    pointer_event_callback_ = nullptr;
  }
}

TEST_F_UI(MouseRegionManagerTest, ReportsPointerTimestampsInMilliseconds) {
  auto& root = page_;
  root->SetBound(0, 0, 1000, 1000);

  std::vector<std::pair<std::string, int64_t>> records;
  pointer_event_callback_ =
      [&records](const std::string& event_name, int, int, ClayPointerDeviceKind,
                 bool, int, int, float, float, float, int64_t timestamp,
                 int) { records.emplace_back(event_name, timestamp); };

  PointerEvent event(PointerEvent::EventType::kAddEvent);
  event.device = PointerEvent::DeviceType::kMouse;
  event.position = {50, 50};
  event.timestamp = 1234000;
  root->DispatchPointerEvent({event});

  records.clear();
  event.type = PointerEvent::EventType::kHoverEvent;
  event.timestamp = 5678000;
  root->DispatchPointerEvent({event});
  EXPECT_THAT(records, ElementsAre(std::pair<std::string, int64_t>{
                           "pointermove", 5678}));
}

TEST_F_UI(MouseRegionManagerTest, ReportsPointerContactData) {
  auto& root = page_;
  root->SetBound(0, 0, 1000, 1000);

  float width = 0.f;
  float height = 0.f;
  float pressure = 0.f;
  pointer_event_callback_ = [&](const std::string& event_name, int, int,
                                ClayPointerDeviceKind, bool, int, int,
                                float event_width, float event_height,
                                float event_pressure, int64_t, int) {
    if (event_name == "pointermove") {
      width = event_width;
      height = event_height;
      pressure = event_pressure;
    }
  };

  PointerEvent event(PointerEvent::EventType::kHoverEvent);
  event.device = PointerEvent::DeviceType::kMouse;
  event.position = {50, 50};
  event.buttons = PointerEvent::MouseButton::kPrimary;
  event.radius_major = 3.0;
  event.radius_minor = 2.0;
  event.pressure = 0.6;
  event.pressure_min = 0.2;
  event.pressure_max = 1.0;
  root->DispatchPointerEvent({event});
  EXPECT_FLOAT_EQ(width, 4.f);
  EXPECT_FLOAT_EQ(height, 6.f);
  EXPECT_FLOAT_EQ(pressure, 0.5f);

  event.orientation = std::acos(0.0);
  root->DispatchPointerEvent({event});
  EXPECT_FLOAT_EQ(width, 6.f);
  EXPECT_FLOAT_EQ(height, 4.f);

  event.radius_minor = 0.0;
  root->DispatchPointerEvent({event});
  EXPECT_FLOAT_EQ(width, 6.f);
  EXPECT_FLOAT_EQ(height, 6.f);

  event.pressure = event.pressure_min;
  root->DispatchPointerEvent({event});
  EXPECT_FLOAT_EQ(pressure, 0.f);

  event.radius_major = 0.0;
  event.radius_minor = 0.0;
  event.pressure = 1.0;
  event.pressure_min = 1.0;
  event.pressure_max = 1.0;
  root->DispatchPointerEvent({event});
  EXPECT_FLOAT_EQ(width, 1.f);
  EXPECT_FLOAT_EQ(height, 1.f);
  EXPECT_FLOAT_EQ(pressure, 0.5f);

  event.buttons = 0;
  root->DispatchPointerEvent({event});
  EXPECT_FLOAT_EQ(pressure, 0.f);
}

TEST_F_UI(MouseRegionManagerTest, CancelPreservesPressureAcrossBoundaryEvents) {
  page_->SetBound(0, 0, 1000, 1000);
  for (bool measured : {false, true}) {
    std::vector<std::string> names;
    std::vector<float> pressures;
    std::vector<int> buttons;
    pointer_event_callback_ =
        [&](const std::string& name, int, int, ClayPointerDeviceKind, bool, int,
            int button_state, float, float, float pressure, int64_t, int) {
          if (name == "pointerover" || name == "pointerenter") {
            return;
          }
          names.push_back(name);
          pressures.push_back(pressure);
          buttons.push_back(button_state);
        };

    PointerEvent event(PointerEvent::EventType::kDownEvent);
    event.device = PointerEvent::DeviceType::kStylus;
    event.pointer_id = 10;
    event.device_id = 10;
    event.position = {50, 50};
    event.buttons = PointerEvent::MouseButton::kPrimary;
    event.pressure = measured ? 4.0 : 1.0;
    event.pressure_min = 1.0;
    event.pressure_max = measured ? 5.0 : 1.0;
    page_->DispatchPointerEvent({event});

    event.type = PointerEvent::EventType::kCancel;
    event.pressure = 0.0;
    page_->DispatchPointerEvent({event});

    const float expected = measured ? 0.75f : 0.5f;
    EXPECT_THAT(names, ElementsAre("pointerdown", "pointercancel", "pointerout",
                                   "pointerleave"));
    EXPECT_THAT(pressures, ElementsAre(expected, expected, expected, expected));
    EXPECT_THAT(buttons, ElementsAre(1, 0, 0, 0));
  }
  pointer_event_callback_ = nullptr;
}
#endif
}  // namespace testing
}  // namespace clay
