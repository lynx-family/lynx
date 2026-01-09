// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/thread.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/event/focus_manager.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace {
View* AddView(int id, PageView* root, float left, float top, float width,
              float height) {
  View* childView = new View(id, root);
  childView->SetX(left);
  childView->SetY(top);
  childView->SetWidth(width);
  childView->SetHeight(height);
  root->AddChild(childView);
  childView->SetFocusable(true);
  return childView;
}

[[maybe_unused]] View* AddViewToParent(int id, PageView* root, BaseView* parent,
                                       float left, float top, float width,
                                       float height) {
  View* childView = new View(id, root);
  childView->SetX(left);
  childView->SetY(top);
  childView->SetWidth(width);
  childView->SetHeight(height);
  parent->AddChild(childView);
  return childView;
}
}  // namespace

TEST(FocusManagerTest, SortViews) {
  auto thread = std::make_unique<fml::Thread>("ui");
  std::unique_ptr<PageView> root =
      std::make_unique<PageView>(0, nullptr, thread->GetTaskRunner());

  //  view1               view2
  //              view3
  //                      view4
  //  view5 view6

  AddView(1, root.get(), 100, 100, 100, 100);
  AddView(2, root.get(), 700, 100, 100, 100);
  AddView(3, root.get(), 500, 300, 100, 100);
  AddView(4, root.get(), 700, 500, 100, 100);
  AddView(5, root.get(), 100, 700, 100, 100);
  AddView(6, root.get(), 300, 700, 100, 100);

  FocusManager* manager = root->GetFocusManager();
  manager->preference().pick_first = false;
  manager->preference().pick_left_when_same_y = false;
  manager->UpdateFocusRect();

  manager->SortViews(FocusManager::Direction::kUp);
  int res_up[] = {6, 5, 4, 3, 2, 1};
  int index = 0;
  for (auto it = manager->focusable_views_.begin();
       it != manager->focusable_views_.end(); ++it) {
    EXPECT_EQ(static_cast<BaseView*>(*it)->id(), res_up[index]);
    index++;
  }

  manager->SortViews(FocusManager::Direction::kDown);
  int res_down[] = {1, 2, 3, 4, 5, 6};
  index = 0;
  for (auto it = manager->focusable_views_.begin();
       it != manager->focusable_views_.end(); ++it) {
    EXPECT_EQ(static_cast<BaseView*>(*it)->id(), res_down[index]);
    index++;
  }

  manager->SortViews(FocusManager::Direction::kLeft);
  int res_left[] = {4, 2, 3, 6, 5, 1};
  index = 0;
  for (auto it = manager->focusable_views_.begin();
       it != manager->focusable_views_.end(); ++it) {
    EXPECT_EQ(static_cast<BaseView*>(*it)->id(), res_left[index]);
    index++;
  }

  manager->SortViews(FocusManager::Direction::kRight);
  int res_right[] = {1, 5, 6, 3, 2, 4};
  index = 0;
  for (auto it = manager->focusable_views_.begin();
       it != manager->focusable_views_.end(); ++it) {
    EXPECT_EQ(static_cast<BaseView*>(*it)->id(), res_right[index]);
    index++;
  }

  root->DestroyAllChildren();
}

TEST(FocusManagerTest, FindNextFocusView) {
  auto thread = std::make_unique<fml::Thread>("ui");
  std::unique_ptr<PageView> root =
      std::make_unique<PageView>(0, nullptr, thread->GetTaskRunner());

  //                      |----------------|
  //                      |                |
  //    |----------|      |      view1     |
  //    |   view2  |      |----------------|
  //    |          |
  //    |----------|
  //
  //         |--------------|     |----------------|
  //         |              |     |     view4      |
  //         |    view3     |     |----------------|
  //         |              |
  //         |--------------|
  //
  //                        |-----|
  //                        |view5|
  //                        |-----|

  View* view1 = AddView(1, root.get(), 300, 100, 200, 200);
  View* view2 = AddView(2, root.get(), 100, 200, 150, 200);
  View* view3 = AddView(3, root.get(), 200, 500, 150, 200);
  View* view4 = AddView(4, root.get(), 400, 500, 150, 100);
  AddView(5, root.get(), 350, 800, 50, 100);

  FocusManager* manager = root->GetFocusManager();
  manager->preference().pick_first = false;
  manager->preference().pick_left_when_same_y = false;
  manager->UpdateFocusRect();

  FocusNode* next_focus_view = manager->FindNextFocusView(
      FocusManager::Direction::kDown, FocusManager::TraversalType::kDefault);
  EXPECT_EQ(static_cast<BaseView*>(next_focus_view), view1);
  manager->current_focus_view_ = next_focus_view;

  next_focus_view = manager->FindNextFocusView(
      FocusManager::Direction::kDown, FocusManager::TraversalType::kDefault);
  EXPECT_EQ(static_cast<BaseView*>(next_focus_view), view4);
  manager->current_focus_view_ = next_focus_view;

  next_focus_view = manager->FindNextFocusView(
      FocusManager::Direction::kDown, FocusManager::TraversalType::kDefault);
  EXPECT_EQ(next_focus_view, nullptr);

  next_focus_view = manager->FindNextFocusView(
      FocusManager::Direction::kLeft, FocusManager::TraversalType::kDefault);
  EXPECT_EQ(static_cast<BaseView*>(next_focus_view), view3);
  manager->current_focus_view_ = next_focus_view;

  next_focus_view = manager->FindNextFocusView(
      FocusManager::Direction::kUp, FocusManager::TraversalType::kDefault);
  EXPECT_EQ(static_cast<BaseView*>(next_focus_view), view2);
  manager->current_focus_view_ = next_focus_view;

  next_focus_view = manager->FindNextFocusView(
      FocusManager::Direction::kRight, FocusManager::TraversalType::kDefault);
  EXPECT_EQ(static_cast<BaseView*>(next_focus_view), view1);
  manager->current_focus_view_ = next_focus_view;

  root->DestroyAllChildren();
}

#if !defined(OS_WIN) && !defined(OS_MAC) && !defined(OS_HARMONY)
TEST(FocusManagerTest, ScrollViewFocusTest) {
  auto thread = std::make_unique<fml::Thread>("ui");
  std::unique_ptr<PageView> root =
      std::make_unique<PageView>(0, nullptr, thread->GetTaskRunner());
  ScrollView* scroll_view =
      new ScrollView(1, ScrollDirection::kVertical, root.get());
  scroll_view->SetX(300);
  scroll_view->SetY(300);
  scroll_view->SetWidth(700);
  scroll_view->SetHeight(300);
  root->AddChild(scroll_view);
  scroll_view->SetFocusable(true);

  View* view2 = AddViewToParent(2, root.get(), scroll_view, 0, 0, 500, 200);
  View* view3 = AddViewToParent(3, root.get(), view2, 100, 100, 200, 100);
  View* view4 = AddViewToParent(4, root.get(), scroll_view, 0, 300, 300, 400);
  View* view5 = AddViewToParent(5, root.get(), view4, 100, 100, 200, 200);
  View* view6 = AddViewToParent(6, root.get(), scroll_view, 400, 400, 300, 400);
  View* view7 = AddViewToParent(7, root.get(), view6, 100, 100, 200, 200);
  view3->SetFocusable(true);
  view5->SetFocusable(true);
  view7->SetFocusable(true);
  scroll_view->RequestFocus();
  scroll_view->OnLayoutUpdated();
  //
  //     |----------------------------|
  //     |             view2          |
  //     |     |-----------|          |
  //     |     |   view3   |          |
  //     |-----|-----------|----------|
  //
  //     |-----------------|
  //     |      view4      |
  //     |     |-----------|   |---------------|
  //     |     |   view5   |   |     view6     |
  //     |     |           |   |    |----------|
  //     |     |-----------|   |    |   view7  |
  //     |                 |   |    |          |
  //     |-----------------|   |    |----------|
  //                           |               |
  //                           |---------------|

  KeyEvent key_down(0, KeyEventType::kDown, PhysicalKeyboardKey::kUnknown,
                    KeyCode::kArrowDown, false, "");
  KeyEvent key_right(0, KeyEventType::kDown, PhysicalKeyboardKey::kUnknown,
                     KeyCode::kArrowRight, false, "");

  scroll_view->GetFocusManager()->preference().switch_up_to_once_per_frame =
      false;
  scroll_view->GetFocusManager()->DispatchKeyEvent(&key_down);
  EXPECT_TRUE(view3->IsFocused());
  EXPECT_EQ(view3->focus_rect(), FloatRect(100, 100, 200, 100));
  EXPECT_EQ(view5->focus_rect(), FloatRect(100, 400, 200, 200));
  EXPECT_EQ(view7->focus_rect(), FloatRect(500, 500, 200, 200));

  scroll_view->GetFocusManager()->DispatchKeyEvent(&key_down);
  EXPECT_TRUE(view5->IsFocused());
  EXPECT_EQ(scroll_view->GetScrollOffset().y(), 300);

  scroll_view->GetFocusManager()->DispatchKeyEvent(&key_right);
  EXPECT_TRUE(view7->IsFocused());
  EXPECT_EQ(scroll_view->GetScrollOffset().y(), 400);

  root->DestroyAllChildren();
}
#endif
}  // namespace clay
