// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/focus/focus_isolate.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/testing/ui_test.h"

namespace clay {

constexpr int kIsolateLeftId = 1;
constexpr int kIsolateRightId = 2;
constexpr int kIsolateNested = 3;

class FocusIsolateTest : public UITest {
 protected:
  void UISetUp() override {
    //                  isolate-left
    //                   with               isolate-right
    //                  2 child
    //                 ┌──────────┐        ┌──────────┐
    // ┌─────────┐     │┼────────┼│        │┼────────┼│
    // │         │     ││        ││        ││        ││
    // │  1-1    │     ││  2-1   ││        ││  3-1   ││
    // │         │     ││        ││        ││        ││
    // │         │     ││        ││        ││        ││
    // └─────────┘     ├┴────────┴┤        │┼────────┼│
    //                 │          │        ├──────────┤
    //                 │          │        │          │
    //                 │          │        │          │
    //                 ├┬────────┬┤        ├┬────────┬┤
    //                 ││        ││        ││        ││
    //                 ││ 2-2    ││        ││        ││
    //                 ││        ││        ││  3-2   ││
    //                 ││        ││        ││        ││
    //                 │┼────────┼│        │┼────────┼│
    //                 └──────────┘        └──────────┘

    page_->AddChild(CreateFocusableView({}, {100, 100}, "1-1"));

    isolate_left_.reset(
        CreateFocusIsolate(kIsolateLeftId, {200, 0}, {100, 300}));
    isolate_left_->AddChild(CreateFocusableView({}, {100, 100}, "2-1"));
    isolate_left_->AddChild(CreateFocusableView({0, 200}, {100, 100}, "2-2"));
    page_->AddChild(isolate_left_.get());

    isolate_right_.reset(
        CreateFocusIsolate(kIsolateRightId, {400, 0}, {100, 100}));
    page_->AddChild(isolate_right_.get());
    isolate_right_->AddChild(CreateFocusableView({}, {100, 100}, "3-1"));
    isolate_right_->AddChild(CreateFocusableView({0, 200}, {100, 100}, "3-2"));

    custom_event_callback_ = [this](int id, const char* event_name,
                                    clay::Value::Map map) {
      if (strcmp(event_name, "focusescape") == 0 ||
          strcmp(event_name, "focusenter") == 0) {
        events_.emplace_back(event_name, id);
      }
    };
  }

  void UITearDown() override {
    isolate_left_->DestroyAllChildren();
    isolate_right_->DestroyAllChildren();
    isolate_left_ = nullptr;
    isolate_right_ = nullptr;
  }

  void AssignFocusFor(const std::string& id) { ViewForId(id)->RequestFocus(); }

  BaseView* GetCurrentFocus() {
    return static_cast<BaseView*>(
        page_->GetFocusManager()->GetLeafFocusedNode());
  }

 protected:
  FocusIsolate* CreateFocusIsolate(int id, const FloatPoint& pos,
                                   const FloatSize& size) {
    auto* view = new FocusIsolate(id, page_.get());
    view->SetBound(pos.x(), pos.y(), size.width(), size.height());
    return view;
  }

  View* CreateFocusableView(const FloatPoint& pos, const FloatSize& size,
                            const std::string& id_selector) {
    auto* view = new View(-1, page_.get());
    view->SetBound(pos.x(), pos.y(), size.width(), size.height());
    view->SetFocusable(true);
    view->SetIdSelector(id_selector);
    return view;
  }

  std::unique_ptr<FocusIsolate> isolate_left_;
  std::unique_ptr<FocusIsolate> isolate_right_;

  std::vector<std::pair<std::string, int>> events_;
};

using ::testing::ElementsAre;

#define MOVE_AND_CHECK_FOCUS(dir, id_selector)             \
  {                                                        \
    NavigateByDirection(FocusManager::Direction::dir);     \
    BaseView* focused_view = GetCurrentFocus();            \
    ASSERT_TRUE(!!focused_view) << "focus has lost";       \
    EXPECT_EQ(id_selector, focused_view->GetIdSelector()); \
  }

#define PAIR(e, i) std::pair<std::string, int>(e, i)

#if !defined(OS_WIN) && !defined(OS_MAC) && !defined(OS_HARMONY)
TEST_F_UI(FocusIsolateTest, NotAllowEscape) {
  AssignFocusFor("2-1");
  // Not allow to leave isolate
  MOVE_AND_CHECK_FOCUS(kRight, "2-1");

  MOVE_AND_CHECK_FOCUS(kDown, "2-2");

  // trigger escape
  EXPECT_THAT(events_, ElementsAre(PAIR("focusescape", kIsolateLeftId)));
}

TEST_F_UI(FocusIsolateTest, AllowEscape) {
  AssignFocusFor("2-1");
  isolate_left_->allow_escape_ = true;

  // isolate-right hasn't set any isolate, so focus still remain
  MOVE_AND_CHECK_FOCUS(kRight, "2-1");
  EXPECT_THAT(events_, ElementsAre(PAIR("focusescape", kIsolateLeftId),
                                   PAIR("focusenter", kIsolateRightId)));

  MOVE_AND_CHECK_FOCUS(kDown, "2-2");

  // Test save last focus child.
  isolate_left_->save_last_focus_child_ = true;
  isolate_left_->preferred_child_ = "2-1";
  MOVE_AND_CHECK_FOCUS(kLeft, "1-1");

  // Back to 2-2 although preferred 2-1 as priority preset.
  MOVE_AND_CHECK_FOCUS(kRight, "2-2");

  isolate_left_->save_last_focus_child_ = false;
  isolate_right_->preferred_child_ = "3-2";
  isolate_right_->allow_escape_ = true;
  // isolate-left's saved focus child will be cleared.
  MOVE_AND_CHECK_FOCUS(kRight, "3-2");

  // Back to isolate-left, preferred 2-1.
  MOVE_AND_CHECK_FOCUS(kLeft, "2-1");
}

#endif

TEST_F_UI(FocusIsolateTest, NestedScope) {
  // 1. Nested Isolate
  // Add a nested focus-isolate into 2-1
  auto* inner_isolate = CreateFocusIsolate(kIsolateNested, {}, {50, 50});
  ViewForId("2-1")->AddChild(inner_isolate);
  auto* inner_view = CreateFocusableView({10, 10}, {30, 30}, "2-1-1");
  inner_isolate->AddChild(inner_view);

  // Simulate setFocus directly to a specific node.
  inner_view->RequestFocus();
  EXPECT_EQ(inner_isolate->last_focused_view_, inner_view);
  EXPECT_NE(isolate_left_->last_focused_view_, inner_view);

  // 2. Nested Scope
  ScrollView* scroll_view = new ScrollView(-1, page_.get());
  ViewForId("2-2")->AddChild(scroll_view);
  scroll_view->SetBound(0, 0, 50, 50);
  auto* inner_view2 = CreateFocusableView({10, 10}, {30, 30}, "2-1-1");
  scroll_view->AddChild(inner_view2, 0);
  inner_view2->RequestFocus();
  EXPECT_EQ(isolate_left_->last_focused_view_, inner_view2);
}

// Test current leaf focus node is global unique no matter in which isolate.
TEST_F_UI(FocusIsolateTest, FocusNodeIsUnique) {
  AssignFocusFor("1-1");
  EXPECT_EQ(GetCurrentFocus(), ViewForId("1-1"));
  AssignFocusFor("2-1");
  EXPECT_EQ(GetCurrentFocus(), ViewForId("2-1"));
  AssignFocusFor("3-1");
  EXPECT_EQ(GetCurrentFocus(), ViewForId("3-1"));

  EXPECT_FALSE(ViewForId("1-1")->IsFocused());
  EXPECT_FALSE(ViewForId("2-1")->IsFocused());
  EXPECT_TRUE(ViewForId("3-1")->IsFocused());

  // What if focus-isolate is setFocus
  isolate_left_->RequestFocus();

  EXPECT_FALSE(ViewForId("3-1")->IsFocused());
}

#if !defined(OS_WIN) && !defined(OS_MAC) && !defined(OS_HARMONY)
// Test if can restore the last focused child across focus scope.
TEST_F_UI(FocusIsolateTest, AutoRestoreFocusAcrossScope) {
  // Focus node is nested in a scroll-view, which has a focus scope.
  isolate_left_->allow_escape_ = true;

  ScrollView* scroll_view = new ScrollView(-1, page_.get());
  ViewForId("2-1")->AddChild(scroll_view);
  scroll_view->SetBound(0, 0, 50, 50);
  auto* inner_view = CreateFocusableView({10, 10}, {30, 30}, "2-1-1");
  scroll_view->AddChild(inner_view, 0);
  inner_view->RequestFocus();

  MOVE_AND_CHECK_FOCUS(kLeft, "1-1");

  // Destroy the current focus node
  scroll_view->RemoveChild(inner_view);
  delete inner_view;

  isolate_left_->preferred_child_ = "2-2";
  MOVE_AND_CHECK_FOCUS(kRight, "2-2");
}

#endif

#undef PAIR

}  // namespace clay
