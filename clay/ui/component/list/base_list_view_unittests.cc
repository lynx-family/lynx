// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <forward_list>
#include <memory>
#include <vector>

#include "base/include/fml/time/time_point.h"
#include "clay/gfx/animation/animation_handler.h"
#include "clay/ui/component/layout_controller.h"
#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/list/list_adapter.h"
#include "clay/ui/component/list/list_children_helper.h"
#include "clay/ui/component/list/list_common/layout_types.h"
#include "clay/ui/component/list/list_item_view.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager_grid.h"
#include "clay/ui/component/list/list_layout_manager_staggered_grid.h"
#include "clay/ui/component/list/list_recycler.h"
#include "clay/ui/component/nested_scroll/nested_scrollable.h"
#include "clay/ui/component/view.h"
#include "clay/ui/gesture/velocity_tracker.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace {
constexpr int kDefaultSpanCount = 3;
}  // namespace

class MockListItemViewHolder : public ListItemViewHolder {
 public:
  MockListItemViewHolder(PageView* page) : page_(page) {}
  virtual ~MockListItemViewHolder() = default;

  BaseView* InitView() {
    static int id = 0;
    my_view_ = std::make_unique<View>(id++, page_);
    SetView(my_view_.get());
    return my_view_.get();
  }
  void ReleaseView() {
    SetView(nullptr);
    my_view_.reset();
  }

  MeasureResult Measure(const MeasureConstraint& constraint) override {
    BaseView::LayoutIgnoreHelper helper(GetView());
    switch (constraint.height_mode) {
      case MeasureMode::kDefinite: {
        GetView()->SetHeight(*constraint.height);
        break;
      }
      default: {
        float height = height_;
        if (double_height_when_width_expands_ && constraint.width &&
            *constraint.width > width_expand_threshold_) {
          height *= 2.f;
        }
        GetView()->SetHeight(height);
        break;
      }
    }
    switch (constraint.width_mode) {
      case MeasureMode::kDefinite: {
        GetView()->SetWidth(*constraint.width);
        break;
      }
      default: {
        GetView()->SetWidth(width_);
        break;
      }
    }
    return {GetView()->Width(), GetView()->Height()};
  }

 public:
  float height_ = 0.f;
  float width_ = 0.f;
  bool double_height_when_width_expands_ = false;
  float width_expand_threshold_ = 0.f;
  PageView* page_;
  std::unique_ptr<View> my_view_;
};

class MockAdapter : public ListAdapter {
 public:
  explicit MockAdapter(BaseListView* list_view) : ListAdapter(list_view) {}
  virtual ~MockAdapter() = default;

  ListItemViewHolder* OnCreateListItem(TypeId type) override {
    holders_.emplace_front(new MockListItemViewHolder(list_view_->page_view()));
    return holders_.front().get();
  }

  void OnBindListItem(ListItemViewHolder* item, int prev_position, int position,
                      bool newly_created) override {
    static_cast<MockListItemViewHolder*>(item)->height_ =
        height_list_[position];
    static_cast<MockListItemViewHolder*>(item)->width_ = height_list_[position];
    static_cast<MockListItemViewHolder*>(item)
        ->double_height_when_width_expands_ = double_height_when_width_expands_;
    static_cast<MockListItemViewHolder*>(item)->width_expand_threshold_ =
        width_expand_threshold_;
  }

  void OnDeleteListItem(ListItemViewHolder* view_holder) override {
    holders_.remove_if(
        [this, view_holder](std::unique_ptr<MockListItemViewHolder>& vh) {
          if (vh.get() == view_holder) {
            deleted_.emplace_back(vh.release());
            return true;
          }
          return false;
        });
  }

  int GetItemCount() const override { return height_list_.size(); }

 public:
  std::vector<float> height_list_;
  bool double_height_when_width_expands_ = false;
  float width_expand_threshold_ = 0.f;
  std::forward_list<std::unique_ptr<MockListItemViewHolder>> holders_;
  std::vector<std::unique_ptr<MockListItemViewHolder>> deleted_;
};

class MockListView : public BaseListView {
 public:
  explicit MockListView(PageView* page,
                        ScrollDirection orientation = kDefaultScrollDirection)
      : BaseListView(0, "mock-list", page) {
    SetLayoutManager(std::make_unique<ListLayoutManagerLinear>(orientation));
    page_view_->AddChild(this);
  }
  virtual ~MockListView() {
    SetAdapter(nullptr);
    StopAnimation();
  }

  AnimationHandler* GetAnimationHandler() { return &animator_handler_; }

 protected:
  void Invalidate() override { invalidated_ = true; }

  BaseView* HandleCreateView(ListItemViewHolder* item) override {
    auto* text_item = static_cast<MockListItemViewHolder*>(item);
    BaseView* new_view = text_item->InitView();
    AddChild(new_view, child_count());
    return new_view;
  }

  void HandleDestroyView(BaseView* to_destroy,
                         ListItemViewHolder* item) override {
    auto* text_item = static_cast<MockListItemViewHolder*>(item);
    text_item->ReleaseView();
  }

 public:
  bool invalidated_ = false;
  LayoutController controller_;
  AnimationHandler animator_handler_;
};

class MockGridView : public MockListView {
 public:
  MockGridView(PageView* page, int span_count,
               ScrollDirection orientation = kDefaultScrollDirection)
      : MockListView(page, orientation) {
    SetLayoutManager(
        std::make_unique<ListLayoutManagerGrid>(span_count, orientation));
  }
  virtual ~MockGridView() { SetAdapter(nullptr); }
};

class MockStaggeredGridView : public MockListView {
 public:
  explicit MockStaggeredGridView(
      PageView* page, int span_count,
      ScrollDirection orientation = kDefaultScrollDirection)
      : MockListView(page, orientation) {
    SetLayoutManager(std::make_unique<ListLayoutManagerStaggeredGrid>(
        span_count, orientation));
  }
  virtual ~MockStaggeredGridView() { SetAdapter(nullptr); }
};

class ListLayoutManagerTest : public UITest {};
class ListLayoutManagerLinearTest : public ListLayoutManagerTest {};
class ListLayoutManagerGridTest : public ListLayoutManagerTest {};
class ListLayoutManagerStaggeredGridTest : public ListLayoutManagerTest {};

TEST_F_UI(ListLayoutManagerLinearTest, ListItemSizeChangedRequestsLayout) {
  MockListView* list_view = new MockListView(page_.get());
  auto list_item = std::make_unique<ListItemView>(100, page_.get());

  list_view->BaseView::AddChild(list_item.get(), 0);
  list_view->Layout();
  EXPECT_FALSE(list_view->NeedsLayout());

  list_item->SetBound(0.f, 0.f, 100.f, 20.f);
  list_view->Layout();
  EXPECT_FALSE(list_view->NeedsLayout());

  list_item->SetBound(0.f, 0.f, 100.f, 40.f);
  EXPECT_TRUE(list_view->NeedsLayout());

  list_view->BaseView::RemoveChild(list_item.get());
}

TEST_F_UI(ListLayoutManagerLinearTest, Layout) {
  // Empty list
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(50.f);
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 0);
  }

  // 10 items with identical height
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(50.f);
    for (int i = 0; i < 10; ++i) {
      adapter.height_list_.emplace_back(10.f);
    }

    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);
    std::vector<BaseView*>& children = list_view->GetChildren();
    float expected_height = 0.f;
    for (int i = 0; i < 5; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }
  }

  // 10 items with identical height, list has border.
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(50.f);
    BordersData scroll_border_data;
    scroll_border_data.width_top_ = 5.f;
    scroll_border_data.width_bottom_ = 5.f;
    list_view->SetBorder(scroll_border_data);
    for (int i = 0; i < 10; ++i) {
      adapter.height_list_.emplace_back(10.f);
    }
    list_view->Layout();
    // 50 - 5 - 5 = 40.
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 4);
    std::vector<BaseView*>& children = list_view->GetChildren();
    float expected_height = 5.f;
    for (int i = 0; i < 4; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }
  }

  // 10 items with different height
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetX(10.f);
    list_view->SetY(10.f);
    list_view->SetWidth(200.f);
    list_view->SetHeight(50.f);
    // 10 + 20 + 30(partial) > 50
    for (int i = 0; i < 10; ++i) {
      adapter.height_list_.emplace_back((i + 1) * 10.f);
    }
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 3);
    std::vector<BaseView*>& children = list_view->GetChildren();
    float expected_height = 0.f;
    for (int i = 0; i < 3; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += (i + 1) * 10.f;
    }
  }
}

TEST_F_UI(ListLayoutManagerLinearTest, InvalidateLengthCacheOnCrossAxisResize) {
  MockListView* list_view = new MockListView(page_.get());
  MockAdapter adapter(list_view);
  list_view->SetAdapter(&adapter);
  list_view->SetWidth(300.f);
  list_view->SetHeight(50.f);
  for (int i = 0; i < 100; ++i) {
    adapter.height_list_.emplace_back(10.f);
  }
  adapter.double_height_when_width_expands_ = true;
  adapter.width_expand_threshold_ = 300.f;

  list_view->Layout();
  EXPECT_EQ(list_view->GetTotalScrollLength(), 1000.f);

  list_view->SetWidth(390.f);
  list_view->Layout();

  EXPECT_EQ(list_view->GetTotalScrollLength(), 2000.f);
}

TEST_F_UI(ListLayoutManagerLinearTest, Scroll) {
  MockListView* list_view = new MockListView(page_.get());
  MockAdapter adapter(list_view);
  list_view->SetAdapter(&adapter);
  list_view->SetWidth(200.f);
  list_view->SetHeight(50.f);
  BordersData scroll_border_data;
  scroll_border_data.width_top_ = 5.f;
  scroll_border_data.width_bottom_ = 5.f;
  list_view->SetBorder(scroll_border_data);
  for (int i = 0; i < 10; ++i) {
    adapter.height_list_.emplace_back(10.f);
  }
  list_view->Layout();
  list_view->invalidated_ = false;

  // Scroll up but no space left.
  list_view->OnScrollBy(FloatSize(0.f, 10.f));
  std::vector<BaseView*>& children = list_view->GetChildren();
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 4);
  float expected_height = 5.f;
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(children[i]->Top(), expected_height);
    expected_height += 10.f;
  }
  EXPECT_FALSE(list_view->invalidated_);

  // Scroll down 5 px. Item on the head is not hidden yet but a new item is
  // added to the tail.
  list_view->OnScrollBy(FloatSize(0.f, -5.f));
  EXPECT_TRUE(list_view->invalidated_);
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);
  expected_height = 0.f;
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(children[i]->Top(), expected_height);
    expected_height += 10.f;
  }
  list_view->invalidated_ = false;

  // Scroll down another 5 px. Item on the head is recycled.
  list_view->OnScrollBy(FloatSize(0.f, -5.f));
  EXPECT_TRUE(list_view->invalidated_);
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 4);
  expected_height = 5.f;
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(children[i]->Top(), expected_height);
    expected_height += 10.f;
  }
  list_view->invalidated_ = false;

  // Scroll up back 5 px. A recycled item is used as the head.
  list_view->OnScrollBy(FloatSize(0.f, 5.f));
  EXPECT_TRUE(list_view->invalidated_);
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);
  expected_height = 0.f;
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(children[i]->Top(), expected_height);
    expected_height += 10.f;
  }
  list_view->invalidated_ = false;

  // Scroll down to the end.
  list_view->OnScrollBy(FloatSize(0.f, -1000.f));
  EXPECT_TRUE(list_view->invalidated_);
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 4);
  expected_height = 5.f;
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(children[i]->Top(), expected_height);
    expected_height += 10.f;
  }
  list_view->invalidated_ = false;

  // Already at the end.
  list_view->OnScrollBy(FloatSize(0.f, -1000.f));
  EXPECT_FALSE(list_view->invalidated_);
}

TEST_F_UI(ListLayoutManagerLinearTest, ScrollHorizontally) {
  MockListView* list_view =
      new MockListView(page_.get(), ScrollDirection::kHorizontal);
  MockAdapter adapter(list_view);
  list_view->SetAdapter(&adapter);
  list_view->SetWidth(50.f);
  list_view->SetHeight(200.f);
  BordersData scroll_border_data;
  scroll_border_data.width_left_ = 5.f;
  scroll_border_data.width_right_ = 5.f;
  list_view->SetBorder(scroll_border_data);
  for (int i = 0; i < 10; ++i) {
    adapter.height_list_.emplace_back(10.f);
  }
  list_view->Layout();
  list_view->invalidated_ = false;

  // Scroll left but no space left.
  list_view->OnScrollBy(FloatSize(10.f, 0.f));
  std::vector<BaseView*>& children = list_view->GetChildren();
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 4);
  float expected_left = 5.f;
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(children[i]->Left(), expected_left);
    expected_left += 10.f;
  }
  EXPECT_FALSE(list_view->invalidated_);

  // Scroll right 5 px. Item on the head is not hidden yet but a new item is
  // added to the tail.
  list_view->OnScrollBy(FloatSize(-5.f, 0.f));
  EXPECT_TRUE(list_view->invalidated_);
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);
  expected_left = 0.f;
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(children[i]->Left(), expected_left);
    expected_left += 10.f;
  }
  list_view->invalidated_ = false;

  // Scroll right another 5 px. Item on the head is recycled.
  list_view->OnScrollBy(FloatSize(-5.f, 0.f));
  EXPECT_TRUE(list_view->invalidated_);
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 4);
  expected_left = 5.f;
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(children[i]->Left(), expected_left);
    expected_left += 10.f;
  }
  list_view->invalidated_ = false;

  // Scroll left back 5 px. A recycled item is used as the head.
  list_view->OnScrollBy(FloatSize(5.f, 0.f));
  EXPECT_TRUE(list_view->invalidated_);
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);
  expected_left = 0.f;
  for (int i = 0; i < 5; ++i) {
    EXPECT_EQ(children[i]->Left(), expected_left);
    expected_left += 10.f;
  }
  list_view->invalidated_ = false;

  // Scroll right to the end.
  list_view->OnScrollBy(FloatSize(-1000.f, 0.f));
  EXPECT_TRUE(list_view->invalidated_);
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 4);
  expected_left = 5.f;
  for (int i = 0; i < 4; ++i) {
    EXPECT_EQ(children[i]->Left(), expected_left);
    expected_left += 10.f;
  }
  list_view->invalidated_ = false;

  // Already at the end.
  list_view->OnScrollBy(FloatSize(-1000.f, 0.f));
  EXPECT_FALSE(list_view->invalidated_);
}

TEST_F_UI(ListLayoutManagerLinearTest, ScrollDiffHeight) {
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(40.f);
    for (int i = 0; i < 10; ++i) {
      if (i == 4) {
        adapter.height_list_.emplace_back(40.f);
      } else {
        adapter.height_list_.emplace_back(10.f);
      }
    }
    list_view->Layout();

    // Scroll down 40 px. Four items disappear and one appear.
    list_view->OnScrollBy(FloatSize(0.f, -40.f));
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 1);
  }

  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(40.f);
    for (int i = 0; i < 10; ++i) {
      if (i >= 4) {
        adapter.height_list_.emplace_back(2.f);
      } else {
        adapter.height_list_.emplace_back(10.f);
      }
    }
    list_view->Layout();

    // Scroll down 9 px. Two items disappear and one appear.
    list_view->OnScrollBy(FloatSize(0.f, -9.f));
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 9);

    // Scroll up 9 px.
    list_view->OnScrollBy(FloatSize(0.f, 9.f));
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 4);
  }
}

TEST_F_UI(ListLayoutManagerLinearTest, DISABLED_InsertData) {
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(100.f);
    std::vector<float>& height_list = adapter.height_list_;
    for (int i = 0; i < 5; ++i) {
      height_list.emplace_back(10.f);
    }
    EXPECT_TRUE(list_view->NeedsLayout());
    list_view->Layout();
    EXPECT_FALSE(list_view->NeedsLayout());
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);

    // Insert an item with 1.f height to head.
    height_list.insert(height_list.begin(), 1.f);
    adapter.NotifyItemRangeInserted(0, 1);
    EXPECT_TRUE(list_view->NeedsLayout());
    list_view->Layout();
    EXPECT_FALSE(list_view->NeedsLayout());
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 6);
    std::vector<BaseView*>& children = list_view->GetChildren();
    EXPECT_EQ(children[0]->Top(), 0.f);
    EXPECT_EQ(children[0]->Height(), 1.f);

    float expected_height = 1.f;
    for (int i = 1; i < 6; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }

    // Insert 4 items with 1.f height to head at once.
    for (int i = 0; i < 4; ++i) {
      height_list.insert(height_list.begin(), 1.f);
      adapter.NotifyItemRangeInserted(0, 1);
    }
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 10);
    expected_height = 0.f;
    for (int i = 0; i < 5; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 1.f;
    }
    for (int i = 5; i < 10; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }

    // Insert 10 items with 2.f height with one call
    auto itr = height_list.begin();
    for (int i = 0; i < 5; ++i) {
      ++itr;
    }
    for (int i = 0; i < 10; ++i) {
      itr = height_list.insert(itr, 2.f);
    }
    adapter.NotifyItemRangeInserted(5, 10);
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 20);
    expected_height = 0.f;
    for (int i = 0; i < 5; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 1.f;
    }
    for (int i = 5; i < 15; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 2.f;
    }
    for (int i = 15; i < 20; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }
  }

  // Fill the list from the head. When the all items height exceeds list view's
  // height, should align with the bottom item.
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(50.f);
    std::vector<float>& height_list = adapter.height_list_;
    for (int i = 0; i < 4; ++i) {
      height_list.emplace_back(10.f);
    }
    list_view->Layout();
    height_list.insert(height_list.begin(), 11.f);
    adapter.NotifyItemRangeInserted(0, 1);
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);
    std::vector<BaseView*>& children = list_view->GetChildren();
    float expected_height = -1.f;
    EXPECT_EQ(children[0]->Top(), expected_height);
    expected_height += 11.f;
    for (int i = 1; i < 5; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }
  }

  // When the inserted position is not in the frame, it won't affect the current
  // view.
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(50.f);
    std::vector<float>& height_list = adapter.height_list_;
    for (int i = 0; i < 10; ++i) {
      height_list.emplace_back(10.f);
    }
    for (int i = 0; i < 10; ++i) {
      height_list.emplace_back(5.f);
    }
    for (int i = 0; i < 10; ++i) {
      height_list.emplace_back(10.f);
    }
    list_view->Layout();
    // Scroll down 100 px. Now it should show 10 children with 5px height.
    list_view->OnScrollBy({0.f, -100.f});
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 10);
    std::vector<BaseView*>& children = list_view->GetChildren();
    float expected_height = 0.f;
    for (int i = 0; i < 10; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 5.f;
    }

    // Insert an item to index 9.
    auto itr = height_list.begin();
    for (int i = 0; i < 9; i++) {
      ++itr;
    }
    height_list.insert(itr, 20.f);
    adapter.NotifyItemRangeInserted(9, 1);
    list_view->Layout();
    // Should not change the current view.
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 10);
    expected_height = 0.f;
    for (int i = 0; i < 10; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 5.f;
    }

    // Scroll up 30 px. Should see the new item and the original item at 9.
    list_view->OnScrollBy({0.f, 30.f});
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 6);
    expected_height = 0.f;
    EXPECT_EQ(children[0]->Top(), expected_height);
    expected_height += 20.f;
    EXPECT_EQ(children[1]->Top(), expected_height);
    expected_height += 10.f;
    for (int i = 2; i < 6; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 5.f;
    }
  }
}

// Recycler's Max Limit is tested in this case.
TEST_F_UI(ListLayoutManagerLinearTest, RemoveData) {
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(100.f);
    std::vector<float>& height_list = adapter.height_list_;
    for (int i = 0; i < 10; ++i) {
      height_list.emplace_back(10.f);
    }
    height_list.emplace_back(20.f);
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 10);
    std::vector<BaseView*>& children = list_view->GetChildren();
    float expected_height = 0.f;
    for (int i = 0; i < 10; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }

    // Scroll down 5 px. All 11 items should in the frame.
    list_view->OnScrollBy({0.f, -5.f});
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 11);

    // Remove two items from the head
    height_list.erase(height_list.begin());
    height_list.erase(height_list.begin());
    adapter.NotifyItemRangeRemoved(0, 2);
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 9);
    expected_height = 0.f;
    for (int i = 0; i < 9; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }

    // Remove one item from the tail
    height_list.pop_back();
    adapter.NotifyItemRangeRemoved(height_list.size() - 2, 1);
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 8);
    expected_height = 0.f;
    for (int i = 0; i < 8; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }

    // Remove all items
    height_list.clear();
    adapter.NotifyItemRangeRemoved(0, 8);
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 0);
  }

  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(50.f);
    std::vector<float>& height_list = adapter.height_list_;
    for (int i = 0; i < 10; ++i) {
      height_list.emplace_back(1.f);
    }
    for (int i = 0; i < 20; ++i) {
      height_list.emplace_back(10.f);
    }
    for (int i = 0; i < 10; ++i) {
      height_list.emplace_back(1.f);
    }
    list_view->Layout();
    std::vector<BaseView*>& children = list_view->GetChildren();

    list_view->OnScrollBy({0.f, -55.f});
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 6);
    float expected_height = -5.f;
    for (int i = 0; i < 6; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }

    // Removing item out of frame should not affect the current view
    {
      auto itr = height_list.begin();
      ++itr;
      ++itr;
      height_list.erase(itr);
      adapter.NotifyItemRangeRemoved(2, 1);
    }
    {
      auto itr = height_list.begin();
      for (int i = 0; i < 34; ++i) {
        ++itr;
      }
      height_list.erase(itr);
      adapter.NotifyItemRangeRemoved(34, 1);
    }
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 6);
    expected_height = -5.f;
    for (int i = 0; i < 6; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }
  }
}

TEST_F_UI(ListLayoutManagerLinearTest, MoveData) {
  // from > to
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(50.f);
    std::vector<float>& height_list = adapter.height_list_;
    for (int i = 0; i < 10; ++i) {
      height_list.emplace_back(10.f);
    }
    height_list.emplace_back(1.f);
    list_view->Layout();
    std::vector<BaseView*>& children = list_view->GetChildren();

    // Scroll Down 50 px. Then 5 ~ 9 items are shown.
    list_view->OnScrollBy({0.f, -50.f});
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);
    float expected_height = 0.f;
    for (int i = 0; i < 5; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }

    // Move the second to last (9th) item to head.
    // Because 0 - 9 items having the same value, we don't need to operate on
    // height_list.
    adapter.NotifyItemMoved(9, 1);
    list_view->Layout();
    // The bottom should align with the 10th item.
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 6);
    expected_height = -1.f;
    for (int i = 0; i < 6; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }
  }

  // If the moved item is the current anchor, the view will try to scroll to the
  // new position that the moved item is still the anchor.
  {
    MockListView* list_view = new MockListView(page_.get());
    MockAdapter adapter(list_view);
    list_view->SetAdapter(&adapter);
    list_view->SetWidth(200.f);
    list_view->SetHeight(50.f);
    std::vector<float>& height_list = adapter.height_list_;
    height_list.emplace_back(10.f);
    height_list.emplace_back(11.f);
    for (int i = 0; i < 4; ++i) {
      height_list.emplace_back(10.f);
    }
    for (int i = 0; i < 5; ++i) {
      height_list.emplace_back(20.f);
    }
    list_view->Layout();
    std::vector<BaseView*>& children = list_view->GetChildren();

    // Scroll Down 11 px, so that [1] item is the anchor.
    list_view->OnScrollBy({0.f, -11.f});
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);
    float expected_height = -1.f;
    EXPECT_EQ(children[0]->Top(), expected_height);
    expected_height += 11.f;
    for (int i = 1; i < 5; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 10.f;
    }

    // Move [1] item to index 5
    height_list[1] = 10.f;
    height_list[5] = 11.f;
    adapter.NotifyItemMoved(1, 5);
    list_view->Layout();
    EXPECT_EQ(static_cast<int>(list_view->child_count()), 3);
    // The anchor is still the moved item.
    expected_height = -1.f;
    EXPECT_EQ(children[0]->Top(), expected_height);
    expected_height += 11.f;
    for (int i = 1; i < 3; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      expected_height += 20.f;
    }
  }
}

TEST_F_UI(ListLayoutManagerLinearTest, ChangeData) {
  MockListView* list_view = new MockListView(page_.get());
  MockAdapter adapter(list_view);
  list_view->SetAdapter(&adapter);
  list_view->SetWidth(200.f);
  list_view->SetHeight(50.f);
  std::vector<float>& height_list = adapter.height_list_;
  for (int i = 0; i < 30; ++i) {
    height_list.emplace_back(10.f);
  }
  list_view->Layout();
  std::vector<BaseView*>& children = list_view->GetChildren();

  for (int i = 2; i < 12; ++i) {
    height_list[i] = 1.f;
  }
  adapter.NotifyItemRangeChanged(2, 10);
  list_view->Layout();
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 14);
  float expected_height = 0.f;
  EXPECT_EQ(children[0]->Top(), expected_height);
  expected_height += 10.f;
  EXPECT_EQ(children[1]->Top(), expected_height);
  expected_height += 10.f;
  for (int i = 2; i < 12; ++i) {
    EXPECT_EQ(children[i]->Top(), expected_height);
    expected_height += 1.f;
  }
  EXPECT_EQ(children[12]->Top(), expected_height);
  expected_height += 10.f;
  EXPECT_EQ(children[13]->Top(), expected_height);
  expected_height += 10.f;
}

TEST_F_UI(ListLayoutManagerGridTest, Layout) {
  // Empty list
  {
    MockGridView* grid_view = new MockGridView(page_.get(), kDefaultSpanCount);
    MockAdapter adapter(grid_view);
    grid_view->SetAdapter(&adapter);
    grid_view->SetWidth(200.f);
    grid_view->SetHeight(50.f);
    grid_view->Layout();
    EXPECT_EQ(static_cast<int>(grid_view->child_count()), 0);
  }

  // 30 items with identical height
  {
    MockGridView* grid_view = new MockGridView(page_.get(), kDefaultSpanCount);
    MockAdapter adapter(grid_view);
    grid_view->SetAdapter(&adapter);
    grid_view->SetWidth(200.f);
    grid_view->SetHeight(50.f);
    for (int i = 0; i < 30; ++i) {
      adapter.height_list_.emplace_back(10.f);
    }
    grid_view->Layout();
    EXPECT_EQ(static_cast<int>(grid_view->child_count()), 15);
    std::vector<BaseView*>& children = grid_view->GetChildren();
    float expected_height = -10.f;
    for (int i = 0; i < 15; ++i) {
      if (i % kDefaultSpanCount == 0) {
        expected_height += 10.f;
      }
      EXPECT_EQ(children[i]->Top(), expected_height);
    }
  }

  // 30 items with identical height, list has border.
  {
    MockGridView* grid_view = new MockGridView(page_.get(), kDefaultSpanCount);
    MockAdapter adapter(grid_view);
    grid_view->SetAdapter(&adapter);
    grid_view->SetWidth(200.f);
    grid_view->SetHeight(50.f);
    BordersData scroll_border_data;
    scroll_border_data.width_top_ = 5.f;
    scroll_border_data.width_bottom_ = 5.f;
    grid_view->SetBorder(scroll_border_data);
    for (int i = 0; i < 30; ++i) {
      adapter.height_list_.emplace_back(10.f);
    }
    grid_view->Layout();
    // 50 - 5 - 5 = 40.
    EXPECT_EQ(static_cast<int>(grid_view->child_count()), 12);
    std::vector<BaseView*>& children = grid_view->GetChildren();
    float expected_height = 5.f;
    for (int i = 0; i < 12; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      if (i % kDefaultSpanCount == kDefaultSpanCount - 1) {
        expected_height += 10.f;
      }
    }
  }

  // 30 items with different height
  {
    MockGridView* grid_view = new MockGridView(page_.get(), kDefaultSpanCount);
    MockAdapter adapter(grid_view);
    grid_view->SetAdapter(&adapter);
    grid_view->SetWidth(200.f);
    grid_view->SetHeight(50.f);
    // first row is 30(max height), second row is 60.
    for (int i = 0; i < 10; ++i) {
      adapter.height_list_.emplace_back((i + 1) * 10.f);
    }
    grid_view->Layout();
    EXPECT_EQ(static_cast<int>(grid_view->child_count()), 6);
    std::vector<BaseView*>& children = grid_view->GetChildren();
    float expected_height = -30.f;
    for (int i = 0; i < 6; ++i) {
      if (i % kDefaultSpanCount == 0) {
        expected_height += 30.f;
      }
      EXPECT_EQ(children[i]->Top(), expected_height);
    }
  }
}

TEST_F_UI(ListLayoutManagerGridTest, Scroll) {
  MockGridView* grid_view = new MockGridView(page_.get(), kDefaultSpanCount);
  MockAdapter adapter(grid_view);
  grid_view->SetAdapter(&adapter);
  grid_view->SetWidth(200.f);
  grid_view->SetHeight(50.f);
  grid_view->Layout();

  // The height of row is 30.f in first three, 20.f in last row.
  for (int i = 0; i < 10; ++i) {
    int height = 20.f;
    if (i % (kDefaultSpanCount + 1) == 0) {
      height = 30.f;
    }
    adapter.height_list_.emplace_back(height);
  }
  grid_view->Layout();
  grid_view->invalidated_ = false;

  // Scroll up but no space left.
  grid_view->OnScrollBy(FloatSize(0.f, 10.f));
  std::vector<BaseView*>& children = grid_view->GetChildren();
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 6);
  float expected_height = -30.f;
  for (int i = 0; i < 6; ++i) {
    if (i % kDefaultSpanCount == 0) {
      expected_height += 30.f;
    }
    EXPECT_EQ(children[i]->Top(), expected_height);
  }
  EXPECT_FALSE(grid_view->invalidated_);

  // Scroll down 20 px. Item on the head is not hidden yet but a new item is
  // added to the tail.
  grid_view->OnScrollBy(FloatSize(0.f, -20.f));
  EXPECT_TRUE(grid_view->invalidated_);
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 9);
  expected_height = -50.f;
  for (int i = 0; i < 9; ++i) {
    if (i % kDefaultSpanCount == 0) {
      expected_height += 30.f;
    }
    EXPECT_EQ(children[i]->Top(), expected_height);
  }
  grid_view->invalidated_ = false;

  // Scroll down another 15 px. Items on the head was recycled.
  grid_view->OnScrollBy(FloatSize(0.f, -15.f));
  EXPECT_TRUE(grid_view->invalidated_);
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 6);
  expected_height = -35.f;
  for (int i = 0; i < 6; ++i) {
    if (i % kDefaultSpanCount == 0) {
      expected_height += 30.f;
    }
    EXPECT_EQ(children[i]->Top(), expected_height);
  }
  grid_view->invalidated_ = false;

  // Scroll up back 15 px. Recycled items was add to head.
  grid_view->OnScrollBy(FloatSize(0.f, 15.f));
  EXPECT_TRUE(grid_view->invalidated_);
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 9);
  expected_height = -50.f;
  for (int i = 0; i < 9; ++i) {
    if (i % kDefaultSpanCount == 0) {
      expected_height += 30.f;
    }
    EXPECT_EQ(children[i]->Top(), expected_height);
  }
  grid_view->invalidated_ = false;

  // Scroll down to the end.
  grid_view->OnScrollBy(FloatSize(0.f, -1000.f));
  EXPECT_TRUE(grid_view->invalidated_);
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 4);
  expected_height = -30.f;
  for (int i = 0; i < 4; ++i) {
    if (i % kDefaultSpanCount == 0) {
      expected_height += 30.f;
    }
    EXPECT_EQ(children[i]->Top(), expected_height);
  }
  grid_view->invalidated_ = false;

  // Already at the end.
  grid_view->OnScrollBy(FloatSize(0.f, -1000.f));
  EXPECT_FALSE(grid_view->invalidated_);
}

TEST_F_UI(ListLayoutManagerStaggeredGridTest, Layout) {
  // Empty list
  {
    MockStaggeredGridView* staggered_view =
        new MockStaggeredGridView(page_.get(), kDefaultSpanCount);
    MockAdapter adapter(staggered_view);
    staggered_view->SetAdapter(&adapter);
    staggered_view->SetWidth(200.f);
    staggered_view->SetHeight(50.f);
    staggered_view->Layout();
    EXPECT_EQ(static_cast<int>(staggered_view->child_count()), 0);
  }

  // 30 items with identical height
  {
    MockStaggeredGridView* staggered_view =
        new MockStaggeredGridView(page_.get(), kDefaultSpanCount);
    MockAdapter adapter(staggered_view);
    staggered_view->SetAdapter(&adapter);
    staggered_view->SetWidth(200.f);
    staggered_view->SetHeight(50.f);
    for (int i = 0; i < 30; ++i) {
      adapter.height_list_.emplace_back(10.f);
    }
    staggered_view->Layout();
    EXPECT_EQ(static_cast<int>(staggered_view->child_count()), 15);
    std::vector<BaseView*>& children = staggered_view->GetChildren();
    float expected_height = -10.f;
    for (int i = 0; i < 15; ++i) {
      if (i % kDefaultSpanCount == 0) {
        expected_height += 10.f;
      }
      EXPECT_EQ(children[i]->Top(), expected_height);
    }
  }

  // 30 items with identical height, list has border.
  {
    MockStaggeredGridView* grid_view =
        new MockStaggeredGridView(page_.get(), kDefaultSpanCount);
    MockAdapter adapter(grid_view);
    grid_view->SetAdapter(&adapter);
    grid_view->SetWidth(200.f);
    grid_view->SetHeight(50.f);
    BordersData scroll_border_data;
    scroll_border_data.width_top_ = 5.f;
    scroll_border_data.width_bottom_ = 5.f;
    grid_view->SetBorder(scroll_border_data);
    for (int i = 0; i < 30; ++i) {
      adapter.height_list_.emplace_back(10.f);
    }
    grid_view->Layout();
    // 50 - 5 - 5 = 40.
    EXPECT_EQ(static_cast<int>(grid_view->child_count()), 12);
    std::vector<BaseView*>& children = grid_view->GetChildren();
    float expected_height = 5.f;
    for (int i = 0; i < 12; ++i) {
      EXPECT_EQ(children[i]->Top(), expected_height);
      if (i % kDefaultSpanCount == kDefaultSpanCount - 1) {
        expected_height += 10.f;
      }
    }
  }

  // 30 items with different height
  {
    MockStaggeredGridView* grid_view =
        new MockStaggeredGridView(page_.get(), kDefaultSpanCount);
    MockAdapter adapter(grid_view);
    grid_view->SetAdapter(&adapter);
    grid_view->SetWidth(200.f);
    grid_view->SetHeight(50.f);
    // first row is 30(max height).
    for (int i = 0; i < 10; ++i) {
      adapter.height_list_.emplace_back((i + 1) * 10.f);
    }
    grid_view->Layout();
    EXPECT_EQ(static_cast<int>(grid_view->child_count()), 6);
    std::vector<BaseView*>& children = grid_view->GetChildren();
    std::array<float, kDefaultSpanCount> expected_heights;
    expected_heights.fill(0.f);
    for (int i = 0; i < 6; ++i) {
      int span_idx = i % kDefaultSpanCount;
      EXPECT_EQ(children[i]->Top(), expected_heights[span_idx]);
      expected_heights[span_idx] += 10.f * (i + 1);
    }
  }
}

TEST_F_UI(ListLayoutManagerStaggeredGridTest, Scroll) {
  MockStaggeredGridView* grid_view =
      new MockStaggeredGridView(page_.get(), kDefaultSpanCount);
  MockAdapter adapter(grid_view);
  grid_view->SetAdapter(&adapter);
  grid_view->SetWidth(200.f);
  grid_view->SetHeight(60.f);
  grid_view->Layout();

  for (int i = 0; i < 10; ++i) {
    adapter.height_list_.emplace_back((i + 1) * 10.f);
  }
  grid_view->Layout();
  grid_view->invalidated_ = false;

  // Scroll up but no space left.
  grid_view->OnScrollBy(FloatSize(0.f, 10.f));
  std::vector<BaseView*>& children = grid_view->GetChildren();
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 7);

  std::array<float, kDefaultSpanCount> expected_heights;
  expected_heights.fill(0.f);
  for (int i = 0; i < 7; ++i) {
    int span_idx = i % kDefaultSpanCount;
    EXPECT_EQ(children[i]->Top(), expected_heights[span_idx]);
    expected_heights[span_idx] += 10.f * (i + 1);
  }
  EXPECT_FALSE(grid_view->invalidated_);

  // Scroll down 5 px. Item on the head is not hidden yet but a new item is
  // added to the tail.
  grid_view->OnScrollBy(FloatSize(0.f, -5.f));
  EXPECT_TRUE(grid_view->invalidated_);
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 7);
  expected_heights.fill(-5.f);
  for (int i = 0; i < 7; ++i) {
    int span_idx = i % kDefaultSpanCount;
    EXPECT_EQ(children[i]->Top(), expected_heights[span_idx]);
    expected_heights[span_idx] += 10.f * (i + 1);
  }
  grid_view->invalidated_ = false;

  // Scroll down another 15 px. Two items on the head was recycled.
  grid_view->OnScrollBy(FloatSize(0.f, -15.f));
  EXPECT_TRUE(grid_view->invalidated_);
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 6);
  expected_heights[0] = -20.f;  // span 3
  expected_heights[1] = -10.f;  // span 1
  expected_heights[2] = 0.f;    // span 2
  for (int i = 0; i < 6; ++i) {
    int span_idx = i % kDefaultSpanCount;
    EXPECT_EQ(children[i]->Top(), expected_heights[span_idx]);
    expected_heights[span_idx] += 10.f * (i + 3);
  }
  grid_view->invalidated_ = false;

  // Scroll up back 15 px. Recycled items was add to head.
  grid_view->OnScrollBy(FloatSize(0.f, 15.f));
  EXPECT_TRUE(grid_view->invalidated_);
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 7);
  expected_heights.fill(-5.f);
  for (int i = 0; i < 7; ++i) {
    int span_idx = i % kDefaultSpanCount;
    EXPECT_EQ(children[i]->Top(), expected_heights[span_idx]);
    expected_heights[span_idx] += 10.f * (i + 1);
  }
  grid_view->invalidated_ = false;
  // Scroll down to the end.
  grid_view->OnScrollBy(FloatSize(0.f, -1000.f));
  EXPECT_TRUE(grid_view->invalidated_);
  EXPECT_EQ(static_cast<int>(grid_view->child_count()), 3);
  EXPECT_EQ(children[0]->Top(), -90.f);
  EXPECT_EQ(children[0]->Height(), 80.f);
  EXPECT_EQ(children[1]->Top(), -70.f);
  EXPECT_EQ(children[1]->Height(), 90.f);
  EXPECT_EQ(children[2]->Top(), -40.f);
  EXPECT_EQ(children[2]->Height(), 100.f);
  grid_view->invalidated_ = false;

  // Already at the end.
  grid_view->OnScrollBy(FloatSize(0.f, -1000.f));
  EXPECT_FALSE(grid_view->invalidated_);
}

TEST_F_UI(ListLayoutManagerLinearTest, ScrollToPositionImmediately) {
  MockListView* list_view = new MockListView(page_.get());
  MockAdapter adapter(list_view);
  list_view->SetAdapter(&adapter);
  list_view->SetWidth(200.f);
  list_view->SetHeight(50.f);
  for (int i = 0; i < 10; ++i) {
    adapter.height_list_.emplace_back(10.f);
  }
  list_view->Layout();
  list_view->invalidated_ = false;

  // Offset or AlignTo is not supported yet.
  bool callback_called = false;
  list_view->ScrollToPosition(
      false, 9, 0.f, AlignTo::kNone, "",
      [&callback_called](uint32_t code, const std::string& unused) {
        EXPECT_EQ(code, static_cast<uint32_t>(LynxUIMethodResult::kSuccess));
        callback_called = true;
      });
  EXPECT_TRUE(callback_called);

  std::vector<BaseView*>& children = list_view->GetChildren();
  EXPECT_EQ(static_cast<int>(list_view->child_count()), 5);
  EXPECT_EQ(children[0]->Top(), 0.f);
  ListChildrenHelper* children_helper = list_view->children_helper_.get();
  EXPECT_EQ(children_helper->GetChildAt(0)->GetPosition(), 5);
}

TEST_F_UI(ListLayoutManagerLinearTest, DISABLED_ScrollToPositionSmooth) {
  MockListView* list_view = new MockListView(page_.get());
  MockAdapter adapter(list_view);
  list_view->SetAdapter(&adapter);
  list_view->SetWidth(200.f);
  list_view->SetHeight(50.f);
  for (int i = 0; i < 200; ++i) {
    adapter.height_list_.emplace_back(10.f);
  }
  list_view->Layout();
  list_view->invalidated_ = false;

  ListScroller* scroller = list_view->scroller_.get();
  EXPECT_EQ(scroller, nullptr);

  bool callback_called = false;
  list_view->ScrollToPosition(
      true, 100, 0.f, AlignTo::kMiddle, "",
      [&callback_called](uint32_t code, const std::string& unused) {
        EXPECT_EQ(code, static_cast<uint32_t>(LynxUIMethodResult::kSuccess));
        callback_called = true;
      });
  // For smooth scrolling, callback will be called after the scrolling finished.
  EXPECT_FALSE(callback_called);

  scroller = list_view->scroller_.get();
  EXPECT_NE(scroller, nullptr);
  EXPECT_TRUE(scroller->Scrolling());
  scroller->OnAnimation(fml::TimePoint::Now().ToEpochDelta().ToMillisecondsF());

  EXPECT_TRUE(list_view->NeedsLayout());
  list_view->Layout();

  // The first iteration is the "far-away" path. Will scroll to position 52.
  ListChildrenHelper* children_helper = list_view->children_helper_.get();
  EXPECT_EQ(children_helper->GetChildAt(4)->GetPosition(), 52);

  // Not reach the target position yet. But start to slow down.
  int last_pos = 52;
  const int step = 50 / 10;

  auto start_time = fml::TimePoint::Now().ToEpochDelta().ToMillisecondsF();
  scroller->OnAnimation(start_time);

#define TARGET_ITEM() \
  scroller->GetChildrenHelper()->FindChildByPosition(scroller->position_)

  ListItemViewHolder* target_item;
  // Will use this step until seeing the target position
  do {
    start_time += 30;
    EXPECT_EQ(last_pos + step, children_helper->GetChildAt(4)->GetPosition());
    last_pos = children_helper->GetChildAt(4)->GetPosition();
    EXPECT_TRUE(scroller->Scrolling());
    target_item = TARGET_ITEM();
    scroller->OnAnimation(start_time);
  } while (target_item == nullptr);

  float last_distance = scroller->DistanceToTarget(TARGET_ITEM());
  while (scroller->Scrolling()) {
    start_time += 16;
    scroller->OnAnimation(start_time);
    EXPECT_LT(scroller->DistanceToTarget(TARGET_ITEM()), last_distance);
    last_distance = scroller->DistanceToTarget(TARGET_ITEM());
  }

  EXPECT_EQ(children_helper->GetChildAt(2)->GetPosition(), 100);
  EXPECT_TRUE(callback_called);
}

// Data change during smooth scrolling
TEST_F_UI(ListLayoutManagerLinearTest,
          DISABLED_ScrollToPositionSmoothDataChange) {
  MockListView* list_view = new MockListView(page_.get());
  MockAdapter adapter(list_view);
  list_view->SetAdapter(&adapter);
  list_view->SetWidth(200.f);
  list_view->SetHeight(50.f);
  for (int i = 0; i < 200; ++i) {
    adapter.height_list_.emplace_back(10.f);
  }
  list_view->Layout();
  list_view->invalidated_ = false;

  list_view->ScrollToPosition(true, 190, 0.f, AlignTo::kStart, "", nullptr);

  // Case 1: remove items during scrolling.
  ListScroller* scroller = list_view->scroller_.get();
  ListChildrenHelper* children_helper = list_view->children_helper_.get();
  while (children_helper->GetChildAt(4)->GetPosition() < 100) {
    scroller->OnAnimation(
        fml::TimePoint::Now().ToEpochDelta().ToMillisecondsF() + 16);
  }

  adapter.height_list_.resize(100);
  adapter.NotifyItemRangeRemoved(100, 100);

  // Will Stop earlier.
  scroller->OnAnimation(fml::TimePoint::Now().ToEpochDelta().ToMillisecondsF() +
                        540);
  EXPECT_FALSE(scroller->Scrolling());

  // Case 2: add items during scrolling.
  list_view->ScrollToPosition(true, 0, 0.f, AlignTo::kStart, "", nullptr);
  // The first iteration uses "far-away" path.
  // Then it starts to scroll with step 5-positions/iteration. If we add 5 items
  // to the list(which will "push" the current position down by 5 items), the
  // scrolling should keep going.
  for (int i = 0; i < 30; ++i) {
    for (int j = 0; j < 5; ++j) {
      // Should insert to the font. But it doesn't matter in our case because
      // all items have the same value.
      adapter.height_list_.push_back(10.f);
    }
    adapter.NotifyItemRangeInserted(0, 5);
    scroller->OnAnimation(
        fml::TimePoint::Now().ToEpochDelta().ToMillisecondsF());
    EXPECT_TRUE(scroller->Scrolling());
  }

  while (scroller->Scrolling()) {
    scroller->OnAnimation(
        fml::TimePoint::Now().ToEpochDelta().ToMillisecondsF());
  }
}

}  // namespace clay
