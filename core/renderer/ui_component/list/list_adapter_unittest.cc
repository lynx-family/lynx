// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

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
#include "core/renderer/ui_component/list/default_list_adapter.h"
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
  std::unique_ptr<DefaultListAdapter> default_list_adapter;
  std::unique_ptr<ListContainerImpl> list_container;
  std::unique_ptr<ListChildrenHelper> list_children_helper;

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
    default_list_adapter = std::make_unique<DefaultListAdapter>(
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
  default_list_adapter->UpdateDataSource(
      lepus_value(diff_result.GenerateDiffResult()));
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_keys().size(),
            diff_result.GetItemCount());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->item_key_map().size(),
            diff_result.GetItemCount());
  EXPECT_EQ(default_list_adapter->GetDataCount(), diff_result.GetItemCount());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()
                ->estimated_heights_px()
                .size(),
            diff_result.GetItemCount());
  EXPECT_EQ(
      default_list_adapter->list_adapter_helper()->estimated_sizes_px().size(),
      diff_result.GetItemCount());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->full_spans().size(),
            diff_result.full_spans.size());
  EXPECT_EQ(default_list_adapter->list_adapter_helper()->sticky_tops().size(),
            diff_result.sticky_tops.size());
  EXPECT_EQ(default_list_adapter->GetStickyTops().size(),
            diff_result.sticky_tops.size());
  EXPECT_EQ(
      default_list_adapter->list_adapter_helper()->sticky_bottoms().size(),
      diff_result.sticky_bottoms.size());
  EXPECT_EQ(default_list_adapter->GetStickyBottoms().size(),
            diff_result.sticky_bottoms.size());
  EXPECT_TRUE(default_list_adapter->HasFullSpanItems());
  default_list_adapter->UpdateItemHolderToLatest(list_children_helper.get());
  for (int index = 0; index < static_cast<int>(diff_result.GetItemCount());
       ++index) {
    ItemHolder* item_holder =
        default_list_adapter->GetItemHolderForIndex(index);
    EXPECT_NE(item_holder, nullptr);
    if (index == 0 || index == 1) {
      EXPECT_TRUE(item_holder->sticky_top());
    } else if (index == 7 || index == 8) {
      EXPECT_TRUE(item_holder->sticky_bottom());
    }
  }
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
