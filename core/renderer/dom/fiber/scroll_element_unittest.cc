// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <mutex>
#include <optional>
#include <string>
#include <utility>

#define private public
#define protected public

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/page_element.h"
#include "core/renderer/dom/fiber/scroll_element.h"
#include "core/renderer/dom/fiber/view_element.h"
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

class ScopedScrollExternalBoolEnv {
 public:
  ScopedScrollExternalBoolEnv(LynxEnv::Key key, bool value) : key_(key) {
    auto& env = LynxEnv::GetInstance();
    std::lock_guard<std::recursive_mutex> lock(env.external_env_mutex_);
    auto it = env.external_env_map_.find(key_);
    if (it != env.external_env_map_.end()) {
      previous_value_ = it->second;
    }
    env.external_env_map_[key_] =
        value ? LynxEnv::kLocalEnvValueTrue : LynxEnv::kLocalEnvValueFalse;
  }

  ~ScopedScrollExternalBoolEnv() {
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

class ScrollElementTest : public ::testing::Test {
 public:
  ScrollElementTest() {}
  ~ScrollElementTest() override {}
  lynx::tasm::ElementManager* manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  std::shared_ptr<lynx::tasm::TemplateAssembler> tasm;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto unique_manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    manager = unique_manager.get();
    tasm = std::make_shared<lynx::tasm::TemplateAssembler>(
        *tasm_mediator.get(), std::move(unique_manager), tasm_mediator.get(),
        0);

    auto test_entry = std::make_shared<TemplateEntry>();
    tasm->template_entries_.insert({"test_entry", test_entry});

    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }
};

TEST_F(ScrollElementTest, TestCreate) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto scroll_view = manager->CreateFiberScrollView("scroll-view");
  EXPECT_FALSE(scroll_view->CanHasLayoutOnlyChildren());
}

TEST_F(ScrollElementTest,
       ScrollAttributeCachesAndRemovesCommittedLinearOrientationStyle) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto scroll_view = manager->CreateFiberScrollView("scroll-view");
  scroll_view->SetAttributeInternal("scroll-x", lepus::Value("true"));

  const auto* cached_styles = scroll_view->PeekCachedStylesFromAttributes();
  ASSERT_NE(cached_styles, nullptr);
  auto cached_it =
      cached_styles->find(CSSPropertyID::kPropertyIDLinearOrientation);
  ASSERT_TRUE(cached_it != cached_styles->end());
  EXPECT_EQ(cached_it->second,
            CSSValue(starlight::LinearOrientationType::kHorizontal));

  const auto* committed_styles =
      scroll_view->PeekCommittedStylesFromAttributes();
  ASSERT_NE(committed_styles, nullptr);
  auto committed_it =
      committed_styles->find(CSSPropertyID::kPropertyIDLinearOrientation);
  ASSERT_TRUE(committed_it != committed_styles->end());
  EXPECT_EQ(committed_it->second,
            CSSValue(starlight::LinearOrientationType::kHorizontal));

  scroll_view->ResetAttribute("scroll-x");
  EXPECT_EQ(scroll_view->PeekCachedStylesFromAttributes(), nullptr);
  EXPECT_EQ(scroll_view->PeekCommittedStylesFromAttributes(), nullptr);
}

TEST_F(ScrollElementTest, TestChildInsert0) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("page", 11);
  auto scroll_view = manager->CreateFiberScrollView("scroll-view");
  auto wrapper = manager->CreateFiberWrapperElement();

  auto child_view_0 = manager->CreateFiberView();
  child_view_0->SetStyle(CSSPropertyID::kPropertyIDWidth,
                         lepus::Value("200px"));
  child_view_0->SetStyle(CSSPropertyID::kPropertyIDHeight,
                         lepus::Value("200px"));
  child_view_0->computed_css_style()->SetOverflowDefaultVisible(true);

  auto child_view_1 = manager->CreateFiberView();
  child_view_1->SetStyle(CSSPropertyID::kPropertyIDWidth,
                         lepus::Value("200px"));
  child_view_1->SetStyle(CSSPropertyID::kPropertyIDHeight,
                         lepus::Value("200px"));
  child_view_1->computed_css_style()->SetOverflowDefaultVisible(true);

  page->InsertNode(scroll_view);
  page->InsertNode(child_view_0);
  scroll_view->InsertNode(child_view_1);
  scroll_view->InsertNode(wrapper);

  page->FlushActionsAsRoot();

  EXPECT_TRUE(child_view_0->IsLayoutOnly());
  EXPECT_TRUE(wrapper->IsLayoutOnly());
  // Scroll's child should not be layout only.
  EXPECT_FALSE(child_view_1->IsLayoutOnly());
}

// Verifies how the scroll platform renderer env affects each scroll path.
TEST_F(ScrollElementTest, PlatformRendererEnvOnlyAppliesToDefaultScrollView) {
  ScopedScrollExternalBoolEnv enable_platform_renderer_scroll(
      LynxEnv::Key::ENABLE_PLATFORM_RENDERER_SCROLL, true);

  auto page = manager->CreateFiberPage("page", 11);
  auto create_scroll_with_child = [this, &page](const base::String& tag,
                                                bool enable_new_arch) {
    auto scroll = manager->CreateFiberScrollView(tag);
    auto child = manager->CreateFiberView();
    scroll->InsertNode(child);
    // Element::OnNodeAdded initially marks the child as a direct child of a
    // compatible component.
    EXPECT_TRUE(child->is_direct_child_of_compatible_component());
    if (enable_new_arch) {
      scroll->SetAttribute(kScrollNewArch, lepus::Value(kTrue));
    }
    page->InsertNode(scroll);
    return std::make_pair(scroll, child);
  };

  // Case 1: The default scroll-view enables its dedicated platform renderer
  // and marks its child as not being a direct child of a compatible component.
  auto [scroll, scroll_child] = create_scroll_with_child(
      BASE_STATIC_STRING(kElementScrollViewTag), false);

  // Case 2: A scroll-view using scroll-view-new-arch keeps the new-arch path
  // and its child's compatible-component flag.
  auto [new_arch_scroll, new_arch_child] =
      create_scroll_with_child(BASE_STATIC_STRING(kElementScrollViewTag), true);

  // Case 3: x-scroll-view does not enable the dedicated platform renderer and
  // keeps its child's compatible-component flag.
  auto [x_scroll, x_scroll_child] = create_scroll_with_child(
      BASE_STATIC_STRING(kElementXScrollViewTag), false);

  // Case 4: x-nested-scroll-view does not enable the dedicated platform
  // renderer and keeps its child's compatible-component flag.
  auto [x_nested_scroll, x_nested_scroll_child] = create_scroll_with_child(
      BASE_STATIC_STRING(kElementXNestedScrollViewTag), false);

  page->FlushActionsAsRoot();

  ASSERT_TRUE(scroll->enable_platform_renderer_.has_value());
  EXPECT_TRUE(*scroll->enable_platform_renderer_);
  EXPECT_FALSE(scroll_child->is_direct_child_of_compatible_component());

  ASSERT_TRUE(new_arch_scroll->enable_platform_renderer_.has_value());
  EXPECT_FALSE(*new_arch_scroll->enable_platform_renderer_);
  EXPECT_TRUE(new_arch_child->is_direct_child_of_compatible_component());

  ASSERT_TRUE(x_scroll->enable_platform_renderer_.has_value());
  EXPECT_FALSE(*x_scroll->enable_platform_renderer_);
  EXPECT_TRUE(x_scroll_child->is_direct_child_of_compatible_component());

  ASSERT_TRUE(x_nested_scroll->enable_platform_renderer_.has_value());
  EXPECT_FALSE(*x_nested_scroll->enable_platform_renderer_);
  EXPECT_TRUE(x_nested_scroll_child->is_direct_child_of_compatible_component());
}

TEST_F(ScrollElementTest,
       PlatformRendererEnvDisabledKeepsDefaultScrollViewCompatible) {
  ScopedScrollExternalBoolEnv enable_platform_renderer_scroll(
      LynxEnv::Key::ENABLE_PLATFORM_RENDERER_SCROLL, false);

  auto page = manager->CreateFiberPage("page", 11);
  auto scroll =
      manager->CreateFiberScrollView(BASE_STATIC_STRING(kElementScrollViewTag));
  auto child = manager->CreateFiberView();
  scroll->InsertNode(child);
  page->InsertNode(scroll);

  page->FlushActionsAsRoot();

  ASSERT_TRUE(scroll->enable_platform_renderer_.has_value());
  EXPECT_FALSE(*scroll->enable_platform_renderer_);
  EXPECT_TRUE(child->is_direct_child_of_compatible_component());
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
