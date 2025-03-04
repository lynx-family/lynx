// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/batch_list_adapter.h"

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

class BatchListAdapterTest : public ::testing::Test {
 public:
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  fml::RefPtr<list::MockListElement> list_element_ref;
  std::unique_ptr<BatchListAdapter> batch_list_adapter;
  std::unique_ptr<ListContainerImpl> list_container;
  std::unique_ptr<ListChildrenHelper> list_children_helper;

 protected:
  BatchListAdapterTest() = default;
  ~BatchListAdapterTest() override = default;
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
    batch_list_adapter = std::make_unique<BatchListAdapter>(
        list_container.get(), list_element_ref.get());
    list_children_helper = std::make_unique<ListChildrenHelper>();
  }

 public:
  fml::RefPtr<Element> CreateListItemElement() {
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
};  // BatchListAdapterTest

TEST_F(BatchListAdapterTest, DiffCase0) {
  list::DiffResult diff_result{
      .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                    "I_8"},
      .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8},
      .estimated_height_pxs = {100, 100, 100, 100, 100, 100, 100, 100, 100},
  };
  batch_list_adapter->UpdateDataSource(
      lepus_value(diff_result.GenerateDiffResult()));
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_keys().size(),
            diff_result.GetItemCount());
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_key_map().size(),
            diff_result.GetItemCount());
  EXPECT_EQ(
      batch_list_adapter->list_adapter_helper()->estimated_heights_px().size(),
      diff_result.GetItemCount());
  batch_list_adapter->UpdateItemHolderToLatest(list_children_helper.get());
  for (int i = 0; i < static_cast<int>(diff_result.GetItemCount()); ++i) {
    ItemHolder* item_holder = batch_list_adapter->GetItemHolderForIndex(i);
    EXPECT_NE(item_holder, nullptr);
    EXPECT_FALSE(batch_list_adapter->IsRecycled(item_holder));
    EXPECT_FALSE(batch_list_adapter->IsBinding(item_holder));
    EXPECT_FALSE(batch_list_adapter->IsFinishedBinding(item_holder));
    EXPECT_TRUE(batch_list_adapter->IsDirty(item_holder));
    EXPECT_FALSE(batch_list_adapter->IsUpdated(item_holder));
    EXPECT_FALSE(batch_list_adapter->IsRemoved(item_holder));
    EXPECT_TRUE(batch_list_adapter->GetListItemElement(item_holder) == nullptr);
  }
}

TEST_F(BatchListAdapterTest, DiffCase1) {
  // before
  list::DiffResult diff_result_0{
      .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                    "I_8", "J_9", "K_10"},
      .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
  };
  batch_list_adapter->UpdateDataSource(
      lepus_value(diff_result_0.GenerateDiffResult()));
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_keys().size(),
            diff_result_0.GetItemCount());
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_key_map().size(),
            diff_result_0.GetItemCount());
  batch_list_adapter->UpdateItemHolderToLatest(list_children_helper.get());
  for (int index = 0; index < static_cast<int>(diff_result_0.GetItemCount());
       ++index) {
    ItemHolder* item_holder = batch_list_adapter->GetItemHolderForIndex(index);
    if (item_holder) {
      // Bind
      EXPECT_TRUE(batch_list_adapter->BindItemHolder(item_holder, index));
      EXPECT_FALSE(batch_list_adapter->IsDirty(item_holder));
      EXPECT_TRUE(batch_list_adapter->IsBinding(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsRecycled(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsFinishedBinding(item_holder));
      // Finish Bind
      PipelineOptions pipeline;
      pipeline.operation_id =
          list::GenerateOperationId(list_element_ref->impl_id());
      auto list_item_ref = CreateListItemElement();
      batch_list_adapter->OnFinishBindItemHolder(list_item_ref.get(), pipeline);
      EXPECT_FALSE(batch_list_adapter->IsBinding(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsRecycled(item_holder));
      EXPECT_TRUE(batch_list_adapter->IsFinishedBinding(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsDirty(item_holder));
      EXPECT_TRUE(batch_list_adapter->GetListItemElement(item_holder) !=
                  nullptr);
    }
  }

  // after
  list::DiffResult diff_result_1{
      .item_keys = {"New_A_0", "New_B_1", "New_C_2", "New_D_3", "E_4",
                    "New_F_5", "New_G_6", "New_H_7", "New_I_8", "J_9", "K_10"},
      .insertion = {0, 1, 2, 3, 5, 6, 7, 8},
      .removal = {0, 1, 2, 3, 5, 6, 7, 8},
      .update_from = {9, 10},
      .update_to = {9, 10},
  };
  batch_list_adapter->UpdateDataSource(
      lepus_value(diff_result_1.GenerateDiffResult()));
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_keys().size(),
            diff_result_1.GetItemCount());
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_key_map().size(),
            diff_result_1.GetItemCount());
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->insertions().size(),
            diff_result_1.insertion.size());
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->removals().size(),
            diff_result_1.removal.size());
  batch_list_adapter->UpdateItemHolderToLatest(list_children_helper.get());

  // Check removed / update status.
  const auto& item_holder_map = batch_list_adapter->item_holder_map();
  for (int index = 0; index < static_cast<int>(diff_result_0.GetItemCount());
       ++index) {
    const std::string& item_key = diff_result_0.item_keys[index];
    if (auto it = item_holder_map->find(item_key);
        it != item_holder_map->end()) {
      if (index == 4) {
        // The data of index 4 is not removed.
        EXPECT_FALSE(batch_list_adapter->IsRemoved(it->second.get()));
        EXPECT_FALSE(batch_list_adapter->IsUpdated(it->second.get()));
        EXPECT_FALSE(batch_list_adapter->IsDirty(it->second.get()));
        EXPECT_TRUE(batch_list_adapter->IsFinishedBinding(it->second.get()));
      } else if (index == 9 || index == 10) {
        // The data of index 9 or 10 is updated.
        EXPECT_FALSE(batch_list_adapter->IsRemoved(it->second.get()));
        EXPECT_TRUE(batch_list_adapter->IsUpdated(it->second.get()));
        EXPECT_TRUE(batch_list_adapter->IsDirty(it->second.get()));
        EXPECT_FALSE(batch_list_adapter->IsFinishedBinding(it->second.get()));
      } else {
        // Other data is removed.
        EXPECT_TRUE(batch_list_adapter->IsRemoved(it->second.get()));
      }
    }
  }

  for (int index = 0; index < static_cast<int>(diff_result_1.GetItemCount());
       ++index) {
    ItemHolder* item_holder = batch_list_adapter->GetItemHolderForIndex(index);
    if (item_holder) {
      if (index == 4) {
        // No need to Bind
        EXPECT_FALSE(batch_list_adapter->BindItemHolder(item_holder, index));
      } else {
        // Bind
        EXPECT_TRUE(batch_list_adapter->BindItemHolder(item_holder, index));
        PipelineOptions pipeline;
        pipeline.operation_id =
            list::GenerateOperationId(list_element_ref->impl_id());
        auto list_item_ref = CreateListItemElement();
        batch_list_adapter->OnFinishBindItemHolder(list_item_ref.get(),
                                                   pipeline);
        EXPECT_FALSE(batch_list_adapter->IsBinding(item_holder));
        EXPECT_FALSE(batch_list_adapter->IsRecycled(item_holder));
        EXPECT_TRUE(batch_list_adapter->IsFinishedBinding(item_holder));
        EXPECT_FALSE(batch_list_adapter->IsDirty(item_holder));
        EXPECT_TRUE(batch_list_adapter->GetListItemElement(item_holder) !=
                    nullptr);
      }
    }
  }
  // Before invoke RecycleRemovedItemHolders(), check the size of
  // item_holder_map.
  EXPECT_EQ(batch_list_adapter->item_holder_map()->size(),
            diff_result_0.GetItemCount() + diff_result_1.insertion.size());
  batch_list_adapter->RecycleRemovedItemHolders();
  // After invoke RecycleRemovedItemHolders(), check the size of
  // item_holder_map.
  EXPECT_EQ(batch_list_adapter->item_holder_map()->size(),
            diff_result_1.GetItemCount());
}

TEST_F(BatchListAdapterTest, RecycleItemHolder) {
  list::DiffResult diff_result_0{
      .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                    "I_8", "J_9", "K_10"},
      .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
  };
  batch_list_adapter->UpdateDataSource(
      lepus_value(diff_result_0.GenerateDiffResult()));
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_keys().size(),
            diff_result_0.GetItemCount());
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_key_map().size(),
            diff_result_0.GetItemCount());
  batch_list_adapter->UpdateItemHolderToLatest(list_children_helper.get());
  for (int index = 0; index < static_cast<int>(diff_result_0.GetItemCount());
       ++index) {
    ItemHolder* item_holder = batch_list_adapter->GetItemHolderForIndex(index);
    if (item_holder) {
      // Bind
      EXPECT_TRUE(batch_list_adapter->BindItemHolder(item_holder, index));
      // Finish Bind
      PipelineOptions pipeline;
      pipeline.operation_id =
          list::GenerateOperationId(list_element_ref->impl_id());
      auto list_item_ref = CreateListItemElement();
      batch_list_adapter->OnFinishBindItemHolder(list_item_ref.get(), pipeline);
    }
  }
  // Recycle part of ItemHolders.
  for (int index = 0;
       index < static_cast<int>(diff_result_0.GetItemCount() / 3); ++index) {
    ItemHolder* item_holder = batch_list_adapter->GetItemHolderForIndex(index);
    if (item_holder) {
      EXPECT_TRUE(batch_list_adapter->IsFinishedBinding(item_holder));
      EXPECT_TRUE(batch_list_adapter->GetListItemElement(item_holder) !=
                  nullptr);
      batch_list_adapter->RecycleItemHolder(item_holder);
      EXPECT_TRUE(batch_list_adapter->IsRecycled(item_holder));
      EXPECT_TRUE(batch_list_adapter->GetListItemElement(item_holder) ==
                  nullptr);
    }
  }
}

TEST_F(BatchListAdapterTest, BindItemHolders) {
  list::DiffResult diff_result_0{
      .item_keys = {"A_0", "B_1", "C_2", "D_3", "E_4", "F_5", "G_6", "H_7",
                    "I_8", "J_9", "K_10"},
      .insertion = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10},
  };
  batch_list_adapter->UpdateDataSource(
      lepus_value(diff_result_0.GenerateDiffResult()));
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_keys().size(),
            diff_result_0.GetItemCount());
  EXPECT_EQ(batch_list_adapter->list_adapter_helper()->item_key_map().size(),
            diff_result_0.GetItemCount());
  batch_list_adapter->UpdateItemHolderToLatest(list_children_helper.get());
  // Test list batch render.
  ItemHolderSet item_holder_set;
  PipelineOptions pipeline;
  std::vector<Element*> list_items;
  for (int index = 0;
       index < static_cast<int>(diff_result_0.GetItemCount() / 2); ++index) {
    item_holder_set.insert(batch_list_adapter->GetItemHolderForIndex(index));
    list_items.emplace_back(CreateListItemElement().get());
    pipeline.operation_ids_.emplace_back(
        list::GenerateOperationId(list_element_ref->impl_id()));
  }
  // Invoke batch render.
  batch_list_adapter->BindItemHolders(item_holder_set);
  for (int index = 0; index < static_cast<int>(diff_result_0.GetItemCount());
       ++index) {
    ItemHolder* item_holder = batch_list_adapter->GetItemHolderForIndex(index);
    if (index < static_cast<int>(diff_result_0.GetItemCount() / 2)) {
      // In batch render.
      EXPECT_FALSE(batch_list_adapter->IsDirty(item_holder));
      EXPECT_TRUE(batch_list_adapter->IsBinding(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsRecycled(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsFinishedBinding(item_holder));
    } else {
      // No render.
      EXPECT_TRUE(batch_list_adapter->IsDirty(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsBinding(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsRecycled(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsFinishedBinding(item_holder));
    }
  }
  // After finish batch render.
  batch_list_adapter->OnFinishBindItemHolders(list_items, pipeline);
  for (int index = 0; index < static_cast<int>(diff_result_0.GetItemCount());
       ++index) {
    ItemHolder* item_holder = batch_list_adapter->GetItemHolderForIndex(index);
    if (index < static_cast<int>(diff_result_0.GetItemCount() / 2)) {
      // Finish batch render.
      EXPECT_FALSE(batch_list_adapter->IsDirty(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsBinding(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsRecycled(item_holder));
      EXPECT_TRUE(batch_list_adapter->IsFinishedBinding(item_holder));
      EXPECT_TRUE(batch_list_adapter->GetListItemElement(item_holder) !=
                  nullptr);
    } else {
      // No render.
      EXPECT_TRUE(batch_list_adapter->IsDirty(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsBinding(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsRecycled(item_holder));
      EXPECT_FALSE(batch_list_adapter->IsFinishedBinding(item_holder));
      EXPECT_TRUE(batch_list_adapter->GetListItemElement(item_holder) ==
                  nullptr);
    }
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
