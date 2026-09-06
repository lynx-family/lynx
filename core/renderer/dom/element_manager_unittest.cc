// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/element_manager.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "base/include/fml/synchronization/waitable_event.h"
#include "base/include/fml/thread.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_fragment_decorator.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_property.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/view_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/dom/fragment/display_list.h"
#include "core/renderer/dom/fragment/event/platform_event_bundle.h"
#include "core/renderer/element_manager_delegate_impl.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context.h"
#include "core/services/timing_handler/timing.h"
#include "core/services/timing_handler/timing_constants.h"
#include "core/shell/dynamic_ui_operation_queue.h"
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

class ScopedExternalBoolEnv {
 public:
  ScopedExternalBoolEnv(LynxEnv::Key key, bool value) : key_(key) {
    auto& env = LynxEnv::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(env.external_env_mutex_);
    auto it = env.external_env_map_.find(key_);
    if (it != env.external_env_map_.end()) {
      previous_value_ = it->second;
    }
    env.external_env_map_[key_] =
        value ? LynxEnv::kLocalEnvValueTrue : LynxEnv::kLocalEnvValueFalse;
  }

  ~ScopedExternalBoolEnv() {
    auto& env = LynxEnv::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(env.external_env_mutex_);
    if (previous_value_) {
      env.external_env_map_[key_] = *previous_value_;
    } else {
      env.external_env_map_.erase(key_);
    }
  }

 private:
  LynxEnv::Key key_;
  std::optional<std::string> previous_value_;
};

class RecordingMockPaintingContext : public MockPaintingContext,
                                     public NativePaintingContext {
 public:
  void RecordInitialLynxUITreeForReplay(
      std::vector<InitialLynxUITreeNodeForReplay> nodes) override {
    initial_tree_nodes_ = std::move(nodes);
  }

  void Flush() override {
    flush_called_ = true;
    if (ui_operation_queue_) {
      ui_operation_queue_->Flush();
    }
    MockPaintingContext::Flush();
  }

  void SetUIOperationQueue(
      const std::shared_ptr<shell::UIOperationQueueInterface>& queue) override {
    ui_operation_queue_ = queue;
  }

  NativePaintingContext* CastToNativeCtx() override { return this; }

  void UpdateLayoutPatching() override {
    if (on_layout_) {
      on_layout_();
    }
  }

  void FinishTasmOperation(
      const std::shared_ptr<PipelineOptions>& options) override {}
  void FinishLayoutOperation(
      const std::shared_ptr<PipelineOptions>& options) override {}
  void CreatePlatformRenderer(
      int id, PlatformRendererType type,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config) override {}
  void CreatePlatformExtendedRenderer(
      int id, const base::String& tag_name,
      const fml::RefPtr<PropBundle>& init_data,
      const PlatformRendererInitConfig& init_config) override {
    EnqueueUIOperation();
  }
  void UpdateDisplayList(int id, DisplayList list) override {}
  fml::RefPtr<PaintImage> CreateImage(int id, base::String src,
                                      const ImagePaintInfo& paint_info,
                                      float width, float height,
                                      int32_t event_mask,
                                      bool disable_default_resize) override {
    return nullptr;
  }
  void UpdateTextBundle(int id, intptr_t bundle) override {}
  void DestroyTextBundle(int id) override {}
  void ReconstructEventTargetTreeRecursively() override {}
  void UpdatePlatformEventBundle(int id, PlatformEventBundle bundle) override {}

  void EnqueueUIOperation() {
    if (!ui_operation_queue_) {
      return;
    }
    ui_operation_queue_->Enqueue([this]() {
      if (on_ui_operation_) {
        on_ui_operation_();
      }
    });
  }

  std::function<void()> on_layout_;
  std::function<void()> on_ui_operation_;
  std::shared_ptr<shell::UIOperationQueueInterface> ui_operation_queue_;
  std::atomic_bool flush_called_{false};
  std::vector<InitialLynxUITreeNodeForReplay> initial_tree_nodes_;
};

class RecordingTimingDelegate {
 public:
  void SetTiming(Timing timing) { timing_.emplace(std::move(timing)); }

  std::optional<Timing> timing_;
};

const InitialLynxUITreeNodeForReplay* FindInitialTreeNode(
    const std::vector<InitialLynxUITreeNodeForReplay>& nodes, int id) {
  for (const auto& node : nodes) {
    if (node.id == id) {
      return &node;
    }
  }
  return nullptr;
}

class ElementManagerTest : public ::testing::Test {
 public:
  ElementManagerTest() {}
  ~ElementManagerTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  RecordingMockPaintingContext* painting_context = nullptr;

  void SetUp() override { CreateManager(); }

  void CreateManager() {
    manager.reset();
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto painting_context_impl =
        std::make_unique<RecordingMockPaintingContext>();
    painting_context = painting_context_impl.get();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::move(painting_context_impl), tasm_mediator.get(), lynx_env_config,
        tasm::PageOptions());
    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }
};

constexpr EmbeddedMode MakeEmbeddedMode(bool enable_text_service) {
  return static_cast<EmbeddedMode>(
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT) |
      static_cast<int32_t>(EmbeddedMode::FRAGMENT_LAYER_RENDER) |
      (enable_text_service
           ? static_cast<int32_t>(EmbeddedMode::USE_TEXT_SERVICE)
           : 0));
}

constexpr EmbeddedMode kLayoutFragmentMode = MakeEmbeddedMode(false);
constexpr EmbeddedMode kLayoutFragmentTextMode = MakeEmbeddedMode(true);

class ElementManagerUIOperationOverlapTest : public ElementManagerTest {
 protected:
  void ConfigureManager(EmbeddedMode embedded_mode,
                        base::ThreadStrategyForRendering thread_strategy) {
    manager->SetElementManagerDelegate(&element_manager_delegate_);
    manager->page_options_.SetEmbeddedMode(embedded_mode);
    manager->SetThreadStrategy(thread_strategy);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableFiberArch(true);
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }

  void CreateAndFlushPage() {
    auto page = manager->CreateFiberPage("page", 11);
    page->FlushActionsAsRoot();
  }

  std::shared_ptr<shell::DynamicUIOperationQueue> SetUIOperationQueue(
      base::ThreadStrategyForRendering thread_strategy) {
    auto queue = std::make_shared<shell::DynamicUIOperationQueue>(
        thread_strategy, ui_thread_.GetTaskRunner());
    manager->painting_context()->SetUIOperationQueue(queue);
    return queue;
  }

 private:
  fml::Thread ui_thread_{"LayoutUIOperationOverlapUI"};
  ElementManagerDelegateImpl element_manager_delegate_{nullptr};
};

TEST_F(ElementManagerTest, CreateFiberPage) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  base::String component_id("21");
  int32_t css_id = 100;
  auto page = manager->CreateFiberPage(component_id, css_id);

  EXPECT_EQ(page->component_id().c_str(), component_id.c_str());
  EXPECT_TRUE(page->is_page());
  EXPECT_EQ(manager->GetComponent(component_id.str()), page.get());
}

TEST_F(ElementManagerTest,
       InitialTreeReplayUsesLayoutOnlyAncestorPlatformLayout) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);
  auto layout_only_parent = manager->CreateFiberView();
  layout_only_parent->computed_css_style()->SetOverflowDefaultVisible(true);
  layout_only_parent->has_layout_only_props_ = true;

  auto child = manager->CreateFiberView();
  child->MarkCanBeLayoutOnly(false);

  page->InsertNode(layout_only_parent);
  layout_only_parent->InsertNode(child);
  page->FlushActionsAsRoot();

  ASSERT_TRUE(layout_only_parent->IsLayoutOnly());
  ASSERT_FALSE(child->IsLayoutOnly());

  page->UpdateLayout(0, 0, kWidth, kHeight, {0}, {0}, {0}, nullptr, 0);
  layout_only_parent->UpdateLayout(10, 20, 100, 100, {0}, {0}, {0}, nullptr, 0);
  child->UpdateLayout(3, 4, 30, 40, {0}, {0}, {0}, nullptr, 0);
  page->element_container_impl()->UpdateLayout(page->left(), page->top());

  auto* child_painting_node =
      painting_context->node_map_.at(child->impl_id()).get();
  ASSERT_NE(child_painting_node, nullptr);
  EXPECT_FLOAT_EQ(child_painting_node->frame_.left_, 13);
  EXPECT_FLOAT_EQ(child_painting_node->frame_.top_, 24);

  manager->RecordCurrentLynxUITree();

  const auto* child_node = FindInitialTreeNode(
      painting_context->initial_tree_nodes_, child->impl_id());
  ASSERT_NE(child_node, nullptr);
  EXPECT_TRUE(child_node->has_parent);
  EXPECT_EQ(child_node->parent, page->impl_id());
  EXPECT_EQ(child_node->index, 0);
  EXPECT_FLOAT_EQ(child_node->x, child_painting_node->frame_.left_);
  EXPECT_FLOAT_EQ(child_node->y, child_painting_node->frame_.top_);
}

TEST_F(ElementManagerTest, InitialTreeReplayUsesZIndexHoistPlatformLayout) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);
  auto parent = manager->CreateFiberView();
  parent->MarkCanBeLayoutOnly(false);

  auto z_child = manager->CreateFiberView();
  z_child->MarkCanBeLayoutOnly(false);
  z_child->SetStyle(CSSPropertyID::kPropertyIDZIndex, lepus::Value(1));

  page->InsertNode(parent);
  parent->InsertNode(z_child);
  page->FlushActionsAsRoot();

  ASSERT_EQ(z_child->element_container_impl()->parent(),
            page->element_container_impl());

  page->UpdateLayout(0, 0, kWidth, kHeight, {0}, {0}, {0}, nullptr, 0);
  parent->UpdateLayout(50, 60, 100, 100, {0}, {0}, {0}, nullptr, 0);
  z_child->UpdateLayout(7, 8, 30, 40, {0}, {0}, {0}, nullptr, 0);
  page->element_container_impl()->UpdateLayout(page->left(), page->top());

  auto* z_child_painting_node =
      painting_context->node_map_.at(z_child->impl_id()).get();
  ASSERT_NE(z_child_painting_node, nullptr);
  EXPECT_FLOAT_EQ(z_child_painting_node->frame_.left_, 57);
  EXPECT_FLOAT_EQ(z_child_painting_node->frame_.top_, 68);

  manager->RecordCurrentLynxUITree();

  const auto* z_child_node = FindInitialTreeNode(
      painting_context->initial_tree_nodes_, z_child->impl_id());
  ASSERT_NE(z_child_node, nullptr);
  EXPECT_TRUE(z_child_node->has_parent);
  EXPECT_EQ(z_child_node->parent, page->impl_id());
  EXPECT_FLOAT_EQ(z_child_node->x, z_child_painting_node->frame_.left_);
  EXPECT_FLOAT_EQ(z_child_node->y, z_child_painting_node->frame_.top_);
}

TEST_F(ElementManagerTest, CreateFiberNode) {
  base::String tag("view");
  auto node = manager->CreateFiberNode(tag);

  EXPECT_EQ(node->GetTag(), tag.str());
}

TEST_F(ElementManagerTest, ExternalMemorySnapshotUsesRepresentativeSize) {
  manager->config_->SetEnableFiberArch(true);
  EXPECT_EQ(manager->node_manager()->GetExternalMemorySnapshot().total_size, 0);

  auto page = manager->CreateFiberPage("page", 11);
  auto attached = manager->CreateFiberNode("view");
  auto detached = manager->CreateFiberNode("view");
  page->InsertNode(attached);

  const int64_t unit_size = attached->GetMemoryUsage();
  auto snapshot = manager->node_manager()->GetExternalMemorySnapshot();
  EXPECT_EQ(snapshot.total_size, 3 * unit_size);
  EXPECT_EQ(snapshot.garbage_size, unit_size);

  page->InsertNode(detached);
  snapshot = manager->node_manager()->GetExternalMemorySnapshot();
  EXPECT_EQ(snapshot.total_size, 3 * unit_size);
  EXPECT_EQ(snapshot.garbage_size, 0);

  page->RemoveNode(attached);
  snapshot = manager->node_manager()->GetExternalMemorySnapshot();
  EXPECT_EQ(snapshot.total_size, 3 * unit_size);
  EXPECT_EQ(snapshot.garbage_size, unit_size);
}

TEST_F(ElementManagerTest,
       NonMoveRemovalsRequestExternalMemoryReportOncePerPatching) {
  auto* platform_ref = static_cast<MockPaintingContextPlatformRef*>(
      manager->painting_context()->impl()->GetPlatformRef().get());

  manager->painting_context()->RemovePaintingNode(1, 2, 0, true);
  EXPECT_TRUE(platform_ref->remove_ids_.empty());
  EXPECT_EQ(platform_ref->external_memory_report_request_count_, 0);

  manager->painting_context()->RemovePaintingNode(1, 3, 0, false);
  EXPECT_EQ(platform_ref->external_memory_report_request_count_, 0);
  manager->painting_context()->RemovePaintingNode(1, 4, 1, false);
  EXPECT_EQ(platform_ref->external_memory_report_request_count_, 0);

  manager->painting_context()->UpdateNodeReadyPatching();
  ASSERT_EQ(platform_ref->remove_ids_.size(), 2U);
  EXPECT_EQ(platform_ref->remove_ids_[0], 3);
  EXPECT_EQ(platform_ref->remove_ids_[1], 4);
  EXPECT_TRUE(platform_ref->should_cache_external_memory_candidates_);
  EXPECT_EQ(platform_ref->node_ready_patching_update_count_, 1);
  EXPECT_EQ(platform_ref->external_memory_report_request_count_, 1);
  EXPECT_EQ(platform_ref->external_memory_report_delay_ms_, 1000);

  manager->painting_context()->UpdateNodeReadyPatching();
  EXPECT_EQ(platform_ref->external_memory_report_request_count_, 1);
}

TEST_F(ElementManagerTest, FeatureOffDoesNotRequestExternalMemoryReport) {
  {
    ScopedExternalBoolEnv feature(
        LynxEnv::Key::ENABLE_FIBER_ELEMENT_MEMORY_REPORT, false);
    CreateManager();
  }
  auto* platform_ref = static_cast<MockPaintingContextPlatformRef*>(
      manager->painting_context()->impl()->GetPlatformRef().get());

  manager->painting_context()->RemovePaintingNode(1, 2, 0, false);
  manager->painting_context()->UpdateNodeReadyPatching();

  ASSERT_EQ(platform_ref->remove_ids_.size(), 1U);
  EXPECT_EQ(platform_ref->remove_ids_[0], 2);
  EXPECT_FALSE(platform_ref->should_cache_external_memory_candidates_);
  EXPECT_EQ(platform_ref->node_ready_patching_update_count_, 1);
  EXPECT_EQ(platform_ref->external_memory_report_request_count_, 0);
}

TEST_F(ElementManagerTest, CreateFiberComponent) {
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");
  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);

  EXPECT_EQ(comp->GetTag(), "component");
  EXPECT_TRUE(comp->is_component());
  EXPECT_EQ(comp->component_path().c_str(), path.c_str());
  EXPECT_EQ(manager->GetComponent(component_id.str()), comp.get());
}

TEST_F(ElementManagerTest, CreateFiberList) {
  lepus::Value component_at_index(10);
  lepus::Value enqueue_component;
  lepus::Value component_at_indexes;

  auto list = manager->CreateFiberList(nullptr, "list", component_at_index,
                                       enqueue_component, component_at_indexes);

  EXPECT_EQ(list->GetTag(), "list");
  EXPECT_TRUE(list->is_list());
}

TEST_F(ElementManagerTest, CreateFiberWrapperElement) {
  auto wrapper = manager->CreateFiberWrapperElement();

  EXPECT_EQ(wrapper->GetTag(), "wrapper");
  EXPECT_TRUE(wrapper->is_wrapper());
}

TEST_F(ElementManagerTest, ComponentManagerFiber) {
  base::String component_id("21");
  int32_t css_id = 100;
  base::String entry_name("__Card__");
  base::String component_name("TestComp");
  base::String path("/index/components/TestComp");

  EXPECT_EQ(manager->GetComponent(component_id.str()), nullptr);

  auto comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                            component_name, path);
  EXPECT_EQ(manager->GetComponent(component_id.str()), comp.get());

  base::String component2_id("22");
  auto comp2 = manager->CreateFiberComponent(component2_id, css_id, entry_name,
                                             component_name, path);
  EXPECT_EQ(manager->GetComponent(component2_id.str()), comp2.get());

  // erase component
  comp = nullptr;
  EXPECT_EQ(manager->GetComponent(component_id.str()), nullptr);
  EXPECT_EQ(manager->GetComponent(component2_id.str()), comp2.get());

  // record component back
  comp = manager->CreateFiberComponent(component_id, css_id, entry_name,
                                       component_name, path);
  EXPECT_EQ(manager->GetComponent(component_id.str()), comp.get());

  // record another component with same id
  auto comp_copy = manager->CreateFiberComponent(
      component_id, css_id, entry_name, component_name, path);
  EXPECT_EQ(manager->GetComponent(component_id.str()), comp_copy.get());
}

TEST_F(ElementManagerTest, CreateFiberRawText) {
  auto raw_text = manager->CreateFiberRawText();

  EXPECT_EQ(raw_text->GetTag(), "raw-text");
  EXPECT_TRUE(raw_text->is_raw_text());
}

TEST_F(ElementManagerTest, IsTagVirtual) {
  EXPECT_EQ(manager->IsShadowNodeVirtual("view"), false);
  EXPECT_EQ(manager->IsShadowNodeVirtual("inline-text"), true);
  EXPECT_EQ(manager->IsShadowNodeVirtual("inline-image"), true);
  EXPECT_EQ(manager->IsShadowNodeVirtual("image"), false);
  EXPECT_EQ(manager->IsShadowNodeVirtual("text"), false);
}

TEST_F(ElementManagerTest, IsTagCustom) {
  EXPECT_EQ(static_cast<bool>(manager->GetNodeInfoByTag("view") &
                              LayoutNodeType::CUSTOM),
            false);
  EXPECT_EQ(static_cast<bool>(manager->GetNodeInfoByTag("inline-text") &
                              LayoutNodeType::CUSTOM),
            true);
  EXPECT_EQ(static_cast<bool>(manager->GetNodeInfoByTag("inline-image") &
                              LayoutNodeType::CUSTOM),
            true);
  EXPECT_EQ(static_cast<bool>(manager->GetNodeInfoByTag("image") &
                              LayoutNodeType::CUSTOM),
            false);
  EXPECT_EQ(static_cast<bool>(manager->GetNodeInfoByTag("text") &
                              LayoutNodeType::CUSTOM),
            true);
}

TEST_F(ElementManagerTest, CreateFiberElementView) {
  base::String tag("view");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_view());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_VIEW;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_view());
}

TEST_F(ElementManagerTest, CreateFiberElementText) {
  base::String tag("text");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_text());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_TEXT;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_text());
}

TEST_F(ElementManagerTest, CreateFiberElementRawText) {
  base::String tag("raw-text");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_raw_text());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_RAW_TEXT;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_raw_text());
}

TEST_F(ElementManagerTest, CreateFiberElementImage) {
  base::String tag("image");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_image());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_IMAGE;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_image());

  base::String raw_tag("custom-image");
  auto static_node =
      ElementManager::StaticCreateFiberElement(tag_enum, raw_tag);

  EXPECT_EQ(static_node->GetTag(), tag.str());

  EXPECT_TRUE(static_node->is_image());
}

TEST_F(ElementManagerTest, CreateFiberElementEcomImage) {
  base::String tag("ecom-image");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_image());

  node = manager->CreateFiberNode(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_image());

  auto static_node =
      ElementManager::StaticCreateFiberElement(ELEMENT_OTHER, tag);

  EXPECT_EQ(static_node->GetTag(), tag.str());

  EXPECT_TRUE(static_node->is_image());
}

TEST_F(ElementManagerTest, CreateFiberElementScrollView) {
  base::String tag("scroll-view");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_scroll_view());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_SCROLL_VIEW;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_scroll_view());
}

TEST_F(ElementManagerTest, CreateFiberElementList) {
  base::String tag("list");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_list());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_LIST;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_list());
}

TEST_F(ElementManagerTest, CreateFiberElementComponent) {
  base::String tag("component");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_component());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_COMPONENT;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_component());
}

TEST_F(ElementManagerTest, CreateFiberElementPage) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);
  base::String tag("page");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_page());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_PAGE;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_page());
}

TEST_F(ElementManagerTest, CreateFiberElementNone) {
  base::String tag("none");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_TRUE(node->is_none());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_NONE;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_TRUE(node->is_none());
}

TEST_F(ElementManagerTest, CreateFiberElementWrapper) {
  base::String tag("wrapper");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_wrapper());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_WRAPPER;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_wrapper());
}

TEST_F(ElementManagerTest, CreateFiberElementXText) {
  base::String tag("x-text");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_text());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_X_TEXT;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_text());
}

TEST_F(ElementManagerTest, CreateFiberElementXScrollView) {
  base::String tag("x-scroll-view");
  auto node = manager->CreateFiberElement(tag);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_scroll_view());

  ElementBuiltInTagEnum tag_enum = ElementBuiltInTagEnum::ELEMENT_X_SCROLL_VIEW;
  node = manager->CreateFiberElement(tag_enum);

  EXPECT_EQ(node->GetTag(), tag.str());

  EXPECT_TRUE(node->is_scroll_view());
}

TEST_F(ElementManagerTest, ReloadTemplateEvent) {
  base::String tag("none");
  auto node = manager->CreateFiberElement(tag);
  node->CreateElementContainer(false);
  auto options = std::make_shared<PipelineOptions>();
  options->is_reload_template = true;
  auto config = std::make_shared<PageConfig>();
  config->SetEnableReloadLifecycle(true);
  manager->SetConfig(config);
  manager->OnPatchFinish(options, node.get());
  auto* mock_platform_ref = reinterpret_cast<MockPaintingContextPlatformRef*>(
      manager->painting_context()->impl()->GetPlatformRef().get());
  auto reload_ids = mock_platform_ref->reload_ids_;
  EXPECT_EQ(reload_ids.size(), 1);
  EXPECT_EQ(reload_ids.front(), node->impl_id());

  mock_platform_ref->reload_ids_.clear();
  config->SetEnableReloadLifecycle(false);
  manager->OnPatchFinish(options, node.get());
  reload_ids = mock_platform_ref->reload_ids_;
  EXPECT_EQ(reload_ids.size(), 0);
}

// Mock SharedCSSFragmentWrapper for testing adopted stylesheets
class MockSharedCSSFragmentWrapper : public tasm::SharedCSSFragmentWrapper {
 public:
  MockSharedCSSFragmentWrapper() : SharedCSSFragmentWrapper(nullptr) {}

  // Simple mock fragment pointer accessible for tests
  std::unique_ptr<tasm::SharedCSSFragment> fragment_;
};

TEST_F(ElementManagerTest, AdoptStyleSheet_Basic) {
  // Create a mock wrapper
  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());

  // Initially no adopted stylesheets
  EXPECT_TRUE(manager->GetAdoptedStyleSheets().empty());

  // Adopt the stylesheet
  manager->AdoptStyleSheet(wrapper);

  // Verify it was added
  const auto& adopted_sheets = manager->GetAdoptedStyleSheets();
  EXPECT_EQ(adopted_sheets.size(), 1);
  EXPECT_EQ(adopted_sheets[0].get(), wrapper.get());
}

TEST_F(ElementManagerTest, AdoptStyleSheet_Multiple) {
  // Create multiple mock wrappers
  auto wrapper1 = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  auto wrapper2 = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  auto wrapper3 = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());

  // Adopt multiple stylesheets
  manager->AdoptStyleSheet(wrapper1);
  manager->AdoptStyleSheet(wrapper2);
  manager->AdoptStyleSheet(wrapper3);

  // Verify all were added in order
  const auto& adopted_sheets = manager->GetAdoptedStyleSheets();
  EXPECT_EQ(adopted_sheets.size(), 3);
  EXPECT_EQ(adopted_sheets[0].get(), wrapper1.get());
  EXPECT_EQ(adopted_sheets[1].get(), wrapper2.get());
  EXPECT_EQ(adopted_sheets[2].get(), wrapper3.get());
}

TEST_F(ElementManagerTest, ClearAdoptedStyleSheets) {
  // Create and adopt a stylesheet
  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  manager->AdoptStyleSheet(wrapper);

  // Verify it was added
  EXPECT_EQ(manager->GetAdoptedStyleSheets().size(), 1);

  // Clear adopted stylesheets
  manager->ClearAdoptedStyleSheets();

  // Verify list is empty
  EXPECT_TRUE(manager->GetAdoptedStyleSheets().empty());

  // Can adopt again after clearing
  manager->AdoptStyleSheet(wrapper);
  EXPECT_EQ(manager->GetAdoptedStyleSheets().size(), 1);
}

TEST_F(ElementManagerTest, AdoptStyleSheet_NullWrapper) {
  // Test adopting null wrapper (should not crash)
  manager->AdoptStyleSheet(fml::RefPtr<MockSharedCSSFragmentWrapper>());

  // List should still be valid (may contain null or be empty depending on
  // implementation) The important thing is it doesn't crash
  SUCCEED();
}

TEST_F(ElementManagerTest, GetAdoptedStyleSheets_ThreadSafeCopy) {
  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  manager->AdoptStyleSheet(wrapper);

  const auto sheets1 = manager->GetAdoptedStyleSheets();
  const auto sheets2 = manager->GetAdoptedStyleSheets();

  // GetAdoptedStyleSheets returns a copy under the shared lock for thread
  // safety, so each call produces a distinct vector object.
  EXPECT_NE(&sheets1, &sheets2);
  EXPECT_EQ(sheets1.size(), sheets2.size());
  EXPECT_EQ(sheets1.size(), 1u);
}

TEST_F(ElementManagerTest, AdoptedStylesheets_IntegrationWithFiberElement) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  base::String component_id("test-component");
  int32_t css_id = 100;
  auto root = manager->CreateFiberPage(component_id, css_id);

  auto wrapper = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  manager->AdoptStyleSheet(wrapper);

  EXPECT_EQ(manager->GetAdoptedStyleSheets().size(), 1);

  const auto& adopted_sheets = manager->GetAdoptedStyleSheets();
  EXPECT_FALSE(adopted_sheets.empty());

  manager->ClearAdoptedStyleSheets();
  EXPECT_TRUE(manager->GetAdoptedStyleSheets().empty());
}

TEST_F(ElementManagerTest, AdoptedStylesheets_MultipleAdoption) {
  auto wrapper1 = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  auto wrapper2 = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());
  auto wrapper3 = fml::AdoptRef<MockSharedCSSFragmentWrapper>(
      new MockSharedCSSFragmentWrapper());

  manager->AdoptStyleSheet(wrapper1);
  manager->AdoptStyleSheet(wrapper2);
  manager->AdoptStyleSheet(wrapper3);

  EXPECT_EQ(manager->GetAdoptedStyleSheets().size(), 3);

  manager->ClearAdoptedStyleSheets();
  EXPECT_TRUE(manager->GetAdoptedStyleSheets().empty());

  manager->AdoptStyleSheet(wrapper1);
  EXPECT_EQ(manager->GetAdoptedStyleSheets().size(), 1);
}

TEST_F(ElementManagerTest, EnableAnimationForwardUpdatePreservation_Default) {
  // Default value should be false (initialized in header)
  EXPECT_FALSE(manager->EnableAnimationForwardUpdatePreservation());
}

TEST_F(ElementManagerTest, EnableAnimationForwardUpdatePreservation_True) {
  // Set config with TRUE_VALUE
  auto config = std::make_shared<PageConfig>();
  config->enable_animation_forward_update_preservation_ = true;
  manager->SetConfig(config);
  EXPECT_TRUE(manager->EnableAnimationForwardUpdatePreservation());
}

TEST_F(ElementManagerTest, EnableAnimationForwardUpdatePreservation_False) {
  // Set config with FALSE_VALUE
  auto config = std::make_shared<PageConfig>();
  config->enable_animation_forward_update_preservation_ = false;
  manager->SetConfig(config);
  EXPECT_FALSE(manager->EnableAnimationForwardUpdatePreservation());
}

TEST_F(ElementManagerTest, LayoutUIOperationOverlapEligibility) {
  manager->page_options_.SetEmbeddedMode(EmbeddedMode::LAYOUT_IN_ELEMENT);
  EXPECT_FALSE(manager->IsLayoutUIOperationOverlapModeOn());

  manager->page_options_.SetEmbeddedMode(EmbeddedMode::FRAGMENT_LAYER_RENDER);
  EXPECT_FALSE(manager->IsLayoutUIOperationOverlapModeOn());

  manager->page_options_.SetEmbeddedMode(static_cast<EmbeddedMode>(
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT) |
      static_cast<int32_t>(EmbeddedMode::FRAGMENT_LAYER_RENDER)));
  EXPECT_TRUE(manager->IsLayoutUIOperationOverlapModeOn());

  manager->page_options_.SetEmbeddedMode(static_cast<EmbeddedMode>(
      static_cast<int32_t>(EmbeddedMode::LAYOUT_IN_ELEMENT) |
      static_cast<int32_t>(EmbeddedMode::FRAGMENT_LAYER_RENDER) |
      static_cast<int32_t>(EmbeddedMode::USE_TEXT_SERVICE)));
  EXPECT_TRUE(manager->IsLayoutUIOperationOverlapModeOn());

  manager->SetThreadStrategy(base::ThreadStrategyForRendering::ALL_ON_UI);
  EXPECT_FALSE(manager->ShouldRunLayoutConcurrentWithUIOperations());

  manager->SetThreadStrategy(base::ThreadStrategyForRendering::PART_ON_LAYOUT);
  EXPECT_TRUE(manager->ShouldRunLayoutConcurrentWithUIOperations());

  manager->SetThreadStrategy(base::ThreadStrategyForRendering::MOST_ON_TASM);
  EXPECT_FALSE(manager->ShouldRunLayoutConcurrentWithUIOperations());

  manager->SetThreadStrategy(base::ThreadStrategyForRendering::MULTI_THREADS);
  EXPECT_FALSE(manager->ShouldRunLayoutConcurrentWithUIOperations());
}

class ElementManagerNonPartLayoutTest
    : public ElementManagerUIOperationOverlapTest,
      public ::testing::WithParamInterface<base::ThreadStrategyForRendering> {};

TEST_P(ElementManagerNonPartLayoutTest, DoesNotFlushBeforeCurrentThreadLayout) {
  const auto thread_strategy = GetParam();
  ConfigureManager(kLayoutFragmentTextMode, thread_strategy);
  auto queue = SetUIOperationQueue(thread_strategy);
  CreateAndFlushPage();
  manager->has_viewport_ready_ = true;
  queue->EnqueueUIOperation([]() {});
  ASSERT_TRUE(queue->HasPendingOperations());
  painting_context->flush_called_ = false;
  const auto request_layout_thread = std::this_thread::get_id();
  std::atomic_bool flush_observed_before_layout{false};
  std::atomic_bool layout_on_request_thread{false};
  painting_context->on_layout_ = [&]() {
    flush_observed_before_layout = painting_context->flush_called_.load();
    layout_on_request_thread =
        std::this_thread::get_id() == request_layout_thread;
  };

  auto options = std::make_shared<PipelineOptions>();
  manager->RequestLayout(options);

  EXPECT_TRUE(options->has_layout);
  EXPECT_FALSE(flush_observed_before_layout);
  EXPECT_TRUE(layout_on_request_thread);
  EXPECT_TRUE(painting_context->flush_called_);
}

INSTANTIATE_TEST_SUITE_P(
    NonPartStrategies, ElementManagerNonPartLayoutTest,
    ::testing::Values(base::ThreadStrategyForRendering::ALL_ON_UI,
                      base::ThreadStrategyForRendering::MOST_ON_TASM,
                      base::ThreadStrategyForRendering::MULTI_THREADS));

TEST_F(ElementManagerUIOperationOverlapTest,
       EmptyQueueKeepsLayoutOnCurrentThread) {
  ConfigureManager(kLayoutFragmentMode,
                   base::ThreadStrategyForRendering::PART_ON_LAYOUT);
  CreateAndFlushPage();
  manager->has_viewport_ready_ = true;
  auto queue =
      SetUIOperationQueue(base::ThreadStrategyForRendering::PART_ON_LAYOUT);
  ASSERT_FALSE(queue->HasPendingOperations());
  const auto request_layout_thread = std::this_thread::get_id();
  std::atomic_bool layout_on_request_thread{false};
  painting_context->on_layout_ = [&]() {
    layout_on_request_thread =
        std::this_thread::get_id() == request_layout_thread;
  };

  auto options = std::make_shared<PipelineOptions>();
  manager->RequestLayout(options);

  EXPECT_TRUE(options->has_layout);
  EXPECT_TRUE(layout_on_request_thread);
}

TEST_F(ElementManagerUIOperationOverlapTest,
       PreservesFallbackCreateOperationUntilViewportReadyLayout) {
  ConfigureManager(kLayoutFragmentMode,
                   base::ThreadStrategyForRendering::PART_ON_LAYOUT);
  auto page = manager->CreateFiberPage("page", 11);
  page->FlushActionsAsRoot();
  auto queue =
      SetUIOperationQueue(base::ThreadStrategyForRendering::PART_ON_LAYOUT);
  auto fallback_ui = manager->CreateFiberElement("x-custom");
  ASSERT_FALSE(queue->HasPendingOperations());
  page->InsertNode(fallback_ui);
  page->FlushActionsAsRoot();
  ASSERT_TRUE(queue->HasPendingOperations());
  painting_context->flush_called_ = false;

  auto pre_viewport_options = std::make_shared<PipelineOptions>();
  manager->RequestLayout(pre_viewport_options);

  EXPECT_FALSE(pre_viewport_options->has_layout);
  EXPECT_FALSE(painting_context->flush_called_);
  ASSERT_TRUE(queue->HasPendingOperations());

  const auto request_layout_thread = std::this_thread::get_id();
  fml::ManualResetWaitableEvent layout_entered;
  fml::ManualResetWaitableEvent release_layout;
  std::atomic_bool overlap_observed{false};
  std::atomic_bool layout_on_concurrent_loop{false};
  std::atomic_bool layout_wait_timed_out{false};
  std::atomic_bool ui_operation_on_request_thread{false};
  std::atomic_bool layout_ui_operation_executed{false};
  painting_context->on_layout_ = [&]() {
    queue->EnqueueUIOperation([&]() { layout_ui_operation_executed = true; });
    layout_on_concurrent_loop =
        base::TaskRunnerManufactor::IsOnConcurrentLoopWorker(
            base::ConcurrentTaskType::HIGH_PRIORITY);
    layout_entered.Signal();
    layout_wait_timed_out =
        release_layout.WaitWithTimeout(fml::TimeDelta::FromSeconds(3));
  };
  painting_context->on_ui_operation_ = [&]() {
    ui_operation_on_request_thread =
        std::this_thread::get_id() == request_layout_thread;
    overlap_observed =
        !layout_entered.WaitWithTimeout(fml::TimeDelta::FromSeconds(3));
    release_layout.Signal();
  };
  manager->has_viewport_ready_ = true;

  auto options = std::make_shared<PipelineOptions>();
  options->need_timestamps = true;
  RecordingTimingDelegate timing_delegate;
  {
    TimingCollector::Scope<RecordingTimingDelegate> timing_scope(
        &timing_delegate, options);
    manager->RequestLayout(options);
  }

  EXPECT_TRUE(options->has_layout);
  EXPECT_FALSE(queue->HasPendingOperations());
  EXPECT_TRUE(overlap_observed);
  EXPECT_TRUE(layout_on_concurrent_loop);
  EXPECT_FALSE(layout_wait_timed_out);
  EXPECT_TRUE(ui_operation_on_request_thread);
  EXPECT_TRUE(layout_ui_operation_executed);
  ASSERT_TRUE(timing_delegate.timing_.has_value());
  const auto& timings = timing_delegate.timing_->timings_;
  ASSERT_TRUE(timings.contains(timing::kLayoutStart));
  ASSERT_TRUE(timings.contains(timing::kLayoutEnd));
  EXPECT_GT(timings.find(timing::kLayoutStart)->second, 0u);
  EXPECT_GE(timings.find(timing::kLayoutEnd)->second,
            timings.find(timing::kLayoutStart)->second);
}

TEST_F(ElementManagerTest,
       IntrinsicFontFacesResolvedOnceAcrossDecoratorsInSameView) {
  CSSFontFaceRuleMap fontfaces;
  auto font_rule = std::make_shared<CSSFontFaceRule>(
      "custom-font", CSSFontFaceAttrsMap{{"src", "url(custom.woff2)"}});
  fontfaces["custom-font"].push_back(font_rule);

  SharedCSSFragment shared(1, std::vector<int32_t>{}, CSSParserTokenMap{},
                           CSSKeyframesTokenMap{}, std::move(fontfaces));

  // Two decorators in the same LynxView sharing the same intrinsic fragment.
  CSSFragmentDecorator decorator_a(&shared, manager.get());
  CSSFragmentDecorator decorator_b(&shared, manager.get());

  size_t visit_count_a = 0;
  decorator_a.ForEachUnresolvedFontFaceMap(
      [](const CSSFontFaceRuleMap& map, void* cb_data) {
        if (!map.empty()) {
          ++*static_cast<size_t*>(cb_data);
        }
      },
      &visit_count_a);
  EXPECT_EQ(visit_count_a, 1u);

  // Mark the first decorator resolved; this should also register the shared
  // intrinsic fragment in the ElementManager so the second decorator skips it.
  decorator_a.MarkFontFacesResolved(true);

  size_t visit_count_b = 0;
  decorator_b.ForEachUnresolvedFontFaceMap(
      [](const CSSFontFaceRuleMap& map, void* cb_data) {
        if (!map.empty()) {
          ++*static_cast<size_t*>(cb_data);
        }
      },
      &visit_count_b);
  EXPECT_EQ(visit_count_b, 0u);

  // The shared intrinsic fragment itself must remain unmarked; only the
  // per-LynxView ElementManager tracks the resolved state.
  EXPECT_FALSE(shared.HasFontFacesResolved());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
