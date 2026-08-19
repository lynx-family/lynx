// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

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
  manager->RegisterLeaveCallback(View3, [&](const PointerEvent& event) {
    on_leave(3);
    EXPECT_EQ(event.type, PointerEvent::EventType::kCancel);
    EXPECT_EQ(event.position, FloatPoint());
  });
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
TEST_F_UI(MouseRegionManagerTest, MouseAddDoesNotEnterLegacyRegions) {
  page_->SetBound(0, 0, 100, 100);
  page_->SetAlignMouseEventWithW3C(false);
  std::vector<int> buttons;
  int hover_count = 0;
  auto* manager = page_->mouse_region_manager();
  manager->RegisterEnterCallback(page_.get(), [&](const PointerEvent& event) {
    buttons.push_back(event.buttons);
  });
  manager->RegisterHoverCallback(page_.get(),
                                 [&](const PointerEvent&) { ++hover_count; });
  PointerData data{};
  data.kind = PointerData::DeviceKind::kMouse;
  data.change = PointerData::Change::kAdd;
  data.physical_x = data.physical_y = 50;
  PointerDataPacket packet(2);
  packet.SetPointerData(0, data);
  data.change = PointerData::Change::kDown;
  data.buttons = PointerEvent::kPrimary;
  packet.SetPointerData(1, data);
  page_->DispatchPointerEvent(GetEventsFromPointerDataPacket(&packet));
  EXPECT_THAT(buttons, ElementsAre(PointerEvent::kPrimary));
  EXPECT_EQ(hover_count, 0);
}

TEST_F_UI(MouseRegionManagerTest, BatchedMousePreservesLegacyOrder) {
  page_->SetBound(0, 0, 400, 200);
  for (int id : {1, 2}) {
    auto* view = new View(id, page_.get());
    page_->AddChild(view);
    view->SetBound((id - 1) * 200, 0, 200, 200);
  }
  std::vector<std::string> records;
  mouse_event_callback_ = [&](const std::string& name, int id) {
    records.push_back(name + ":" + std::to_string(id));
  };
  for (bool enabled : {false, true}) {
    SCOPED_TRACE(enabled);
    page_->SetEnablePointerEvents(enabled);
    page_->mouse_region_manager()->Reset();
    records.clear();
    page_->SetAlignMouseEventWithW3C(true);
    PointerEvent first(PointerEvent::EventType::kHoverEvent);
    first.timestamp = enabled ? 800000 : 400000;
    first.device = PointerEvent::kMouse;
    first.position = {50, 50};
    auto second = first;
    second.position = {250, 50};
    page_->DispatchPointerEvent({first, second});
    EXPECT_THAT(records,
                ElementsAre("mouseenter:1", "mouseleave:1", "mouseenter:2",
                            "mousemove:1", "mousemove:2"));

    page_->SetAlignMouseEventWithW3C(false);
    first.type = PointerEvent::EventType::kDownEvent;
    first.position = second.position;
    first.buttons = PointerEvent::kPrimary;
    page_->DispatchPointerEvent({first});
    records.clear();
    first.type = PointerEvent::EventType::kMoveEvent;
    second = first;
    second.type = PointerEvent::EventType::kUpEvent;
    second.buttons = 0;
    page_->DispatchPointerEvent({first, second});
    EXPECT_THAT(records,
                ElementsAre("mouseover:2", "mouseover:2", "mousemove:2",
                            "mouseup:2", "mouseclick:2"));
  }
}

TEST_F_UI(MouseRegionManagerTest, BatchedPenMatchesSequentialLegacyDispatch) {
  page_->SetBound(0, 0, 400, 200);
  auto* child = new View(1, page_.get());
  page_->AddChild(child);
  child->SetBound(0, 0, 400, 200);
  find_view_by_id_callback_ = [child](int id) -> BaseView* {
    return id == child->id() ? child : nullptr;
  };
  std::vector<std::string> records;
  mouse_event_callback_ = [&](const std::string& name, int) {
    records.push_back(name);
  };
  touch_event_callback_ = [&](const std::string& name, int) {
    records.push_back(name);
  };
  const auto dispatch = [&](bool batched) {
    PointerEvent event(PointerEvent::EventType::kDownEvent);
    event.device = PointerEvent::kStylus;
    event.dispatch_mode =
        PointerEvent::DispatchMode::kPenWithTouchCompatibility;
    event.position = {50, 50};
    event.buttons = PointerEvent::kPrimary;
    page_->DispatchPointerEvent({event});
    records.clear();
    event.type = PointerEvent::EventType::kMoveEvent;
    auto up = event;
    up.type = PointerEvent::EventType::kUpEvent;
    up.buttons = 0;
    if (batched) {
      page_->DispatchPointerEvent({event, up});
    } else {
      page_->DispatchPointerEvent({event});
      page_->DispatchPointerEvent({up});
    }
    return records;
  };
  for (bool enabled : {false, true}) {
    SCOPED_TRACE(enabled);
    page_->SetEnablePointerEvents(enabled);
    const auto sequential = dispatch(false);
    EXPECT_THAT(sequential, ElementsAre("mouseover", "touchmove", "mouseover",
                                        "touchend", "tap", "mouseleave"));
    EXPECT_EQ(dispatch(true), sequential);
  }
}

TEST_F_UI(MouseRegionManagerTest, DisabledPointerEventsSkipNewHitTesting) {
  class CountingView : public View {
   public:
    using View::View;
    BaseView* GetTopViewToAcceptEvent(const FloatPoint& point,
                                      FloatPoint* relative,
                                      int platform_try_hit_id = -1) override {
      ++queries;
      return View::GetTopViewToAcceptEvent(point, relative,
                                           platform_try_hit_id);
    }
    int queries = 0;
  };
  page_->SetBound(0, 0, 200, 200);
  page_->SetAlignMouseEventWithW3C(true);
  auto* child = new CountingView(1, page_.get());
  page_->AddChild(child);
  child->SetBound(0, 0, 200, 200);
  std::vector<std::string> pointers;
  pointer_event_callback_ =
      [&](const std::string& name, int, int, ClayPointerDeviceKind, bool, int,
          int, float, float, float, int64_t, int) { pointers.push_back(name); };
  PointerEvent event(PointerEvent::EventType::kHoverEvent);
  event.device = PointerEvent::kMouse;
  event.pointer_id = event.device_id = 7;
  event.position = {50, 50};
  page_->DispatchPointerEvent({event});
  EXPECT_EQ(child->queries, 2);
  EXPECT_TRUE(pointers.empty());
  EXPECT_EQ(page_->mouse_region_manager()->GetLastPointerEvent(event), nullptr);

  FloatPoint relative;
  child->SetAttribute("pointer-events", Value(uint32_t{1}));
  EXPECT_EQ(page_->GetTopViewToAcceptEvent(event.position, &relative),
            page_.get());
  child->SetAttribute("pointer-events", Value(uint32_t{0}));
  EXPECT_EQ(page_->GetTopViewToAcceptEvent(event.position, &relative), child);

  page_->SetEnablePointerEvents(true);
  child->queries = 0;
  page_->DispatchPointerEvent({event});
  EXPECT_EQ(child->queries, 4);
  EXPECT_THAT(pointers, ElementsAre("pointerover", "pointerenter",
                                    "pointerenter", "pointermove"));
  EXPECT_NE(page_->mouse_region_manager()->GetLastPointerEvent(event), nullptr);
  pointers.clear();
  std::vector<std::string> mouse;
  mouse_event_callback_ = [&](const std::string& name, int) {
    mouse.push_back(name);
  };
  child->SetBound(300, 0, 200, 200);
  page_->SetEnablePointerEvents(false);
  DoAnimation(20);
  EXPECT_TRUE(pointers.empty());
  EXPECT_EQ(page_->mouse_region_manager()->GetLastPointerEvent(event), nullptr);
  child->SetBound(0, 0, 200, 200);
  child->queries = 0;
  page_->DispatchPointerEvent({event});
  EXPECT_EQ(child->queries, 2);
  EXPECT_TRUE(pointers.empty());
  EXPECT_THAT(mouse, ElementsAre("mousemove"));
}

TEST_F_UI(MouseRegionManagerTest, OrdersPointerLifecycleBeforeLegacyMouse) {
  auto& root = page_;
  root->SetEnablePointerEvents(true);
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
#endif
}  // namespace testing
}  // namespace clay
