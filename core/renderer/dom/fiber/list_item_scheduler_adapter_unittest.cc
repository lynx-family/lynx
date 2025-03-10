// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/fiber/list_item_scheduler_adapter.h"

#include <memory>
#include <vector>

#include "core/base/threading/vsync_monitor.h"
#include "core/renderer/css/css_color.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/css/ng/parser/css_parser_token_range.h"
#include "core/renderer/css/ng/parser/css_tokenizer.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/component_element.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/layout_bundle.h"
#include "core/renderer/dom/testing/fiber_mock_painting_context.h"
#include "core/renderer/ui_component/list/batch_list_adapter.h"
#include "core/renderer/ui_component/list/list_container_impl.h"
#include "core/renderer/ui_component/list/list_types.h"
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
static constexpr int64_t kFrameDuration = 16;  // ms

const int scheduler_adapter_generation_params[] = {
    0,  // ALL_ON_UI thread strategy
    3,  // MULTI_THREAD strategy
};

class TestVSyncMonitor : public base::VSyncMonitor {
 public:
  TestVSyncMonitor() = default;
  ~TestVSyncMonitor() override = default;

  void RequestVSync() override {}

  void TriggerVsync() {
    OnVSync(current_, current_ + kFrameDuration);
    current_ += kFrameDuration;
  }

 private:
  int64_t current_ = kFrameDuration;
};

class ListItemSchedulerAdapterMockTasmDelegate : public test::MockTasmDelegate {
 public:
  void UpdateLayoutNodeByBundle(
      int32_t id, std::unique_ptr<tasm::LayoutBundle> bundle) override {
    std::unique_lock<std::mutex> locker(mutex_);
    captured_ids_.emplace_back(id);
    captured_bundles_.emplace_back(std::move(bundle));
  }

  std::mutex mutex_;
  std::vector<int> captured_ids_;
  std::vector<std::unique_ptr<LayoutBundle>> captured_bundles_;
};

class ListItemSchedulerAdapterTest : public ::testing::TestWithParam<int> {
 public:
  ListItemSchedulerAdapterTest() { current_parameter_ = GetParam(); }
  ~ListItemSchedulerAdapterTest() override {}
  lynx::tasm::ElementManager* manager;
  ::testing::NiceMock<ListItemSchedulerAdapterMockTasmDelegate> tasm_mediator;
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm;
  FiberMockPaintingContext* platform_impl_;
  std::shared_ptr<TestVSyncMonitor> vsync_monitor_;

  static void SetUpTestSuite() { base::UIThread::Init(); }

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    vsync_monitor_ = std::make_shared<TestVSyncMonitor>();
    vsync_monitor_->BindToCurrentThread();
    auto unique_manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<FiberMockPaintingContext>(), &tasm_mediator,
        lynx_env_config, tasm::report::kUnknownInstanceId, vsync_monitor_);
    manager = unique_manager.get();
    platform_impl_ = static_cast<FiberMockPaintingContext*>(
        manager->painting_context()->platform_impl_.get());

    tasm = std::make_shared<lynx::tasm::TemplateAssembler>(
        tasm_mediator, std::move(unique_manager), 0);

    auto test_entry = std::make_shared<TemplateEntry>();
    tasm->template_entries_.insert({"test_entry", test_entry});

    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    config->SetEnableFiberArch(true);
    manager->SetConfig(config);
    tasm->page_config_ = config;

    thread_strategy = current_parameter_;
    if (thread_strategy == 0) {
      manager->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
    } else {
      manager->SetThreadStrategy(
          base::ThreadStrategyForRendering::MULTI_THREADS);
    }
    manager->SetEnableParallelElement(true);
    const_cast<DynamicCSSConfigs&>(manager->GetDynamicCSSConfigs())
        .unify_vw_vh_behavior_ = true;
  }

 protected:
  int current_parameter_;
  int32_t thread_strategy;
  bool enable_parallel_element_flush;
};

TEST_P(ListItemSchedulerAdapterTest, ListItemResolveSubtreePropertyTest) {
  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  // CSSParserConfigs configs;

  CSSParserTokenMap indexTokenMap;
  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);
  auto style_sheet =
      std::make_shared<CSSFragmentDecorator>(indexFragment.get());

  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(kEnableParallelElementMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  page->style_sheet_ = style_sheet;

  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  list->SetAttribute(list::kExperimentalBatchRenderStrategy, lepus::Value(3));
  page->InsertNode(list);

  PipelineOptions options;
  manager->OnPatchFinish(options);
  EXPECT_FALSE(platform_impl_->HasFlushed());
  platform_impl_->ResetFlushFlag();

  // list-item #0
  auto wrapper_0 = manager->CreateFiberWrapperElement();
  wrapper_0->parent_component_element_ = page.get();
  list->InsertNode(wrapper_0);
  auto text_0 = manager->CreateFiberText("text");
  text_0->parent_component_element_ = page.get();
  text_0->SetStyle(kPropertyIDColor, lepus::Value("red"));
  wrapper_0->InsertNode(text_0);
  wrapper_0->AsyncResolveSubtreeProperty();

  // list-item #1
  auto wrapper_1 = manager->CreateFiberWrapperElement();
  wrapper_1->parent_component_element_ = page.get();
  list->InsertNode(wrapper_1);
  auto text_1 = manager->CreateFiberText("text");
  wrapper_1->InsertNode(text_1);
  text_1->parent_component_element_ = page.get();
  text_1->SetStyle(kPropertyIDColor, lepus::Value("red"));
  wrapper_1->AsyncResolveSubtreeProperty();

  // list-item #2
  auto wrapper_2 = manager->CreateFiberWrapperElement();
  list->InsertNode(wrapper_2);
  wrapper_2->parent_component_element_ = page.get();
  auto text_2 = manager->CreateFiberText("text");
  wrapper_2->InsertNode(text_2);
  text_2->SetStyle(kPropertyIDColor, lepus::Value("red"));
  text_2->parent_component_element_ = page.get();
  wrapper_2->AsyncResolveSubtreeProperty();

  // list-item #3
  auto wrapper_3 = manager->CreateFiberWrapperElement();
  list->InsertNode(wrapper_3);
  wrapper_3->parent_component_element_ = page.get();
  auto text_3 = manager->CreateFiberText("text");
  wrapper_3->InsertNode(text_3);
  text_3->SetStyle(kPropertyIDColor, lepus::Value("red"));
  text_3->parent_component_element_ = page.get();
  wrapper_3->AsyncResolveSubtreeProperty();

  manager->GetTasmWorkerTaskRunner()->WaitForCompletion();
  // Each list-item has its own task
  EXPECT_TRUE(manager->ParallelTasks().size() == 4);

  list->ParallelFlushAsRoot();

  EXPECT_TRUE(manager->ParallelTasks().size() == 0);
  EXPECT_TRUE(manager->ParallelResolveTreeTasks().size() == 0);
  EXPECT_TRUE(wrapper_0->scheduler_adapter_->resolve_property_queue_.size() ==
              0);
  EXPECT_TRUE(wrapper_1->scheduler_adapter_->resolve_property_queue_.size() ==
              0);
  EXPECT_TRUE(wrapper_2->scheduler_adapter_->resolve_property_queue_.size() ==
              0);
  EXPECT_TRUE(wrapper_3->scheduler_adapter_->resolve_property_queue_.size() ==
              0);
  EXPECT_TRUE(
      wrapper_0->scheduler_adapter_->resolve_element_tree_queue_.size() == 0);
  EXPECT_TRUE(
      wrapper_1->scheduler_adapter_->resolve_element_tree_queue_.size() == 0);
  EXPECT_TRUE(
      wrapper_2->scheduler_adapter_->resolve_element_tree_queue_.size() == 0);
  EXPECT_TRUE(
      wrapper_3->scheduler_adapter_->resolve_element_tree_queue_.size() == 0);

  EXPECT_TRUE(text_0->parsed_styles_map_.size() == 1);
  EXPECT_TRUE(text_1->parsed_styles_map_.size() == 1);
  EXPECT_TRUE(text_2->parsed_styles_map_.size() == 1);
  EXPECT_TRUE(text_3->parsed_styles_map_.size() == 1);
}

TEST_P(ListItemSchedulerAdapterTest, RecordingRenderRootComponentElementTest) {
  // css related
  StyleMap indexAttributes;

  CSSParserTokenMap indexTokensMap;
  // CSSParserConfigs configs;

  CSSParserTokenMap indexTokenMap;
  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  auto indexFragment = std::make_shared<SharedCSSFragment>(
      1, dependent_ids, indexTokensMap, keyframes, fontfaces);
  auto style_sheet =
      std::make_shared<CSSFragmentDecorator>(indexFragment.get());

  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(kEnableParallelElementMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  page->style_sheet_ = style_sheet;

  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  list->SetAttribute(list::kExperimentalBatchRenderStrategy, lepus::Value(3));
  page->InsertNode(list);

  PipelineOptions options;
  manager->OnPatchFinish(options);
  EXPECT_FALSE(platform_impl_->HasFlushed());
  platform_impl_->ResetFlushFlag();

  base::String entry_name("TTTT");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  // list-item #0
  base::String component_id_1("21");
  int32_t css_id_1 = 123;
  auto comp_1 = manager->CreateFiberComponent(component_id_1, css_id_1,
                                              entry_name, component_name, path);
  comp_1->SetParentComponentUniqueIdForFiber(page->impl_id());
  auto view_1 = manager->CreateFiberView();
  view_1->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp_1->impl_id()));
  comp_1->InsertNode(view_1);
  list->InsertNode(comp_1);
  comp_1->AsyncResolveSubtreeProperty();

  // list-item #1
  base::String component_id_2("22");
  int32_t css_id_2 = 124;
  auto comp_2 = manager->CreateFiberComponent(component_id_2, css_id_2,
                                              entry_name, component_name, path);
  comp_2->SetParentComponentUniqueIdForFiber(page->impl_id());
  auto view_2 = manager->CreateFiberView();
  view_2->SetParentComponentUniqueIdForFiber(
      static_cast<int64_t>(comp_2->impl_id()));
  comp_2->InsertNode(view_2);
  list->InsertNode(comp_2);
  comp_2->AsyncResolveSubtreeProperty();

  // list-item #2
  base::String component_id_3("23");
  int32_t css_id_3 = 125;
  auto comp_3 = manager->CreateFiberComponent(component_id_3, css_id_3,
                                              entry_name, component_name, path);
  comp_3->SetParentComponentUniqueIdForFiber(page->impl_id());
  auto view_3 = manager->CreateFiberView();
  view_3->SetParentComponentUniqueIdForFiber(comp_3->impl_id());
  comp_3->InsertNode(view_3);
  list->InsertNode(comp_3);
  comp_3->AsyncResolveSubtreeProperty();

  // list-item #3
  base::String component_id_4("23");
  int32_t css_id_4 = 126;
  auto comp_4 = manager->CreateFiberComponent(component_id_4, css_id_4,
                                              entry_name, component_name, path);
  comp_4->SetParentComponentUniqueIdForFiber(page->impl_id());
  auto view_4 = manager->CreateFiberView();
  view_4->SetParentComponentUniqueIdForFiber(comp_4->impl_id());
  comp_4->InsertNode(view_4);
  list->InsertNode(comp_4);
  comp_4->AsyncResolveSubtreeProperty();

  manager->GetTasmWorkerTaskRunner()->WaitForCompletion();
  // Each list-item has its own task
  EXPECT_TRUE(manager->ParallelTasks().size() == 4);

  list->ParallelFlushAsRoot();

  EXPECT_TRUE(manager->ParallelTasks().size() == 0);
  EXPECT_TRUE(manager->ParallelResolveTreeTasks().size() == 0);
  EXPECT_TRUE(comp_1->scheduler_adapter_->resolve_property_queue_.size() == 0);
  EXPECT_TRUE(comp_2->scheduler_adapter_->resolve_property_queue_.size() == 0);
  EXPECT_TRUE(comp_3->scheduler_adapter_->resolve_property_queue_.size() == 0);
  EXPECT_TRUE(comp_4->scheduler_adapter_->resolve_property_queue_.size() == 0);
  EXPECT_TRUE(comp_1->scheduler_adapter_->resolve_element_tree_queue_.size() ==
              0);
  EXPECT_TRUE(comp_2->scheduler_adapter_->resolve_element_tree_queue_.size() ==
              0);
  EXPECT_TRUE(comp_3->scheduler_adapter_->resolve_element_tree_queue_.size() ==
              0);
  EXPECT_TRUE(comp_4->scheduler_adapter_->resolve_element_tree_queue_.size() ==
              0);

  EXPECT_TRUE(comp_1->scheduler_adapter_ != nullptr);
  EXPECT_TRUE(comp_2->scheduler_adapter_ != nullptr);
  EXPECT_TRUE(comp_3->scheduler_adapter_ != nullptr);
  EXPECT_TRUE(comp_4->scheduler_adapter_ != nullptr);

  EXPECT_TRUE(comp_1->parent_component_element_ == page.get());
  EXPECT_TRUE(comp_2->parent_component_element_ == page.get());
  EXPECT_TRUE(comp_3->parent_component_element_ == page.get());
  EXPECT_TRUE(comp_4->parent_component_element_ == page.get());
  EXPECT_TRUE(view_1->parent_component_element_ == comp_1.get());
  EXPECT_TRUE(view_2->parent_component_element_ == comp_2.get());
  EXPECT_TRUE(view_3->parent_component_element_ == comp_3.get());
  EXPECT_TRUE(view_4->parent_component_element_ == comp_4.get());
}

TEST_P(ListItemSchedulerAdapterTest,
       TestResolveBatchRenderStrategyFromPipelineSchedulerConfig) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(
      kEnableParallelElementMask | kEnableListBatchRenderMask |
      kEnableListBatchRenderAsyncResolvePropertyMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(true);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kAsyncResolveProperty);
}

TEST_P(ListItemSchedulerAdapterTest,
       TestResolveBatchRenderStrategyFromPipelineSchedulerConfig1) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(
      kEnableListBatchRenderMask |
      kEnableListBatchRenderAsyncResolvePropertyMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(false);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kBatchRender);
}

TEST_P(ListItemSchedulerAdapterTest,
       TestResolveBatchRenderStrategyFromPipelineSchedulerConfig2) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(
      kEnableListBatchRenderAsyncResolvePropertyMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(false);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kDefault);
}

TEST_P(ListItemSchedulerAdapterTest,
       TestResolveBatchRenderStrategyFromPipelineSchedulerConfig3) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(
      kEnableListBatchRenderAsyncResolvePropertyMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(true);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kDefault);
}

TEST_P(ListItemSchedulerAdapterTest,
       TestResolveBatchRenderStrategyFromPipelineSchedulerConfig4) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(
      kEnableListBatchRenderAsyncResolvePropertyMask |
      kEnableListBatchRenderMask | kEnableListBatchRenderAsyncResolveTreeMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(true);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree);
}

TEST_P(ListItemSchedulerAdapterTest,
       TestResolveBatchRenderStrategyFromPipelineSchedulerConfig5) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(kEnableListBatchRenderMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(true);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kBatchRender);
}

TEST_P(ListItemSchedulerAdapterTest, TestListBatchRenderStrategyIllegalValue) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(kEnableListBatchRenderMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(true);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  list->SetAttributeInternal(list::kExperimentalBatchRenderStrategy,
                             lepus::Value(-2));
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kBatchRender);
}

TEST_P(ListItemSchedulerAdapterTest, TestListBatchRenderStrategyIllegalValue1) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(kEnableListBatchRenderMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(true);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;

  auto& updated_attr_map = const_cast<AttrUMap&>(list->updated_attr_map());
  updated_attr_map[list::kExperimentalBatchRenderStrategy] = lepus::Value(0);
  list->PrepareForCreateOrUpdate();
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kDefault);

  updated_attr_map[list::kExperimentalBatchRenderStrategy] = lepus::Value(1);
  list->PrepareForCreateOrUpdate();
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kBatchRender);

  updated_attr_map[list::kExperimentalBatchRenderStrategy] = lepus::Value(2);
  list->PrepareForCreateOrUpdate();
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kAsyncResolveProperty);

  updated_attr_map[list::kExperimentalBatchRenderStrategy] = lepus::Value(3);
  list->PrepareForCreateOrUpdate();
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree);
}

TEST_P(ListItemSchedulerAdapterTest, TestListBatchRenderStrategyIllegalValue2) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(kEnableListBatchRenderMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(true);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  list->SetAttributeInternal(list::kExperimentalBatchRenderStrategy,
                             lepus::Value(4));
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kBatchRender);
}

TEST_P(ListItemSchedulerAdapterTest, TestListBatchRenderStrategyIllegalValue3) {
  auto config = std::make_shared<PageConfig>();
  config->SetPipelineSchedulerConfig(
      kEnableListBatchRenderMask |
      kEnableListBatchRenderAsyncResolvePropertyMask);
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  manager->SetEnableParallelElement(false);

  // page
  auto page = manager->CreateFiberPage("page", 10);
  auto list = manager->CreateFiberList(tasm.get(), "list", lepus::Value(),
                                       lepus::Value(), lepus::Value());
  list->disable_list_platform_implementation_ = true;
  list->SetAttributeInternal(list::kExperimentalBatchRenderStrategy,
                             lepus::Value(4));
  EXPECT_TRUE(list->list_container_delegate_->GetBatchRenderStrategy() ==
              list::BatchRenderStrategy::kBatchRender);
}

INSTANTIATE_TEST_SUITE_P(
    ListItemSchedulerAdapterTestModule, ListItemSchedulerAdapterTest,
    ::testing::ValuesIn(scheduler_adapter_generation_params));

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
