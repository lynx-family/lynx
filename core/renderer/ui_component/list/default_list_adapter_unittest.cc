// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

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
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class DefaultListAdapterTest : public ::testing::Test {
 public:
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  fml::RefPtr<list::MockListElement> list_element_ref;
  std::unique_ptr<DefaultListAdapter> default_list_adapter;
  std::unique_ptr<ListContainerImpl> list_container;
  std::unique_ptr<ListChildrenHelper> list_children_helper;

 protected:
  DefaultListAdapterTest() = default;
  ~DefaultListAdapterTest() override = default;
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
    default_list_adapter = std::make_unique<DefaultListAdapter>(
        list_container.get(), list_element_ref.get());
    list_children_helper = std::make_unique<ListChildrenHelper>();
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
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper.get());
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
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper.get());
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
      PipelineOptions pipeline;
      pipeline.operation_id =
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
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper.get());
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
        PipelineOptions pipeline;
        pipeline.operation_id =
            list::GenerateOperationId(list_element_ref->impl_id());
        auto list_item_ref = CreateComponentElement();
        default_list_adapter->OnFinishBindItemHolder(list_item_ref.get(),
                                                     pipeline);
        EXPECT_TRUE(default_list_adapter->GetListItemElement(item_holder) !=
                    nullptr);
      }
    }
  }
  default_list_adapter->RecycleRemovedItemHolders();
  EXPECT_EQ(default_list_adapter->item_holder_map()->size(),
            diff_result_1.GetItemCount());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
