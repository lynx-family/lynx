// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/ui_component/list/default_list_adapter.h"

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
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class MockDefaultListAdapter : public DefaultListAdapter {
 public:
  MockDefaultListAdapter(ListContainerImpl* list_container_impl,
                         Element* element)
      : DefaultListAdapter(list_container_impl, element) {}

  MOCK_METHOD(void, OnErrorOccurred, (lynx::base::LynxError error), (override));
};

class DefaultListAdapterTest : public ::testing::Test {
 public:
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  fml::RefPtr<list::MockListElement> list_element_ref;
  std::unique_ptr<ListContainerImpl> list_container;
  MockDefaultListAdapter* default_list_adapter{nullptr};
  ListChildrenHelper* list_children_helper{nullptr};

 protected:
  DefaultListAdapterTest() = default;
  ~DefaultListAdapterTest() override = default;
  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<::testing::NiceMock<lynx::tasm::ElementManager>>(
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
    auto mock_default_list_adapter = std::make_unique<MockDefaultListAdapter>(
        list_container.get(), list_element_ref.get());
    default_list_adapter = mock_default_list_adapter.get();
    list_container->list_adapter_ = std::move(mock_default_list_adapter);
    list_children_helper = list_container->list_children_helper();
    list_container->list_layout_manager_->InitLayoutManager(
        list_children_helper, list::Orientation::kVertical);
  }

 public:
  fml::RefPtr<Element> CreateComponentElement() {
    static int32_t base_css_id = 0;
    base::String component_id(std::to_string(++base_css_id));
    base::String entry_name("__ListItem__");
    base::String component_name("__ListItem__");
    base::String path("/index/components/ListItem");
    auto list_item_ref = manager->CreateFiberComponent(
        component_id, base_css_id, entry_name, component_name, path);
    list_element_ref->AddChildAt(list_item_ref, -1);
    return list_item_ref;
  }
};  // DefaultListAdapterTest

TEST_F(DefaultListAdapterTest, DiffCase0) {
  list::DiffResult diff_result{
      .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                    "I_8"},
      .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .estimated_height_pxs = {100, 100, 100, 100, 100, 100, 100, 100, 100},
  };
  default_list_adapter->UpdateDataSource(
      lepus_value(diff_result.GenerateDiffResult()));
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_keys().size(),
            diff_result.GetItemCount());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_key_map().size(),
            diff_result.GetItemCount());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()
                ->estimated_heights_px()
                .size(),
            diff_result.GetItemCount());
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper);
  for (int i = 0; i < static_cast<int>(diff_result.GetItemCount()); ++i) {
    ItemHolder* item_holder = default_list_adapter->GetItemHolderForIndex(i);
    EXPECT_NE(item_holder, nullptr);
    EXPECT_FALSE(default_list_adapter->IsRecycled(item_holder));
    EXPECT_FALSE(default_list_adapter->IsBinding(item_holder));
    EXPECT_FALSE(default_list_adapter->IsFinishedBinding(item_holder));
    EXPECT_TRUE(default_list_adapter->IsDirty(item_holder));
    EXPECT_FALSE(default_list_adapter->IsUpdated(item_holder));
    EXPECT_FALSE(default_list_adapter->IsRemoved(item_holder));
    EXPECT_TRUE(default_list_adapter->GetListItemElement(item_holder) ==
                nullptr);
  }
}

TEST_F(DefaultListAdapterTest, DiffCase1) {
  // before
  list::DiffResult diff_result_0{
      .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                    "I_8"},
      .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8},
  };
  default_list_adapter->UpdateDataSource(
      lepus_value(diff_result_0.GenerateDiffResult()));
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_keys().size(),
            diff_result_0.GetItemCount());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_key_map().size(),
            diff_result_0.GetItemCount());
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper);
  list_container->StartInterceptListElementUpdated();
  for (int index = 0; index < static_cast<int>(diff_result_0.GetItemCount());
       ++index) {
    ItemHolder* item_holder =
        default_list_adapter->GetItemHolderForIndex(index);
    if (item_holder) {
      // Bind
      EXPECT_TRUE(default_list_adapter->BindItemHolder(item_holder, index));
      EXPECT_FALSE(default_list_adapter->IsDirty(item_holder));
      EXPECT_TRUE(default_list_adapter->IsBinding(item_holder));
      EXPECT_FALSE(default_list_adapter->IsRecycled(item_holder));
      EXPECT_FALSE(default_list_adapter->IsFinishedBinding(item_holder));
      // Finish Bind
      auto pipeline = std::make_shared<PipelineOptions>();
      pipeline->operation_id =
          list::GenerateOperationId(list_element_ref->impl_id());
      auto list_item_ref = CreateComponentElement();
      default_list_adapter->OnFinishBindItemHolder(list_item_ref.get(),
                                                   pipeline);
      EXPECT_FALSE(default_list_adapter->IsBinding(item_holder));
      EXPECT_FALSE(default_list_adapter->IsRecycled(item_holder));
      EXPECT_TRUE(default_list_adapter->IsFinishedBinding(item_holder));
      EXPECT_FALSE(default_list_adapter->IsDirty(item_holder));
      EXPECT_TRUE(default_list_adapter->GetListItemElement(item_holder) !=
                  nullptr);
    }
  }
  list_container->StopInterceptListElementUpdated();

  // after
  list::DiffResult diff_result_1{
      .item_keys = {"New_A_0", "New_B_1", "New_C_2", "New_D_3", "E_4",
                    "New_F_5", "New_G_6", "New_H_7", "New_I_8"},
      .insertion = {0, 1, 2, 3, 5, 6, 7, 8},
      .removal = {0, 1, 2, 3, 5, 6, 7, 8},
  };
  default_list_adapter->UpdateDataSource(
      lepus_value(diff_result_1.GenerateDiffResult()));
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_keys().size(),
            diff_result_1.GetItemCount());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_key_map().size(),
            diff_result_1.GetItemCount());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->insertions().size(),
            diff_result_1.insertion.size());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->removals().size(),
            diff_result_1.removal.size());
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper);
  list_container->StartInterceptListElementUpdated();
  for (int index = 0; index < static_cast<int>(diff_result_1.GetItemCount());
       ++index) {
    ItemHolder* item_holder =
        default_list_adapter->GetItemHolderForIndex(index);
    if (item_holder) {
      if (index == 4) {
        // No need to Bind
        EXPECT_FALSE(default_list_adapter->BindItemHolder(item_holder, index));
      } else {
        // Bind
        EXPECT_TRUE(default_list_adapter->BindItemHolder(item_holder, index));
        auto pipeline = std::make_shared<PipelineOptions>();
        pipeline->operation_id =
            list::GenerateOperationId(list_element_ref->impl_id());
        auto list_item_ref = CreateComponentElement();
        default_list_adapter->OnFinishBindItemHolder(list_item_ref.get(),
                                                     pipeline);
        EXPECT_TRUE(default_list_adapter->GetListItemElement(item_holder) !=
                    nullptr);
      }
    }
  }
  list_container->StopInterceptListElementUpdated();
  default_list_adapter->RecycleRemovedItemHolders();
  EXPECT_EQ(default_list_adapter->item_holder_map()->size(),
            diff_result_1.GetItemCount());
}

TEST_F(DefaultListAdapterTest, DiffCase2) {
  // before
  list::DiffResult diff_result_0{
      .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                    "I_8"},
      .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8},
  };
  default_list_adapter->UpdateDataSource(
      lepus_value(diff_result_0.GenerateDiffResult()));
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper);

  // Trigger all item holders to bind.
  list_container->StartInterceptListElementUpdated();
  int raw_data_count = static_cast<int>(diff_result_0.GetItemCount());
  std::vector<std::shared_ptr<PipelineOptions>> pending_pipeline_vec;
  for (int index = 0; index < raw_data_count; ++index) {
    ItemHolder* item_holder =
        default_list_adapter->GetItemHolderForIndex(index);
    if (item_holder) {
      EXPECT_TRUE(default_list_adapter->BindItemHolder(item_holder, index));
      auto pipeline = std::make_shared<PipelineOptions>();
      pipeline->operation_id =
          list::GenerateOperationId(list_element_ref->impl_id());
      pending_pipeline_vec.emplace_back(pipeline);
    }
  }
  list_container->StopInterceptListElementUpdated();

  // The item holder with index < raw_data_count / 2 is finished bind.
  for (int index = 0; index < raw_data_count; ++index) {
    ItemHolder* item_holder =
        default_list_adapter->GetItemHolderForIndex(index);
    if (item_holder) {
      const auto& pipeline = pending_pipeline_vec[index];
      if (index < raw_data_count / 2) {
        auto list_item_ref = CreateComponentElement();
        default_list_adapter->OnFinishBindItemHolder(list_item_ref.get(),
                                                     pipeline);
        EXPECT_FALSE(default_list_adapter->IsBinding(item_holder));
        EXPECT_FALSE(default_list_adapter->IsRecycled(item_holder));
        EXPECT_TRUE(default_list_adapter->IsFinishedBinding(item_holder));
        EXPECT_FALSE(default_list_adapter->IsDirty(item_holder));
        EXPECT_TRUE(default_list_adapter->GetListItemElement(item_holder) !=
                    nullptr);
      } else {
        EXPECT_TRUE(default_list_adapter->IsBinding(item_holder));
        EXPECT_FALSE(default_list_adapter->IsRecycled(item_holder));
        EXPECT_FALSE(default_list_adapter->IsFinishedBinding(item_holder));
        EXPECT_FALSE(default_list_adapter->IsDirty(item_holder));
        EXPECT_TRUE(default_list_adapter->GetListItemElement(item_holder) ==
                    nullptr);
      }
    }
  }
  EXPECT_TRUE(
      static_cast<int>(
          default_list_adapter->binding_item_holder_weak_map_->size()) ==
      raw_data_count - raw_data_count / 2);

  // after
  list::DiffResult diff_result_1{
      .item_keys = {"New_A_0", "New_B_1", "New_C_2", "New_D_3", "New_E_4",
                    "New_F_5", "New_G_6", "New_H_7", "New_I_8"},
      .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .removal = {0, 1, 2, 3, 4, 5, 6, 7, 8},
  };
  default_list_adapter->UpdateDataSource(
      lepus_value(diff_result_1.GenerateDiffResult()));
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper);
  // Simulate destroy removed item holders.
  default_list_adapter->RecycleRemovedItemHolders();
  for (int index = 0; index < raw_data_count; ++index) {
    if (index >= raw_data_count / 2) {
      auto list_item_ref = CreateComponentElement();
      default_list_adapter->OnFinishBindItemHolder(list_item_ref.get(),
                                                   pending_pipeline_vec[index]);
      EXPECT_TRUE(list_item_ref->impl_id() ==
                  list_element_ref->enqueue_component_sign_);
    }
  }
  EXPECT_TRUE(default_list_adapter->binding_item_holder_weak_map_->empty());
}

TEST_F(DefaultListAdapterTest, FiberDiffCase0) {
  list::InsertAction insert_action{
      .insert_ops_ = {
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
  list::FiberDiffResult fiber_diff_result{
      .insert_action_ = insert_action,
  };
  default_list_adapter->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result.Resolve()));
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_keys().size(),
            10);
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_key_map().size(),
            10);
  EXPECT_EQ(
      default_list_adapter->list_adapter_helper()->estimated_sizes_px().size(),
      10);
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->unrecyclable().size(),
            10);
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper);
  for (int i = 0; i < 10; ++i) {
    ItemHolder* item_holder = default_list_adapter->GetItemHolderForIndex(i);
    EXPECT_NE(item_holder, nullptr);
    EXPECT_FALSE(item_holder->recyclable());
    EXPECT_FALSE(default_list_adapter->IsRecycled(item_holder));
    EXPECT_FALSE(default_list_adapter->IsBinding(item_holder));
    EXPECT_FALSE(default_list_adapter->IsFinishedBinding(item_holder));
    EXPECT_TRUE(default_list_adapter->IsDirty(item_holder));
    EXPECT_FALSE(default_list_adapter->IsUpdated(item_holder));
    EXPECT_FALSE(default_list_adapter->IsRemoved(item_holder));
    EXPECT_TRUE(default_list_adapter->GetListItemElement(item_holder) ==
                nullptr);
  }
}

TEST_F(DefaultListAdapterTest, FiberDiffCase1) {
  list::InsertAction insert_action{
      .insert_ops_ = {
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
  default_list_adapter->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_0.Resolve()));
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper);

  list::RemoveAction remove_action{
      .remove_ops_ = {0, 1, 2, 3, 4},
  };
  list::UpdateAction update_action{
      .update_ops_ =
          {
              {.from_ = 0, .to_ = 0, false, "F_5", 50, true, true, true, true},
              {.from_ = 1, .to_ = 1, false, "G_6", 50, true, true, true, true},
              {.from_ = 2, .to_ = 2, false, "H_7", 50, true, true, true, true},
              {.from_ = 3, .to_ = 3, false, "I_8", 50, true, true, true, true},
              {.from_ = 4, .to_ = 4, false, "J_9", 50, true, true, true, true},
          },
  };
  list::FiberDiffResult fiber_diff_result_1{
      .remove_action_ = remove_action,
      .update_action_ = update_action,
  };
  default_list_adapter->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_1.Resolve()));
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_keys().size(), 5);
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_key_map().size(),
            5);
  EXPECT_EQ(
      default_list_adapter->list_adapter_helper()->estimated_sizes_px().size(),
      5);
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->unrecyclable().size(),
            0);
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper);
  for (int i = 0; i < 5; ++i) {
    ItemHolder* item_holder = default_list_adapter->GetItemHolderForIndex(i);
    EXPECT_TRUE(item_holder->item_full_span());
    EXPECT_TRUE(item_holder->sticky_top());
    EXPECT_TRUE(item_holder->sticky_bottom());
    EXPECT_TRUE(base::FloatsEqual(item_holder->height(), 50));
    EXPECT_TRUE(item_holder->recyclable());
  }
}

TEST_F(DefaultListAdapterTest, FiberDiffCase2) {
  // test no illegal item key
  list::InsertAction insert_action{
      .insert_ops_ = {
          {.position_ = 0, "A_0", 100, false, false, false},
          {.position_ = 1, "B_1", 100, false, false, false},
          {.position_ = 2, "C_2", 100, false, false, false},
          {.position_ = 3, "D_3", 100, false, false, false},
          {.position_ = 4, "E_4", 100, false, false, false},
      }};
  list::FiberDiffResult fiber_diff_result_0{
      .insert_action_ = insert_action,
  };
  EXPECT_CALL(*default_list_adapter, OnErrorOccurred(::testing::_)).Times(0);
  default_list_adapter->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_0.Resolve()));

  // test illegal item key
  list::InsertAction insert_action_1{
      .insert_ops_ = {
          {.position_ = 0, "", 100, false, false, false},
      }};
  list::FiberDiffResult fiber_diff_result_1{
      .insert_action_ = insert_action_1,
  };
  EXPECT_CALL(*default_list_adapter, OnErrorOccurred(::testing::_)).Times(1);
  default_list_adapter->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_1.Resolve()));
}

TEST_F(DefaultListAdapterTest, FiberDiffCase3) {
  // test duplicated item key
  list::InsertAction insert_action{
      .insert_ops_ = {
          {.position_ = 0, "A_0", 100, false, false, false},
          {.position_ = 1, "B_1", 100, false, false, false},
          {.position_ = 2, "C_2", 100, false, false, false},
          {.position_ = 3, "D_3", 100, false, false, false},
          {.position_ = 4, "D_3", 100, false, false, false},
      }};
  list::FiberDiffResult fiber_diff_result_0{
      .insert_action_ = insert_action,
  };
  EXPECT_CALL(*default_list_adapter, OnErrorOccurred(::testing::_)).Times(1);
  default_list_adapter->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_0.Resolve()));
}

TEST_F(DefaultListAdapterTest, FiberFlushInUpdateAction) {
  list::InsertAction insert_action{
      .insert_ops_ = {
          {.position_ = 0, "A_0", 100, false, false, false},
          {.position_ = 1, "B_1", 100, false, false, false},
          {.position_ = 2, "C_2", 100, false, false, false},
          {.position_ = 3, "D_3", 100, false, false, false},
          {.position_ = 4, "E_4", 100, false, false, false},
          {.position_ = 5, "F_5", 100, false, false, false},
          {.position_ = 6, "G_6", 100, false, false, false},
          {.position_ = 7, "H_7", 100, false, false, false},
          {.position_ = 8, "I_8", 100, false, false, false},
          {.position_ = 9, "J_9", 100, false, false, false},
      }};
  list::FiberDiffResult fiber_diff_result_0{
      .insert_action_ = insert_action,
  };
  int item_count = 10;
  default_list_adapter->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_0.Resolve()));
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper);
  list_container->StartInterceptListElementUpdated();
  for (int index = 0; index < item_count; ++index) {
    ItemHolder* item_holder =
        default_list_adapter->GetItemHolderForIndex(index);
    if (item_holder) {
      // Bind
      EXPECT_TRUE(default_list_adapter->BindItemHolder(item_holder, index));
      auto pipeline = std::make_shared<PipelineOptions>();
      pipeline->operation_id =
          list::GenerateOperationId(list_element_ref->impl_id());
      auto list_item_ref = CreateComponentElement();
      default_list_adapter->OnFinishBindItemHolder(list_item_ref.get(),
                                                   pipeline);
      EXPECT_TRUE(default_list_adapter->GetListItemElement(item_holder) !=
                  nullptr);
    }
  }
  list_container->StopInterceptListElementUpdated();
  // Test flush is true for update action.
  list::UpdateAction update_action{
      .update_ops_ =
          {
              {.from_ = 0, .to_ = 0, true, "A_0", 50, true, true, true},
              {.from_ = 1, .to_ = 1, true, "B_1", 50, true, true, true},
              {.from_ = 2, .to_ = 2, true, "C_2", 50, true, true, true},
              {.from_ = 3, .to_ = 3, true, "D_3", 50, true, true, true},
              {.from_ = 4, .to_ = 4, true, "E_4", 50, true, true, true},
          },
  };
  list::RemoveAction remove_action{
      .remove_ops_ = {5, 6, 7, 8, 9},
  };
  list::FiberDiffResult fiber_diff_result_1{
      .remove_action_ = remove_action,
      .update_action_ = update_action,
  };
  item_count = 5;
  default_list_adapter->UpdateFiberDataSource(
      lepus::Value(fiber_diff_result_1.Resolve()));
  EXPECT_EQ(default_list_adapter->fiber_flush_item_holder_set_.size(),
            item_count);
  // Test EnqueueElementsIfNeeded
  default_list_adapter->EnqueueElementsIfNeeded();
  for (const auto& pair : *(default_list_adapter->item_holder_map_)) {
    EXPECT_EQ(default_list_adapter->GetListItemElement(pair.second.get()),
              nullptr);
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
