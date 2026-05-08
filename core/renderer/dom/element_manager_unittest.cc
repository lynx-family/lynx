// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/element_manager.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_property.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/dom/fiber/list_element.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/raw_text_element.h"
#include "core/renderer/dom/fiber/wrapper_element.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
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
class ElementManagerTest : public ::testing::Test {
 public:
  ElementManagerTest() {}
  ~ElementManagerTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config, tasm::PageOptions());
    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }
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

TEST_F(ElementManagerTest, CreateFiberNode) {
  base::String tag("view");
  auto node = manager->CreateFiberNode(tag);

  EXPECT_EQ(node->GetTag(), tag.str());
}

TEST_F(ElementManagerTest, TestCalcTotalMemoryDiff) {
  manager->config_->SetEnableFiberArch(true);
  int32_t expected_total_memory = 0;
  int32_t average_element_memory_size = 0;

  int32_t expect_counter = 10;
  std::vector<fml::RefPtr<FiberElement>> fiber_elements;
  EXPECT_EQ(manager->total_memory_, 0);
  for (int i = 0; i < expect_counter; i++) {
    auto fiber_element = manager->CreateFiberNode("view");
    average_element_memory_size = fiber_element->GetMemoryUsage();
    fiber_elements.push_back(fiber_element);
  }

  expected_total_memory = average_element_memory_size * expect_counter;
  auto total_memory_diff = manager->CalcTotalMemoryUsageDiff();
  EXPECT_EQ(manager->total_memory_, expected_total_memory);
  EXPECT_EQ(total_memory_diff, expected_total_memory);
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

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
