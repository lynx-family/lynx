// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#define protected public

#include "core/renderer/dom/fiber/scroll_element.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/page_element.h"
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
        *tasm_mediator.get(), std::move(unique_manager), 0);

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
  child_view_0->overflow_ = Element::OVERFLOW_XY;

  auto child_view_1 = manager->CreateFiberView();
  child_view_1->SetStyle(CSSPropertyID::kPropertyIDWidth,
                         lepus::Value("200px"));
  child_view_1->SetStyle(CSSPropertyID::kPropertyIDHeight,
                         lepus::Value("200px"));
  child_view_1->overflow_ = Element::OVERFLOW_XY;

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

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
