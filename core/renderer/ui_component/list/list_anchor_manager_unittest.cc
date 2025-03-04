// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/ui_component/list/list_anchor_manager.h"

#include "base/include/fml/memory/ref_ptr.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_component/list/default_list_adapter.h"
#include "core/renderer/ui_component/list/linear_layout_manager.h"
#include "core/renderer/ui_component/list/list_container_impl.h"
#include "core/renderer/ui_component/list/list_orientation_helper.h"
#include "core/renderer/ui_component/list/testing/mock_diff_result.h"
#include "core/renderer/ui_component/list/testing/mock_list_element.h"
#include "core/renderer/ui_component/list/testing/utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/table.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class ListAnchorManagerTest : public ::testing::Test {
 public:
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  fml::RefPtr<list::MockListElement> list_element_ref;
  std::unique_ptr<ListContainerImpl> list_container;
  ListLayoutManager* layout_mananger;
  ListAnchorManager* anchor_manager;
  ListAdapter* adapter;
  ListChildrenHelper* children_helper;

 protected:
  ListAnchorManagerTest() = default;
  ~ListAnchorManagerTest() override = default;
  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    manager->SetConfig(config);

    lepus::Value component_at_index;
    lepus::Value enqueue_component;
    lepus::Value component_at_indexes;
    list_element_ref = fml::AdoptRef<list::MockListElement>(
        new list::MockListElement(manager.get(), "list", component_at_index,
                                  enqueue_component, component_at_indexes));
    list_element_ref->width_ = 400;
    list_element_ref->height_ = 500;
    list_container =
        std::make_unique<ListContainerImpl>(list_element_ref.get());
    layout_mananger = list_container->list_layout_manager_.get();
    adapter = list_container->list_adapter_.get();
    children_helper = list_container->list_children_helper_.get();
    anchor_manager = layout_mananger->list_anchor_manager_.get();
    list::DiffResult diff_result{
        .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                      "I_8", "J_9"},
        .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
        .estimated_height_pxs = {100, 100, 100, 100, 100, 100, 100, 100, 100,
                                 100},
    };
    adapter->UpdateDataSource(lepus_value(diff_result.GenerateDiffResult()));
    adapter->UpdateItemHolderToLatest(children_helper);
    anchor_manager->UpdateDiffAnchorReference();
  }
};  // ListAnchorManagerTest

TEST_F(ListAnchorManagerTest, IsValidInitialScrollIndex) {
  anchor_manager->SetInitialScrollIndex(1);
  EXPECT_TRUE(anchor_manager->IsValidInitialScrollIndex());
  anchor_manager->SetInitialScrollIndex(101);
  EXPECT_FALSE(anchor_manager->IsValidInitialScrollIndex());
}

TEST_F(ListAnchorManagerTest, FindAnchor1) {
  layout_mananger->SetContentOffset(0);
  ListAnchorManager::AnchorInfo anchor_info;
  anchor_manager->RetrieveAnchorInfoBeforeLayout(anchor_info,
                                                 list::kInvalidIndex);
  EXPECT_TRUE(anchor_info.valid_);
  EXPECT_EQ(anchor_info.index_, 0);
}

TEST_F(ListAnchorManagerTest, FindAnchor2) {
  layout_mananger->SetContentOffset(0);
  ListAnchorManager::AnchorInfo anchor_info_0;
  anchor_manager->RetrieveAnchorInfoBeforeLayout(anchor_info_0,
                                                 list::kInvalidIndex);
  EXPECT_TRUE(anchor_info_0.valid_);
  EXPECT_EQ(anchor_info_0.index_, 0);

  list::DiffResult diff_result{
      .item_keys = {},
      .removal = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
  };
  adapter->UpdateDataSource(lepus_value(diff_result.GenerateDiffResult()));
  adapter->UpdateItemHolderToLatest(children_helper);

  ListAnchorManager::AnchorInfo anchor_info_1;
  anchor_manager->RetrieveAnchorInfoBeforeLayout(anchor_info_1,
                                                 list::kInvalidIndex);
  EXPECT_FALSE(anchor_info_1.valid_);
  EXPECT_EQ(anchor_info_1.index_, list::kInvalidIndex);
}

TEST_F(ListAnchorManagerTest, FindAnchor3) {
  layout_mananger->SetContentOffset(0);
  ListAnchorManager::AnchorInfo anchor_info_0;
  anchor_manager->RetrieveAnchorInfoBeforeLayout(anchor_info_0,
                                                 list::kInvalidIndex);
  EXPECT_TRUE(anchor_info_0.valid_);
  EXPECT_EQ(anchor_info_0.index_, 0);

  layout_mananger->LayoutInvalidItemHolder(0);
  children_helper->ForEachChild([](ItemHolder* item_holder) {
    if (item_holder->index_ < 5) {
      item_holder->dirty_ = false;
      item_holder->operation_id_ = 0;
      item_holder->element_ = reinterpret_cast<Element*>(1);
    }
    return false;
  });
  layout_mananger->content_size_ = layout_mananger->GetTargetContentSize();
  layout_mananger->content_offset_ = 150.f;
  children_helper->UpdateOnScreenChildren(
      layout_mananger->list_orientation_helper_.get(), 150.f);

  list::DiffResult diff_result{
      .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                    "I_8", "J_9"},
      .update_from = {0, 1},
      .update_to = {0, 1},
  };
  adapter->UpdateDataSource(lepus_value(diff_result.GenerateDiffResult()));
  adapter->UpdateItemHolderToLatest(children_helper);
  ListAnchorManager::AnchorInfo anchor_info_1;
  anchor_manager->RetrieveAnchorInfoBeforeLayout(anchor_info_1,
                                                 list::kInvalidIndex);
  EXPECT_TRUE(anchor_info_1.valid_);
  EXPECT_EQ(anchor_info_1.index_, 2);
  EXPECT_TRUE(base::FloatsEqual(anchor_info_1.start_alignment_delta_, 50.f));
}

TEST_F(ListAnchorManagerTest, RetrieveAnchorInfoBeforeLayout1) {
  ListAnchorManager::AnchorInfo anchor_info;
  int scroll_index = 1;
  anchor_manager->SetInitialScrollIndex(scroll_index);
  EXPECT_TRUE(anchor_manager->initial_scroll_index_status_ ==
              list::InitialScrollIndexStatus::kSet);
  anchor_manager->RetrieveAnchorInfoBeforeLayout(anchor_info,
                                                 list::kInvalidIndex);
  EXPECT_TRUE(anchor_info.valid_);
  EXPECT_EQ(anchor_info.index_, scroll_index);
  EXPECT_EQ(anchor_info.start_alignment_delta_, 0);
  anchor_manager->MarkScrolledInitialScrollIndex();
  EXPECT_TRUE(anchor_manager->initial_scroll_index_status_ ==
              list::InitialScrollIndexStatus::kScrolled);
}

TEST_F(ListAnchorManagerTest, RetrieveAnchorInfoBeforeLayout2) {
  ListAnchorManager::AnchorInfo anchor_info;
  int scroll_to_position = 5;
  float scrolling_offset = 10;
  anchor_manager->ResetScrollInfo();
  anchor_manager->InvalidateScrollInfoPosition();
  anchor_manager->InitScrollToPositionParam(
      adapter->GetItemHolderForIndex(scroll_to_position), scroll_to_position,
      scrolling_offset, 1, false);
  anchor_manager->CalculateTargetScrollingOffset(
      adapter->GetItemHolderForIndex(scroll_to_position));
  anchor_manager->RetrieveAnchorInfoBeforeLayout(anchor_info,
                                                 list::kInvalidIndex);
  EXPECT_TRUE(anchor_info.valid_);
  EXPECT_EQ(anchor_info.index_, scroll_to_position);
  EXPECT_EQ(anchor_info.start_alignment_delta_, 0);
  EXPECT_NE(anchor_info.item_holder_, nullptr);
  EXPECT_FALSE(anchor_manager->IsValidSmoothScrollInfo());
}

TEST_F(ListAnchorManagerTest, AdjustContentOffsetWithAnchor1) {
  ListAnchorManager::AnchorInfo anchor_info;
  layout_mananger->LayoutInvalidItemHolder(0);
  layout_mananger->content_size_ = layout_mananger->GetTargetContentSize();
  anchor_info.valid_ = true;
  anchor_info.index_ = 2;
  anchor_info.item_holder_ =
      list_container->GetItemHolderForIndex(anchor_info.index_);
  anchor_info.start_offset_ = 200.f;
  anchor_info.start_alignment_delta_ = 20.f;
  anchor_manager->AdjustContentOffsetWithAnchor(
      anchor_info, layout_mananger->content_offset_);
  EXPECT_TRUE(base::FloatsEqual(
      layout_mananger->content_offset_,
      anchor_info.start_offset_ - anchor_info.start_alignment_delta_));
}

TEST_F(ListAnchorManagerTest, AdjustContentOffsetWithAnchor2) {
  ListAnchorManager::AnchorInfo anchor_info;
  anchor_info.Reset();
  float content_offset = 100.f;
  layout_mananger->LayoutInvalidItemHolder(0);
  layout_mananger->content_size_ = layout_mananger->GetTargetContentSize();
  anchor_manager->AdjustContentOffsetWithAnchor(anchor_info, content_offset);
  EXPECT_TRUE(
      base::FloatsEqual(layout_mananger->content_offset_, content_offset));
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
