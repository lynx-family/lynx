// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_component/list/list_container_impl.h"
#include "core/renderer/ui_component/list/testing/mock_diff_result.h"
#include "core/renderer/ui_component/list/testing/mock_list_element.h"
#include "core/renderer/ui_component/list/testing/utils.h"
#include "core/shell/tasm_operation_queue.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class ListAdapterTest : public ::testing::Test {
 public:
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  fml::RefPtr<list::MockListElement> list_element_ref;
  std::unique_ptr<ListContainerImpl> list_container;
  ListChildrenHelper* list_children_helper;

 protected:
  ListAdapterTest() = default;
  ~ListAdapterTest() override = default;
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
    list_container =
        std::make_unique<ListContainerImpl>(list_element_ref.get());
    list_children_helper = list_container->list_children_helper();
  }
};  // ListAdapterTest

TEST_F(ListAdapterTest, DiffCase) {
  list::DiffResult diff_result{
      .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                    "I_8"},
      .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .estimated_height_pxs = {100, 100, 100, 100, 100, 100, 100, 100, 100},
      .estimated_main_axis_size_pxs = {100, 100, 100, 100, 100, 100, 100, 100,
                                       100},
      .sticky_tops = {0, 1},
      .sticky_bottoms = {7, 8},
      .full_spans = {0, 1, 7, 8},
  };
  list_container->list_adapter_->UpdateDataSource(
      lepus_value(diff_result.GenerateDiffResult()));
  EXPECT_EQ(
      list_container->list_adapter_->list_adapter_helper()->item_keys().size(),
      diff_result.GetItemCount());
  EXPECT_EQ(list_container->list_adapter_->list_adapter_helper()
                ->item_key_map()
                .size(),
            diff_result.GetItemCount());
  EXPECT_EQ(list_container->list_adapter_->GetDataCount(),
            diff_result.GetItemCount());
  EXPECT_EQ(list_container->list_adapter_->list_adapter_helper()
                ->estimated_heights_px()
                .size(),
            diff_result.GetItemCount());
  EXPECT_EQ(list_container->list_adapter_->list_adapter_helper()
                ->estimated_sizes_px()
                .size(),
            diff_result.GetItemCount());
  EXPECT_EQ(
      list_container->list_adapter_->list_adapter_helper()->full_spans().size(),
      diff_result.full_spans.size());
  EXPECT_EQ(list_container->list_adapter_->list_adapter_helper()
                ->sticky_tops()
                .size(),
            diff_result.sticky_tops.size());
  EXPECT_EQ(list_container->list_adapter_->GetStickyTops().size(),
            diff_result.sticky_tops.size());
  EXPECT_EQ(list_container->list_adapter_->list_adapter_helper()
                ->sticky_bottoms()
                .size(),
            diff_result.sticky_bottoms.size());
  EXPECT_EQ(list_container->list_adapter_->GetStickyBottoms().size(),
            diff_result.sticky_bottoms.size());
  EXPECT_TRUE(list_container->list_adapter_->HasFullSpanItems());
  list_container->list_adapter_->UpdateItemHolderToLatest(list_children_helper);
  for (int index = 0; index < static_cast<int>(diff_result.GetItemCount());
       ++index) {
    ItemHolder* item_holder =
        list_container->list_adapter_->GetItemHolderForIndex(index);
    EXPECT_NE(item_holder, nullptr);
    if (index == 0 || index == 1) {
      EXPECT_TRUE(item_holder->sticky_top());
    } else if (index == 7 || index == 8) {
      EXPECT_TRUE(item_holder->sticky_bottom());
    }
  }
}

TEST_F(ListAdapterTest, UpdateAnchorRefItemTest) {
  list_container->search_ref_anchor_strategy_ =
      list::SearchRefAnchorStrategy::kToStart;
  list::InsertAction insert_action;
  list::RemoveAction remove_action;
  list::UpdateAction update_action;
  insert_action = {.insert_ops_ = {
                       {.position_ = 0, "A_0", 100, false, false, false, false},
                       {.position_ = 1, "B_1", 100, false, false, false, false},
                       {.position_ = 2, "C_2", 100, false, false, false, false},
                       {.position_ = 3, "D_3", 100, false, false, false, false},
                       {.position_ = 4, "E_4", 100, false, false, false, false},
                       {.position_ = 5, "F_5", 100, false, false, false, false},
                       {.position_ = 6, "G_6", 100, false, false, false, false},
                       {.position_ = 7, "H_7", 100, false, false, false, false},
                       {.position_ = 8, "I_8", 100, false, false, false, false},
                       {.position_ = 9, "J_9", 100, false, false, false, false},
                   }};
  list::FiberDiffResult fiber_diff_result_0{
      .insert_action_ = insert_action,
  };
  list_container->list_adapter_->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_0.Resolve()));
  list_container->list_adapter_->UpdateItemHolderToLatest(list_children_helper);

  // init on screen children: E_4, F_5, G_6, H_7
  list_children_helper->on_screen_children_.insert(
      list_container->GetItemHolderForIndex(4));
  list_children_helper->on_screen_children_.insert(
      list_container->GetItemHolderForIndex(5));
  list_children_helper->on_screen_children_.insert(
      list_container->GetItemHolderForIndex(6));
  list_children_helper->on_screen_children_.insert(
      list_container->GetItemHolderForIndex(7));

  // remove C_2, E_4, F_5, H_7
  // insert New_A_0, New_B_1, New_C_2, New_D_3
  insert_action = {
      .insert_ops_ = {
          {.position_ = 2, "New_A_0", 100, false, false, false, false},
          {.position_ = 3, "New_B_1", 100, false, false, false, false},
          {.position_ = 4, "New_C_2", 100, false, false, false, false},
          {.position_ = 5, "New_D_3", 100, false, false, false, false},
      }};
  remove_action = {
      .remove_ops_ = {2, 4, 5, 7},
  };
  list::FiberDiffResult fiber_diff_result_1{
      .insert_action_ = insert_action,
      .remove_action_ = remove_action,
  };
  list_container->list_adapter_->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_1.Resolve()));
  list_container->list_adapter_->UpdateItemHolderToLatest(list_children_helper);

  ItemHolder* holder_C2 =
      list_container->list_adapter_->item_holder_map_->at("C_2").get();
  ItemHolder* holder_E4 =
      list_container->list_adapter_->item_holder_map_->at("E_4").get();
  ItemHolder* holder_F5 =
      list_container->list_adapter_->item_holder_map_->at("F_5").get();
  ItemHolder* holder_G6 =
      list_container->list_adapter_->item_holder_map_->at("G_6").get();
  ItemHolder* holder_H7 =
      list_container->list_adapter_->item_holder_map_->at("H_7").get();
  // C_2 is not on screen children set, so it has no anchor ref item.
  EXPECT_EQ(holder_C2->weak_anchor_ref().has_value(), false);
  // E_4 and F_5 are on screen children set and they are removed in this diff,
  // so they have anchor ref item with D_3.
  EXPECT_EQ(holder_E4->weak_anchor_ref().has_value(), true);
  EXPECT_TRUE(holder_E4->weak_anchor_ref().value()->item_key() == "D_3");
  EXPECT_EQ(holder_F5->weak_anchor_ref().has_value(), true);
  EXPECT_TRUE(holder_F5->weak_anchor_ref().value().get()->item_key() == "D_3");
  // G_6 is not removed in this diff, so it has no anchor ref item.
  EXPECT_EQ(holder_G6->weak_anchor_ref().has_value(), false);
  // H_7 is on screen children set and removed in this diff, so it has anchor
  // ref item with G_6.
  EXPECT_EQ(holder_H7->weak_anchor_ref().has_value(), true);
  EXPECT_TRUE(holder_H7->weak_anchor_ref().value().get()->item_key() == "G_6");

  // remove D_3
  remove_action = {
      .remove_ops_ = {6},
  };
  list::FiberDiffResult fiber_diff_result_2{
      .remove_action_ = remove_action,
  };
  list_container->list_adapter_->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_2.Resolve()));
  list_container->list_adapter_->UpdateItemHolderToLatest(list_children_helper);
  // D_3 is removed in this diff, so E_4 and F_5 should update anchor ref item
  // to New_D_3.
  EXPECT_EQ(holder_E4->weak_anchor_ref().has_value(), true);
  EXPECT_TRUE(holder_E4->weak_anchor_ref().value()->item_key() == "New_D_3");
  EXPECT_EQ(holder_F5->weak_anchor_ref().has_value(), true);
  EXPECT_TRUE(holder_F5->weak_anchor_ref().value()->item_key() == "New_D_3");

  // remove G_6
  remove_action = {
      .remove_ops_ = {6},
  };
  list::FiberDiffResult fiber_diff_result_3{
      .remove_action_ = remove_action,
  };
  list_container->list_adapter_->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_3.Resolve()));
  list_container->list_adapter_->UpdateItemHolderToLatest(list_children_helper);
  // G_6 is removed in this diff, so G_6 and H_7 should update anchor ref item
  // to New_G_6.
  EXPECT_EQ(holder_G6->weak_anchor_ref().has_value(), true);
  EXPECT_TRUE(holder_G6->weak_anchor_ref().value()->item_key() == "New_D_3");
  EXPECT_EQ(holder_H7->weak_anchor_ref().has_value(), true);
  EXPECT_TRUE(holder_H7->weak_anchor_ref().value()->item_key() == "New_D_3");
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
