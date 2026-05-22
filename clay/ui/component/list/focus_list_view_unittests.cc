// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>
#include <vector>

#include "base/include/fml/thread.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/focus_list_adapter.h"
#include "clay/ui/component/list/focus_list_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {
namespace {
[[maybe_unused]] std::unique_ptr<View> CreateView(PageView* page_view,
                                                  float left, float top,
                                                  float width, float height) {
  auto view = std::unique_ptr<View>(new View(-1, page_view));
  view->SetX(left);
  view->SetY(top);
  view->SetWidth(width);
  view->SetHeight(height);
  page_view->AddChild(view.get());
  view->SetFocusable(true);
  return view;
}

[[maybe_unused]] std::unique_ptr<KeyEvent> MakeEvent(KeyCode dir) {
  return std::make_unique<KeyEvent>(
      0, KeyEventType::kDown, PhysicalKeyboardKey::kUnknown, dir, false, "");
}

void SetAppearanceEvent(ListItemViewHolder* item, bool appear, bool disappear) {
  BaseView* view = item->GetView();
  FML_CHECK(view);
  const std::string item_key = std::to_string(item->GetPosition());
  view->SetAttribute("item-key", clay::Value(item_key));
  if (appear) {
    view->AddEventCallback(event_attr::kEventNodeAppear);
  }
  if (disappear) {
    view->AddEventCallback(event_attr::kEventNodeDisappear);
  }
}

class EventFocusListAdapter : public FocusListAdapter {
 public:
  explicit EventFocusListAdapter(BaseListView* list_view)
      : FocusListAdapter(list_view) {}

 protected:
  void OnBindListItem(ListItemViewHolder* item, int prev_position, int position,
                      bool newly_created) override {
    SetAppearanceEvent(item, true, true);
    FocusListAdapter::OnBindListItem(item, prev_position, position,
                                     newly_created);
  }
};

class EventFocusListView : public BaseFocusListView {
 public:
  explicit EventFocusListView(PageView* page_view)
      : BaseFocusListView(-1, page_view) {
    SetCount(50);
    SetFocusableRule([](int position) { return position >= 8; });
    SetAdapterFactory([](BaseFocusListView* thiz) {
      return std::make_unique<EventFocusListAdapter>(thiz);
    });
    SetColumnRow(0, 0);
    SetItemDefaultDimension(100.f);
    Init();
  }
  ~EventFocusListView() override = default;
};

}  // namespace

class FocusListViewTest : public UITest {};
#if !defined(OS_WIN) && !defined(OS_MAC) && !defined(OS_HARMONY)
TEST_F_UI(FocusListViewTest, Focus) {
  constexpr float kListWidth = 400.f;
  constexpr float kListHeight = 500.f;
  constexpr float kViewDefaultWidth = 100.f;
  constexpr float kViewDefaultHeight = 50.f;
  SetAllowListPrefetch(false);

  auto thread = std::make_unique<fml::Thread>("platform");

  page_->SetDefaultFocusRingEnabled(true);

  //          view1
  // view0    list      view2
  //          view3
  std::vector<std::unique_ptr<View>> views;
  views.emplace_back(CreateView(page_.get(), 0.f, kViewDefaultHeight,
                                kViewDefaultWidth, kListHeight));
  views.emplace_back(CreateView(page_.get(), kViewDefaultWidth, 0.f, kListWidth,
                                kViewDefaultHeight));
  views.emplace_back(CreateView(page_.get(), kViewDefaultWidth + kListWidth,
                                kViewDefaultHeight, kViewDefaultWidth,
                                kListHeight));
  views.emplace_back(CreateView(page_.get(), kViewDefaultWidth,
                                kViewDefaultHeight + kListHeight, kListWidth,
                                kViewDefaultHeight));

  // The list view will contain 7 items. Subviews in item 0 and 4 are focusable.
  auto list_view = std::make_unique<BaseFocusListView>(-1, page_.get());
  list_view->SetAttribute("scroll-without-focus", clay::Value(true));
  list_view->SetAttribute("focus-smooth-scroll", clay::Value(false));
  list_view->SetSkipFocusTraversal(false);
  list_view->SetCount(7);
  list_view->SetFocusableRule([](int position) { return position % 4 == 0; });
  list_view->SetColumnRow(3, 3);
  list_view->Init();

  page_->AddChild(list_view.get());
  list_view->SetX(kViewDefaultWidth);
  list_view->SetY(kViewDefaultHeight);
  list_view->SetWidth(kListWidth);
  list_view->SetHeight(kListHeight);
  list_view->Layout();
  auto* manager = list_view->GetFocusManager();
  manager->preference().pick_first = false;
  manager->preference().pick_left_when_same_y = false;

  FocusManager* root_focus = page_->GetFocusManager();
  root_focus->preference().switch_up_to_once_per_frame = false;
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool handled) {
#if !defined(OS_MACOSX) && !defined(OS_WIN)
    EXPECT_TRUE(handled);
#endif
  });
  EXPECT_EQ(root_focus->GetLeafFocusedNode(), views[1].get());

  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  EXPECT_EQ(root_focus->GetLeafFocusedNode(), list_view.get());

  // focus on child (0, 2)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowLeft), [](bool) {});
  // focus on child (0, 1)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowLeft), [](bool) {});
  // focus on child (0, 0)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowLeft), [](bool) {});
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowLeft), [](bool) {});
  EXPECT_EQ(root_focus->GetLeafFocusedNode(), views[0].get());

  // Focus back to list view
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowRight), [](bool) {});
  // focus on child (0, 0)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowRight), [](bool) {});
  // focus on child (0, 1)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowRight), [](bool) {});
  // focus on child (0, 2)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowRight), [](bool) {});
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowRight), [](bool) {});
  EXPECT_EQ(root_focus->GetLeafFocusedNode(), views[2].get());

  // back to list view
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowLeft), [](bool) {});

  std::vector<BaseView*>& children = list_view->GetChildren();
  auto is_child_at = [](BaseView* parent, BaseView* child, int y, int x) {
    const int index = y * 3 + x;
    std::vector<BaseView*>& children = parent->GetChildren();
    return children[index] == child;
  };

  // focus on child (0, 0)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  auto* focused = static_cast<BaseView*>(root_focus->GetLeafFocusedNode());
  BaseView* item = children[0];
  EXPECT_TRUE(is_child_at(item, focused, 0, 0));

  // focus on child (0, 1)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowRight), [](bool) {});
  focused = static_cast<BaseView*>(root_focus->GetLeafFocusedNode());
  EXPECT_TRUE(is_child_at(item, focused, 0, 1));

  // focus on child (1, 1)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  focused = static_cast<BaseView*>(root_focus->GetLeafFocusedNode());
  EXPECT_TRUE(is_child_at(item, focused, 1, 1));

  EXPECT_EQ(item->Top(), 0.f);

  // focus on child (2, 1)
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  // still focus on child (2, 1) but the list view will scroll down a bit.
  // Align the item 1
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  focused = static_cast<BaseView*>(root_focus->GetLeafFocusedNode());
  EXPECT_TRUE(is_child_at(item, focused, 2, 1));
  // The second item doesn't have focusable nodes. Align it's bottom with list's
  // bottom.
  item = children[1];
  EXPECT_EQ(item->Top() + item->Height(), list_view->Height());
  // Move down one more item. The previous focus view is out of bounds. The
  // focus is back to the list view. (with focus ring)
  // Align the item 2
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  focused = static_cast<BaseView*>(root_focus->GetLeafFocusedNode());
  EXPECT_EQ(focused, list_view.get());
  EXPECT_TRUE(list_view->render_object()->HasDefaultFocusRing());
  // Align the item 3
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  EXPECT_TRUE(list_view->render_object()->HasDefaultFocusRing());

  // There are focusable item in item 4. Won't align the item's bottom with the
  // list. Instead, align the focused view with the list's bottom
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  focused = static_cast<BaseView*>(root_focus->GetLeafFocusedNode());
  // There are 3 visible items.
  EXPECT_EQ(list_view->child_count(), (size_t)3);
  item = children[2];
  EXPECT_EQ(item->Top() + focused->Top() + focused->Height(),
            list_view->Height());

  // Move down 6 times. The focus will move out of the list view.
  for (int i = 0; i < 6; ++i) {
    page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  }
  focused = static_cast<BaseView*>(root_focus->GetLeafFocusedNode());
  EXPECT_EQ(root_focus->GetLeafFocusedNode(), views[3].get());

  // Move up. back to the list view
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowUp), [](bool) {});
  // Move up again. The list view will search for a focusable point in area
  // closed to the visible area.
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowUp), [](bool) {});
  focused = static_cast<BaseView*>(root_focus->GetLeafFocusedNode());
  item = children[0];
  EXPECT_TRUE(is_child_at(item, focused, 2, 2));
  // Align the focused view's top with the list.
  EXPECT_EQ(item->Top() + focused->Top(), 0.f);
}

TEST_F_UI(FocusListViewTest, EventThrottling) {
  SetAllowListPrefetch(false);

  std::vector<std::tuple<bool, int, int>> events;
  EXPECT_CALL(*this, OnCustomEvent(::testing::_, ::testing::_))
      .WillRepeatedly(::testing::Invoke(
          [&events](std::string event_name, const clay::Value::Map& params) {
            if (event_name == event_attr::kEventNodeAppear ||
                event_name == event_attr::kEventNodeDisappear) {
              int item_key = std::stoi(params.at("key").GetString());
              events.emplace_back(event_name == event_attr::kEventNodeAppear,
                                  params.at("position").GetInt(), item_key);
            }
          }));

  auto list_view = std::make_unique<EventFocusListView>(page_.get());
  page_->AddChild(list_view.get());
  list_view->SetX(0);
  list_view->SetY(0);
  list_view->SetWidth(300);
  list_view->SetHeight(900);
  list_view->Layout();
  ListEventCallbackManager* callback_manager =
      list_view->callback_manager_.get();

  // Flush the first time, nothing emits, because only the 2nd gen event will be
  // sent.
  callback_manager->FlushItemAppearanceEvents();
  EXPECT_EQ(events.size(), static_cast<size_t>(0));

  callback_manager->FlushItemAppearanceEvents();
  EXPECT_EQ(events.size(), static_cast<size_t>(9));
  events.clear();

  // "Duplicated" events happens in one iteration will be filtered.
  list_view->OnScrollBy({0, -900.f});
  EXPECT_EQ(callback_manager->appearance_events_gen1_.size(),
            static_cast<size_t>(18));
  list_view->OnScrollBy({0, 800.f});
  EXPECT_EQ(callback_manager->appearance_events_gen1_.size(),
            static_cast<size_t>(34));
  callback_manager->FlushItemAppearanceEvents();
  EXPECT_EQ(events.size(), static_cast<size_t>(0));
  EXPECT_EQ(callback_manager->appearance_events_gen1_.size(),
            static_cast<size_t>(0));
  EXPECT_EQ(callback_manager->appearance_events_gen2_.size(),
            static_cast<size_t>(2));
  callback_manager->FlushItemAppearanceEvents();
  EXPECT_EQ(events.size(), static_cast<size_t>(2));
  EXPECT_EQ(callback_manager->appearance_events_gen1_.size(),
            static_cast<size_t>(0));
  EXPECT_EQ(callback_manager->appearance_events_gen2_.size(),
            static_cast<size_t>(0));

  // Reset to the top
  list_view->OnScrollBy({0, 100.f});
  callback_manager->FlushItemAppearanceEvents();
  callback_manager->FlushItemAppearanceEvents();
  events.clear();

  // "Duplicated" events cross in two iterations will be filtered.
  list_view->OnScrollBy({0, -900.f});
  EXPECT_EQ(callback_manager->appearance_events_gen1_.size(),
            static_cast<size_t>(18));
  callback_manager->FlushItemAppearanceEvents();
  EXPECT_EQ(events.size(), static_cast<size_t>(0));
  EXPECT_EQ(callback_manager->appearance_events_gen1_.size(),
            static_cast<size_t>(0));
  EXPECT_EQ(callback_manager->appearance_events_gen2_.size(),
            static_cast<size_t>(18));
  list_view->OnScrollBy({0, -900.f});
  EXPECT_EQ(callback_manager->appearance_events_gen1_.size(),
            static_cast<size_t>(18));
  callback_manager->FlushItemAppearanceEvents();
  EXPECT_EQ(callback_manager->appearance_events_gen1_.size(),
            static_cast<size_t>(0));
  EXPECT_EQ(callback_manager->appearance_events_gen2_.size(),
            static_cast<size_t>(9));
  EXPECT_EQ(events.size(), static_cast<size_t>(9));
  // Disappearing events from 0 to 8.
  for (size_t i = 0; i < events.size(); ++i) {
    EXPECT_FALSE(std::get<0>(events[i]));
    EXPECT_EQ(std::get<1>(events[i]), static_cast<int>(i));
    EXPECT_EQ(std::get<1>(events[i]), std::get<2>(events[i]));
  }
  events.clear();

  callback_manager->FlushItemAppearanceEvents();
  EXPECT_EQ(callback_manager->appearance_events_gen1_.size(),
            static_cast<size_t>(0));
  EXPECT_EQ(callback_manager->appearance_events_gen2_.size(),
            static_cast<size_t>(0));
  EXPECT_EQ(events.size(), static_cast<size_t>(9));
  // Appearing events from 18 to 26.
  for (size_t i = 0; i < events.size(); ++i) {
    EXPECT_TRUE(std::get<0>(events[i]));
    EXPECT_EQ(std::get<1>(events[i]), static_cast<int>(i + 18));
    EXPECT_EQ(std::get<1>(events[i]), std::get<2>(events[i]));
  }
}
#endif

TEST_F_UI(FocusListViewTest, EventFindFocus) {
  SetAllowListPrefetch(false);

  std::vector<std::tuple<bool, int, int>> events;
  EXPECT_CALL(*this, OnCustomEvent(::testing::_, ::testing::_))
      .WillRepeatedly(::testing::Invoke(
          [&events](std::string event_name, const clay::Value::Map& params) {
            if (event_name == event_attr::kEventNodeAppear ||
                event_name == event_attr::kEventNodeDisappear) {
              int item_key = std::stoi(params.at("key").GetString());
              events.emplace_back(event_name == event_attr::kEventNodeAppear,
                                  params.at("position").GetInt(), item_key);
            }
          }));

  auto list_view = std::make_unique<EventFocusListView>(page_.get());
  page_->AddChild(list_view.get());
  list_view->SetX(0);
  list_view->SetY(0);
  list_view->SetWidth(300);
  list_view->SetHeight(900);
  list_view->Layout();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  list_view->callback_manager_->FlushItemAppearanceEvents();

  // Each item has height 100px. There are 9 appearing items.
  EXPECT_EQ(events.size(), static_cast<size_t>(9));
  for (size_t i = 0; i < events.size(); ++i) {
    EXPECT_TRUE(std::get<0>(events[i]));
    EXPECT_EQ(std::get<1>(events[i]), static_cast<int>(i));
    EXPECT_EQ(std::get<1>(events[i]), std::get<2>(events[i]));
  }

  events.clear();
  EXPECT_TRUE(list_view->OnScrollBy({0, -1800.f}));
  list_view->callback_manager_->FlushItemAppearanceEvents();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  EXPECT_EQ(events.size(), static_cast<size_t>(18));
  for (size_t i = 0; i < 9; ++i) {
    EXPECT_FALSE(std::get<0>(events[i]));
    EXPECT_EQ(std::get<1>(events[i]), static_cast<int>(i));
    EXPECT_EQ(std::get<1>(events[i]), std::get<2>(events[i]));
  }
  for (size_t i = 9; i < 18; ++i) {
    EXPECT_TRUE(std::get<0>(events[i]));
    EXPECT_EQ(std::get<1>(events[i]), static_cast<int>(i + 9));
    EXPECT_EQ(std::get<1>(events[i]), std::get<2>(events[i]));
  }

  // Remove items
  // Scroll back to the top.
  list_view->OnScrollBy({0, 1800.f});
  list_view->callback_manager_->FlushItemAppearanceEvents();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  events.clear();

  list_view->focus_list_adapter_->SetCount(49);
  list_view->focus_list_adapter_->NotifyItemRangeRemoved(0, 1);
  list_view->Layout();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  EXPECT_EQ(events.size(), static_cast<size_t>(2));
  EXPECT_FALSE(std::get<0>(events[0]));
  EXPECT_EQ(std::get<1>(events[0]), static_cast<int>(0));
  EXPECT_EQ(std::get<1>(events[0]), std::get<2>(events[0]));
  EXPECT_TRUE(std::get<0>(events[1]));
  EXPECT_EQ(std::get<1>(events[1]), static_cast<int>(8));
  EXPECT_EQ(std::get<1>(events[1]), std::get<2>(events[1]));

  // Add items
  events.clear();
  list_view->focus_list_adapter_->SetCount(50);
  list_view->focus_list_adapter_->NotifyItemRangeInserted(1, 1);
  list_view->Layout();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  EXPECT_EQ(events.size(), static_cast<size_t>(2));

  EXPECT_FALSE(std::get<0>(events[0]));
  // The disappearing item has latest position 9 but its item-key is still 8.
  EXPECT_EQ(std::get<1>(events[0]), static_cast<int>(9));
  EXPECT_EQ(std::get<2>(events[0]), static_cast<int>(8));

  EXPECT_TRUE(std::get<0>(events[1]));
  EXPECT_EQ(std::get<1>(events[1]), static_cast<int>(1));
  EXPECT_EQ(std::get<1>(events[1]), std::get<2>(events[1]));
}

#if !defined(OS_WIN) && !defined(OS_MAC) && !defined(OS_HARMONY)
TEST_F_UI(FocusListViewTest, AppearDisappearEvent) {
  SetAllowListPrefetch(false);

  std::vector<std::tuple<bool, int, int>> events;
  EXPECT_CALL(*this, OnCustomEvent(::testing::_, ::testing::_))
      .WillRepeatedly(::testing::Invoke(
          [&events](std::string event_name, const clay::Value::Map& params) {
            if (event_name == event_attr::kEventNodeAppear ||
                event_name == event_attr::kEventNodeDisappear) {
              int item_key = std::stoi(params.at("key").GetString());
              events.emplace_back(event_name == event_attr::kEventNodeAppear,
                                  params.at("position").GetInt(), item_key);
            }
          }));

  auto list_view = std::make_unique<EventFocusListView>(page_.get());
  list_view->SetSkipFocusTraversal(false);
  list_view->SetAttribute("focus-smooth-scroll", clay::Value(false));
  page_->AddChild(list_view.get());
  list_view->SetX(0);
  list_view->SetY(0);
  list_view->SetWidth(300);
  list_view->SetHeight(900);
  list_view->Layout();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  events.clear();

  std::vector<BaseView*>& children = list_view->GetChildren();
  EXPECT_EQ(children.size(), static_cast<size_t>(9));

  FocusManager* root_focus = page_->GetFocusManager();
  root_focus->preference().switch_up_to_once_per_frame = false;
  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  list_view->callback_manager_->FlushItemAppearanceEvents();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  EXPECT_EQ(root_focus->GetLeafFocusedNode(),
            static_cast<FocusNode*>(list_view.get()));
  EXPECT_TRUE(events.empty());

  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  EXPECT_EQ(children.size(), static_cast<size_t>(9));
  EXPECT_EQ(root_focus->GetLeafFocusedNode(),
            static_cast<FocusNode*>(children[8]));
  list_view->callback_manager_->FlushItemAppearanceEvents();
  list_view->callback_manager_->FlushItemAppearanceEvents();
  EXPECT_TRUE(events.empty());

  page_->DispatchKeyEvent(MakeEvent(KeyCode::kArrowDown), [](bool) {});
  list_view->callback_manager_->FlushItemAppearanceEvents();
  list_view->callback_manager_->FlushItemAppearanceEvents();

  // There are still 9 visible items. However, more than 9 items are laid out in
  // order to find the next focusable node.
  EXPECT_GT(children.size(), static_cast<size_t>(9));
  EXPECT_EQ(events.size(), static_cast<size_t>(2));
  EXPECT_FALSE(std::get<0>(events[0]));
  EXPECT_EQ(std::get<1>(events[0]), static_cast<int>(0));
  EXPECT_EQ(std::get<2>(events[0]), static_cast<int>(0));

  EXPECT_TRUE(std::get<0>(events[1]));
  EXPECT_EQ(std::get<1>(events[1]), static_cast<int>(9));
  EXPECT_EQ(std::get<1>(events[1]), std::get<2>(events[1]));
}
#endif
}  // namespace clay
