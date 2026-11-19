// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include <algorithm>

#include "core/list/decoupled_linear_layout_manager.h"
#include "core/list/decoupled_list_container_impl.h"
#include "core/list/testing/fiber_data_source.h"
#include "core/list/testing/mock_list_element.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace list {

using ::testing::_;

// Managed mode overlays the active sticky header's display position during
// FlushPatching, after the regular pass has written every bound item at its
// flow position. Legacy mode keeps the platform-side sticky behavior.
//
// Flow layout of the fixture: eight items of height {100, 40, 60, 60, 40,
// 100, 100, 100} with sticky-top items at index 1 ([100, 140]) and index 4
// ([260, 300]) in a viewport of height 100.
class ListStickyTest : public ::testing::TestWithParam<bool> {
 public:
  void SetUp() override {
    factory_ = std::make_shared<pub::PubValueFactoryDefault>();
    element_ = std::make_unique<MockListElement>();
    element_->height_ = 100.f;
    container_ = std::make_unique<ListContainerImpl>(element_.get(), factory_);
    container_->OnAttachToElementManager();
    if (GetParam()) {
      container_->EnableEngineManagedScroll();
    }
    EXPECT_EQ(container_->ResolveAttribute(*factory_->CreateString(kPropSticky),
                                           *factory_->CreateBool(true)),
              !GetParam());
    EXPECT_TRUE(container_->sticky_enabled());
    manager_ = container_->list_layout_manager();
    adapter_ = container_->list_adapter();
    testing::FiberDataSource source;
    for (int i = 0; i < kCount; ++i) {
      source.insert_action_.insert_ops_.push_back(
          {i, std::to_string(i), sizes_[i], false, i == 1 || i == 4, false,
           true});
    }
    pub::ValueImplLepus value{lepus::Value(source.Resolve())};
    container_->ResolveAttribute(
        *factory_->CreateString(kPropFiberUpdateListInfo), value);
    container_->PropsUpdateFinish();
    ON_CALL(*element_, ComponentAtIndex(_, _, _))
        .WillByDefault([this](uint32_t index, int64_t operation, bool) {
          requests_.push_back(static_cast<int>(index));
          if (static_cast<int>(index) == pending_index_) {
            pending_operation_ = operation;
          } else {
            Finish(static_cast<int>(index), operation);
          }
          return true;
        });
    manager_->OnLayoutChildren();
  }

  void Finish(int index, int64_t operation) {
    auto* holder = container_->GetItemHolderForIndex(index);
    const auto key = holder->item_key();
    if (!element_->GetListItemElement(key)) {
      auto item = std::make_unique<MockListItemElement>(100 + index);
      item->height_ = sizes_[index];
      element_->AddListItemElement(key, std::move(item));
    }
    auto pipeline = std::make_shared<tasm::PipelineOptions>();
    pipeline->operation_id = operation;
    container_->FinishBindItemHolder(element_->GetListItemElement(key),
                                     pipeline);
  }

  void MoveTo(float offset) {
    if (GetParam()) {
      ASSERT_TRUE(container_->ScrollByDelta(offset - manager_->content_offset())
                      .state.valid);
    } else {
      manager_->SetContentOffset(offset);
      manager_->OnLayoutChildren();
    }
  }

  static constexpr int kCount = 8;
  int sizes_[kCount] = {100, 40, 60, 60, 40, 100, 100, 100};
  std::shared_ptr<pub::PubValueFactoryDefault> factory_;
  std::unique_ptr<MockListElement> element_;
  std::unique_ptr<ListContainerImpl> container_;
  ListLayoutManager* manager_{nullptr};
  ListAdapter* adapter_{nullptr};
  int pending_index_{-1};
  int64_t pending_operation_{0};
  std::vector<int> requests_;
};

TEST_P(ListStickyTest, StickyAndPushOffOverlayOnlyTheDisplayOutput) {
  auto* header = container_->GetItemHolderForIndex(1);
  const float content = manager_->content_size();
  MoveTo(180.f);
  EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_,
                  GetParam() ? 180.f : 100.f);
  const auto before = container_->GetScrollState();
  MoveTo(240.f);
  const auto pushed = container_->GetScrollState();
  EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_,
                  GetParam() ? 220.f : 100.f);
  EXPECT_FLOAT_EQ(header->top(), 100.f);
  EXPECT_FLOAT_EQ(header->left(), 0.f);
  EXPECT_FLOAT_EQ(container_->GetItemHolderForIndex(3)->top(), 200.f);
  EXPECT_FLOAT_EQ(manager_->content_size(), content);
  if (GetParam()) {
    EXPECT_EQ(before.first_visible_index, 2);
    EXPECT_FLOAT_EQ(before.first_visible_scroll_offset, 40.f);
    EXPECT_EQ(pushed.first_visible_index, 3);
    EXPECT_FLOAT_EQ(pushed.first_visible_scroll_offset, 40.f);
    ASSERT_TRUE(before.sticky_item.IsValid());
    EXPECT_EQ(before.sticky_item, (ListStickyItem{1, "1", 180.f, 40.f}));
    ASSERT_TRUE(pushed.sticky_item.IsValid());
    EXPECT_EQ(pushed.sticky_item, (ListStickyItem{1, "1", 220.f, 40.f}));
    EXPECT_GT(pushed.revision, before.revision);
  } else {
    EXPECT_FALSE(before.sticky_item.IsValid());
    EXPECT_FALSE(pushed.sticky_item.IsValid());
    EXPECT_EQ(pushed.revision, before.revision);
  }
}

TEST_P(ListStickyTest, ScrollBackAndHandoverRestoreFlowPositions) {
  MoveTo(240.f);
  EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_,
                  GetParam() ? 220.f : 100.f);
  MoveTo(280.f);
  if (GetParam()) {
    EXPECT_FALSE(
        adapter_->IsFinishedBinding(container_->GetItemHolderForIndex(1)));
    EXPECT_FLOAT_EQ(element_->GetListItemElement("4")->top_, 280.f);
    const auto state = container_->GetScrollState();
    ASSERT_TRUE(state.sticky_item.IsValid());
    EXPECT_EQ(state.sticky_item, (ListStickyItem{4, "4", 280.f, 40.f}));
  } else {
    EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 100.f);
    EXPECT_FLOAT_EQ(element_->GetListItemElement("4")->top_, 260.f);
  }
  MoveTo(50.f);
  EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 100.f);
  EXPECT_FALSE(container_->GetScrollState().sticky_item.IsValid());
}

TEST_P(ListStickyTest, UnboundActiveHeaderIsSkippedUntilFinished) {
  pending_index_ = 1;
  MoveTo(180.f);
  EXPECT_NE(pending_operation_, 0);
  EXPECT_FALSE(container_->GetScrollState().sticky_item.IsValid());
  Finish(1, pending_operation_);
  pending_index_ = -1;
  manager_->OnLayoutChildren();
  if (GetParam()) {
    const auto state = container_->GetScrollState();
    ASSERT_TRUE(state.sticky_item.IsValid());
    EXPECT_EQ(state.sticky_item, (ListStickyItem{1, "1", 180.f, 40.f}));
    EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 180.f);
  } else {
    EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 100.f);
  }
}

TEST_P(ListStickyTest, OffscreenPushOffPeerIsNotBound) {
  MoveTo(180.f);
  auto* header = container_->GetItemHolderForIndex(1);
  element_->GetListItemElement("1")->height_ = 180.f;
  header->UpdateLayoutFromItemDelegate(element_->GetListItemElement("1"));
  manager_->OnLayoutChildren();
  auto* next = container_->GetItemHolderForIndex(4);
  EXPECT_FLOAT_EQ(next->top(), 400.f);
  requests_.clear();
  MoveTo(280.f);
  EXPECT_FALSE(adapter_->IsFinishedBinding(next));
  EXPECT_EQ(requests_.end(), std::find(requests_.begin(), requests_.end(), 4));
  if (GetParam()) {
    EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 220.f);
    const auto state = container_->GetScrollState();
    ASSERT_TRUE(state.sticky_item.IsValid());
    EXPECT_EQ(state.sticky_item, (ListStickyItem{1, "1", 220.f, 180.f}));
  } else {
    EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 100.f);
  }
}

TEST_P(ListStickyTest, RemeasureRoleRemovalDisableAndIndexShift) {
  MoveTo(240.f);
  const auto initial_revision = container_->GetScrollState().revision;
  auto* header = container_->GetItemHolderForIndex(1);
  element_->GetListItemElement("1")->height_ = 60.f;
  header->UpdateLayoutFromItemDelegate(element_->GetListItemElement("1"));
  manager_->OnLayoutChildren();
  EXPECT_FLOAT_EQ(container_->GetItemHolderForIndex(4)->top(), 280.f);
  if (GetParam()) {
    const auto remeasured = container_->GetScrollState();
    ASSERT_TRUE(remeasured.sticky_item.IsValid());
    EXPECT_EQ(remeasured.sticky_item, (ListStickyItem{1, "1", 220.f, 60.f}));
    EXPECT_GT(remeasured.revision, initial_revision);
    EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 220.f);
  }

  // Insert an item ahead of the pinned header before disabling sticky so the
  // restore exercises WeakPtr holder identity rather than a cached index.
  testing::FiberDataSource insert_front;
  insert_front.insert_action_.insert_ops_.push_back(
      {0, "inserted", 30, false, false, false, true});
  pub::ValueImplLepus value{lepus::Value(insert_front.Resolve())};
  container_->ResolveAttribute(
      *factory_->CreateString(kPropFiberUpdateListInfo), value);
  container_->PropsUpdateFinish();
  ASSERT_EQ(container_->GetItemHolderForIndex(2)->item_key(), "1");
  container_->ResolveAttribute(*factory_->CreateString(kPropSticky),
                               *factory_->CreateBool(false));
  EXPECT_FALSE(container_->sticky_enabled());
  EXPECT_FALSE(container_->GetScrollState().sticky_item.IsValid());
  EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 100.f);

  container_->ResolveAttribute(*factory_->CreateString(kPropSticky),
                               *factory_->CreateBool(true));
  manager_->OnLayoutChildren();
  container_->EnableEngineManagedScroll();
  EXPECT_TRUE(container_->sticky_enabled());
  manager_->OnLayoutChildren();
  ASSERT_TRUE(container_->GetScrollState().sticky_item.IsValid());
}

TEST_P(ListStickyTest, FullViewportStickyHeaderRemeasurePreservesOffset) {
  if (!GetParam()) {
    return;
  }
  auto* header = container_->GetItemHolderForIndex(1);
  MoveTo(100.f);
  sizes_[1] = 220;
  element_->GetListItemElement("1")->height_ = 220.f;
  header->UpdateLayoutFromItemDelegate(element_->GetListItemElement("1"));
  manager_->OnLayoutChildren();

  MoveTo(150.f);
  ASSERT_EQ(container_->list_children_helper()->on_screen_children().size(),
            1u);
  ASSERT_EQ((*container_->list_children_helper()->on_screen_children().begin())
                ->index(),
            1);

  sizes_[1] = 240;
  element_->GetListItemElement("1")->height_ = 240.f;
  header->UpdateLayoutFromItemDelegate(element_->GetListItemElement("1"));
  manager_->OnLayoutChildren();

  const auto state = container_->GetScrollState();
  EXPECT_FLOAT_EQ(manager_->content_offset(), 150.f);
  EXPECT_FLOAT_EQ(state.offset, 150.f);
  ASSERT_TRUE(state.sticky_item.IsValid());
  EXPECT_EQ(state.sticky_item, (ListStickyItem{1, "1", 150.f, 240.f}));
  EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 150.f);

  // Scrolling while the tall sticky header still exclusively fills the
  // viewport must continue to find an anchor and advance cleanly.
  const auto scrolled = container_->ScrollByDelta(20.f);
  EXPECT_FLOAT_EQ(scrolled.consumed, 20.f);
  EXPECT_FLOAT_EQ(scrolled.state.offset, 170.f);
  EXPECT_EQ(scrolled.state.sticky_item, (ListStickyItem{1, "1", 170.f, 240.f}));
}

TEST_P(ListStickyTest, ScrollRebindStickyHeaderReflowsGeometry) {
  if (!GetParam()) {
    return;
  }
  MoveTo(180.f);
  auto* first = container_->GetItemHolderForIndex(1);
  ASSERT_TRUE(adapter_->IsFinishedBinding(first));

  MoveTo(280.f);
  ASSERT_FALSE(adapter_->IsFinishedBinding(first));

  sizes_[1] = 80;
  if (auto* item = element_->GetListItemElement("1")) {
    item->height_ = 80.f;
  }

  const auto scrolled = container_->ScrollByDelta(-40.f);
  EXPECT_TRUE(scrolled.state.valid);
  EXPECT_FLOAT_EQ(scrolled.consumed, -40.f);
  EXPECT_FLOAT_EQ(first->height(), 80.f);
  EXPECT_FLOAT_EQ(container_->GetItemHolderForIndex(4)->top(), 300.f);
  EXPECT_FLOAT_EQ(manager_->content_size(), 640.f);
  EXPECT_FLOAT_EQ(scrolled.state.content, 640.f);
  ASSERT_TRUE(scrolled.state.sticky_item.IsValid());
  EXPECT_EQ(scrolled.state.sticky_item, (ListStickyItem{1, "1", 220.f, 80.f}));
  EXPECT_FLOAT_EQ(element_->GetListItemElement("1")->top_, 220.f);
}

INSTANTIATE_TEST_SUITE_P(ManagedAndLegacy, ListStickyTest, ::testing::Bool());

// The first layout can land past a header that has never been measured. The
// header must be bound, measured and pinned in that same layout.
class ListStickyFirstScreenTest : public ::testing::TestWithParam<int> {};

TEST_P(ListStickyFirstScreenTest, InitialIndexPinsEarlierUnmeasuredHeader) {
  static constexpr int kSizes[] = {100, 40, 60, 60, 40, 100, 100, 100};
  auto factory = std::make_shared<pub::PubValueFactoryDefault>();
  auto element = std::make_unique<MockListElement>();
  element->height_ = 100.f;
  ListContainerImpl container(element.get(), factory);
  container.OnAttachToElementManager();
  container.EnableEngineManagedScroll();
  container.ResolveAttribute(*factory->CreateString(kPropSticky),
                             *factory->CreateBool(true));
  container.ResolveAttribute(*factory->CreateString(kPropInitialScrollIndex),
                             *factory->CreateNumber(6));
  testing::FiberDataSource source;
  for (int i = 0; i < 8; ++i) {
    source.insert_action_.insert_ops_.push_back(
        {i, std::to_string(i), GetParam(), false, i == 1 || i == 4, false,
         true});
  }
  pub::ValueImplLepus value{lepus::Value(source.Resolve())};
  container.ResolveAttribute(*factory->CreateString(kPropFiberUpdateListInfo),
                             value);
  container.PropsUpdateFinish();
  std::vector<int> bound;
  ON_CALL(*element, ComponentAtIndex(_, _, _))
      .WillByDefault([&](uint32_t index, int64_t operation, bool) {
        bound.push_back(static_cast<int>(index));
        const auto key = container.GetItemHolderForIndex(index)->item_key();
        if (!element->GetListItemElement(key)) {
          auto item = std::make_unique<MockListItemElement>(100 + index);
          item->height_ = kSizes[index];
          element->AddListItemElement(key, std::move(item));
        }
        auto pipeline = std::make_shared<tasm::PipelineOptions>();
        pipeline->operation_id = operation;
        container.FinishBindItemHolder(element->GetListItemElement(key),
                                       pipeline);
        return true;
      });
  container.list_layout_manager()->OnLayoutChildren();

  const auto state = container.GetScrollState();
  EXPECT_NE(bound.end(), std::find(bound.begin(), bound.end(), 4));
  EXPECT_EQ(state.first_visible_index, 6);
  EXPECT_FLOAT_EQ(state.first_visible_scroll_offset, 0.f);
  EXPECT_FLOAT_EQ(container.GetItemHolderForIndex(6)->top(), state.offset);
  EXPECT_EQ(state.sticky_item, (ListStickyItem{4, "4", state.offset, 40.f}));
  EXPECT_FLOAT_EQ(element->GetListItemElement("4")->top_, state.offset);
}

INSTANTIATE_TEST_SUITE_P(Estimates, ListStickyFirstScreenTest,
                         ::testing::Values(0, 40, 100));

TEST(ListStickyPlatformTest, ModeSwitchSyncsPlatformSticky) {
  auto factory = std::make_shared<pub::PubValueFactoryDefault>();
  MockListElement element;
  ListContainerImpl container(&element, factory);
  auto set_sticky = [&](bool value) {
    return container.ResolveAttribute(*factory->CreateString(kPropSticky),
                                      *factory->CreateBool(value));
  };
  using Props = std::vector<std::pair<std::string, bool>>;

  // Native sticky reaches the platform directly, then managed mode turns the
  // platform implementation off.
  EXPECT_TRUE(set_sticky(true));
  EXPECT_TRUE(element.flushed_bool_props_.empty());
  container.EnableEngineManagedScroll();
  EXPECT_EQ(element.flushed_bool_props_, (Props{{kPropSticky, false}}));
  // Managed updates are intercepted and the platform already has false.
  EXPECT_FALSE(set_sticky(false));
  EXPECT_EQ(element.flushed_bool_props_.size(), 1u);
  EXPECT_FALSE(set_sticky(true));
  EXPECT_EQ(element.flushed_bool_props_.size(), 1u);
}

}  // namespace list
}  // namespace lynx
