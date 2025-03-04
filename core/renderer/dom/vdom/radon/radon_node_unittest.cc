// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define protected public
#define private public

#include "core/renderer/dom/vdom/radon/radon_node.h"

#include "base/include/value/base_string.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_element.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {
static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;
class RadonNodeTest : public ::testing::Test {
 public:
  RadonNodeTest() {}
  ~RadonNodeTest() override {}
  std::unique_ptr<PageProxy> page_proxy;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;
  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    auto manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
    page_proxy =
        std::make_unique<PageProxy>(nullptr, std::move(manager), nullptr);
  }
};

TEST_F(RadonNodeTest, GetAttributesForWorklet) {
  auto radon_node = std::make_shared<RadonNode>(nullptr, "view", 123);
  auto radon_element =
      fml::MakeRefCounted<RadonElement>("view", radon_node->attribute_holder(),
                                        page_proxy->element_manager().get());

  radon_element->SetAttribute("test", lepus::Value(2));
  EXPECT_EQ(radon_element->GetAttributesForWorklet().at("test"),
            lepus::Value(2));

  radon_element->SetAttribute("test", lepus::Value(3));
  EXPECT_EQ(radon_element->GetAttributesForWorklet().at("test"),
            lepus::Value(3));
}

TEST_F(RadonNodeTest, GetOriginalNodeIndex) {
  auto radon_node1 = new RadonNode(nullptr, "view", 123);
  EXPECT_TRUE(radon_node1->GetOriginalNodeIndex() == 123);

  auto radon_node2 = new RadonNode(nullptr, "view", 124);
  auto child1 = new RadonNode(nullptr, "view", 0);
  radon_node2->AddChild(std::unique_ptr<RadonBase>(child1));
  EXPECT_TRUE(child1->GetOriginalNodeIndex() == 124);

  auto child2 = new RadonNode(nullptr, "view", 0);
  child1->AddChild(std::unique_ptr<RadonBase>(child2));
  EXPECT_TRUE(child2->GetOriginalNodeIndex() == 124);
}

TEST_F(RadonNodeTest, CreateFiberElementView) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node->CreateElementIfNeeded();
  auto* element = radon_node->element();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(element->is_view());
  radon_node->DispatchFirstTime();
  EXPECT_TRUE(static_cast<FiberElement*>(element)->dirty_ &
              FiberElement::kDirtyStyle);
}

TEST_F(RadonNodeTest, CreateFiberElementText) {
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "text", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(element->is_text());
}

TEST_F(RadonNodeTest, CreateFiberElementXText) {
  auto radon_node =
      std::make_unique<RadonNode>(page_proxy.get(), "x-text", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(element->is_text());
}

TEST_F(RadonNodeTest, CreateFiberElementInlineText) {
  auto radon_node =
      std::make_unique<RadonNode>(page_proxy.get(), "inline-text", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(element->is_text());
}

TEST_F(RadonNodeTest, CreateFiberElementXInlineText) {
  auto radon_node =
      std::make_unique<RadonNode>(page_proxy.get(), "x-inline-text", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(element->is_text());
}

TEST_F(RadonNodeTest, CreateFiberElementRawText) {
  auto radon_node =
      std::make_unique<RadonNode>(page_proxy.get(), "raw-text", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_raw_text());
}

TEST_F(RadonNodeTest, CreateFiberElementImage) {
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "image", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(element->is_image());
}

TEST_F(RadonNodeTest, CreateFiberElementScrollView) {
  auto radon_node =
      std::make_unique<RadonNode>(page_proxy.get(), "scroll-view", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_scroll_view());
}

TEST_F(RadonNodeTest, CreateFiberElementXScrollView) {
  auto radon_node =
      std::make_unique<RadonNode>(page_proxy.get(), "x-scroll-view", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_scroll_view());
}

TEST_F(RadonNodeTest, CreateFiberElementXNestedScrollView) {
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(),
                                                "x-nested-scroll-view", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_scroll_view());
}

TEST_F(RadonNodeTest, CreateFiberElementList) {
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "list", 123);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(element->is_list());
}

TEST_F(RadonNodeTest, CreateFiberElementComponent) {
  auto radon_node =
      std::make_unique<RadonComponent>(page_proxy.get(), 0, nullptr, nullptr,
                                       nullptr, nullptr, 123, "component");
  radon_node->SetComponent(nullptr);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_component());
  EXPECT_FALSE(static_cast<FiberElement*>(element.get())->is_wrapper());
}

TEST_F(RadonNodeTest, CreateFiberElementWrapperComponent) {
  auto radon_node =
      std::make_unique<RadonComponent>(page_proxy.get(), 0, nullptr, nullptr,
                                       nullptr, nullptr, 123, "component");
  radon_node->SetRemoveComponentElement("removeComponentElement",
                                        lepus::Value(true));
  radon_node->SetComponent(nullptr);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_component());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_wrapper());
}

TEST_F(RadonNodeTest, CreateFiberElementPage) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node = std::make_unique<RadonPage>(page_proxy.get(), 0, nullptr,
                                                nullptr, nullptr, nullptr);
  radon_node->SetComponent(nullptr);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_page());
}

TEST_F(RadonNodeTest, CreateFiberElementPageByTag) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "page", 123);
  radon_node->SetComponent(nullptr);
  auto element = radon_node->CreateFiberElement();
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_page());
}

TEST_F(RadonNodeTest, TestViewCanBeLayoutOnlyWithoutOptimization) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node->CreateElementIfNeeded();
  auto* element = radon_node->element();
  element->overflow_ = FiberElement::OVERFLOW_XY;
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(element->is_view());
  EXPECT_TRUE(element->CanBeLayoutOnly());
  element->SetStyleInternal(CSSPropertyID::kPropertyIDDirection,
                            tasm::CSSValue(lepus::Value("lynx-rtl")));
  EXPECT_FALSE(element->CanBeLayoutOnly());
}

TEST_F(RadonNodeTest, TestViewCanBeLayoutOnlyWithOptimization) {
  page_proxy->element_manager()->config_->SetEnableExtendedLayoutOpt(true);
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node->CreateElementIfNeeded();
  auto* element = radon_node->element();
  element->overflow_ = FiberElement::OVERFLOW_XY;
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(element->is_view());
  EXPECT_TRUE(element->CanBeLayoutOnly());
  // With enable_extended_layout_only_opt_, "text-align,direction" shall not
  // make the layout only optimization invalid
  element->SetStyleInternal(CSSPropertyID::kPropertyIDDirection,
                            tasm::CSSValue(lepus::Value("lynx-rtl")));
  element->SetStyleInternal(CSSPropertyID::kPropertyIDTextAlign,
                            tasm::CSSValue(lepus::Value("center")));
  // view can be layout only by default.
  EXPECT_TRUE(element->CanBeLayoutOnly());
  // Other style will make layout only false.
  element->SetStyleInternal(CSSPropertyID::kPropertyIDOpacity,
                            tasm::CSSValue(lepus::Value(0.2)));
  EXPECT_FALSE(element->CanBeLayoutOnly());
}

TEST_F(RadonNodeTest, TestComponentCanBeLayoutOnly) {
  auto radon_node =
      std::make_unique<RadonComponent>(page_proxy.get(), 0, nullptr, nullptr,
                                       nullptr, nullptr, 123, "component");
  radon_node->SetComponent(nullptr);
  auto element = radon_node->CreateFiberElement();
  element->overflow_ = FiberElement::OVERFLOW_XY;
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_component());
  EXPECT_FALSE(static_cast<FiberElement*>(element.get())->is_wrapper());
  // Can not be layout only because enable_component_layout_only_ switch
  EXPECT_FALSE(
      static_cast<ComponentElement*>(element.get())->CanBeLayoutOnly());
}

TEST_F(RadonNodeTest,
       TestComponentCanBeLayoutOnlyWithEnableComponentLayoutOnly) {
  page_proxy->element_manager()->config_->SetEnableComponentLayoutOnly(true);
  auto radon_node =
      std::make_unique<RadonComponent>(page_proxy.get(), 0, nullptr, nullptr,
                                       nullptr, nullptr, 123, "component");
  radon_node->SetComponent(nullptr);
  auto element = radon_node->CreateFiberElement();
  element->overflow_ = FiberElement::OVERFLOW_XY;
  EXPECT_TRUE(element->is_fiber_element());
  EXPECT_TRUE(static_cast<FiberElement*>(element.get())->is_component());
  EXPECT_FALSE(static_cast<FiberElement*>(element.get())->is_wrapper());
  // Can be layout only because enable_component_layout_only_ switch
  EXPECT_TRUE(static_cast<ComponentElement*>(element.get())->CanBeLayoutOnly());
}

TEST_F(RadonNodeTest, SetInlineStyleForFiber) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node->SetInlineStyle(
      CSSPropertyID::kPropertyIDBackgroundColor, base::String("black"),
      page_proxy->element_manager()->GetCSSParserConfigs());
  EXPECT_FALSE(radon_node->raw_inline_styles().empty());
  EXPECT_TRUE(radon_node->raw_inline_styles().contains(
      CSSPropertyID::kPropertyIDBackgroundColor));
  auto val = radon_node->raw_inline_styles().find(
      CSSPropertyID::kPropertyIDBackgroundColor);
  EXPECT_TRUE(val->second == lepus::Value("black"));
}

TEST_F(RadonNodeTest, FlushInlineStyleForFiber) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node->CreateElementIfNeeded();
  EXPECT_TRUE(radon_node->element()->is_fiber_element());
  const auto* fiber_element = static_cast<FiberElement*>(radon_node->element());
  radon_node->SetInlineStyle(
      CSSPropertyID::kPropertyIDBackgroundColor, base::String("black"),
      page_proxy->element_manager()->GetCSSParserConfigs());
  EXPECT_FALSE(radon_node->raw_inline_styles().empty());
  EXPECT_TRUE(radon_node->raw_inline_styles().contains(
      CSSPropertyID::kPropertyIDBackgroundColor));
  auto val = radon_node->raw_inline_styles().find(
      CSSPropertyID::kPropertyIDBackgroundColor);
  EXPECT_TRUE(val->second == lepus::Value("black"));
  const auto& fiber_inline_styles = fiber_element->current_raw_inline_styles_;
  EXPECT_TRUE(fiber_inline_styles.empty());
  radon_node->DispatchFirstTime();
  EXPECT_FALSE(fiber_inline_styles.empty());
  EXPECT_TRUE(
      fiber_inline_styles.contains(CSSPropertyID::kPropertyIDBackgroundColor));
  val = fiber_inline_styles.find(CSSPropertyID::kPropertyIDBackgroundColor);
  EXPECT_TRUE(val->second == lepus::Value("black"));
}

TEST_F(RadonNodeTest, FlushAttributeForFiber) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node->CreateElementIfNeeded();
  EXPECT_TRUE(radon_node->element()->is_fiber_element());
  const auto* fiber_element = static_cast<FiberElement*>(radon_node->element());
  radon_node->SetStaticAttribute("123", lepus::Value("black"));
  EXPECT_FALSE(radon_node->attributes().empty());
  EXPECT_TRUE(radon_node->attributes().count("123"));
  const auto& fiber_attribute = fiber_element->updated_attr_map_;
  EXPECT_TRUE(fiber_attribute.empty());
  radon_node->DispatchFirstTime();
  EXPECT_FALSE(fiber_attribute.empty());
  EXPECT_TRUE(fiber_attribute.count("123"));
  auto val = fiber_attribute.find("123");
  EXPECT_TRUE(val->second == lepus::Value("black"));
}

TEST_F(RadonNodeTest, DiffAttributeForFiber) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node1 = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node1->CreateElementIfNeeded();
  auto radon_node2 = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node2->CreateElementIfNeeded();

  radon_node1->SetStaticAttribute("123", lepus::Value("black"));
  const auto* fiber_element =
      static_cast<FiberElement*>(radon_node1->element());
  const auto& fiber_attribute = fiber_element->updated_attr_map_;
  EXPECT_TRUE(fiber_attribute.empty());
  radon_node1->ShouldFlushAttr(radon_node2.get());
  EXPECT_FALSE(fiber_attribute.empty());
  EXPECT_TRUE(fiber_attribute.count("123"));
  auto val = fiber_attribute.find("123");
  EXPECT_TRUE(val->second == lepus::Value("black"));
}

TEST_F(RadonNodeTest, DiffAttributeEmptyForFiber) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node1 = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node1->CreateElementIfNeeded();
  auto radon_node2 = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node2->CreateElementIfNeeded();

  radon_node1->SetStaticAttribute("123", lepus::Value("black"));
  const auto* fiber_element =
      static_cast<FiberElement*>(radon_node2->element());
  const auto& fiber_attribute = fiber_element->reset_attr_vec_;
  EXPECT_TRUE(fiber_attribute.empty());
  radon_node2->ShouldFlushAttr(radon_node1.get());
  EXPECT_FALSE(fiber_attribute.empty());
  EXPECT_TRUE(fiber_attribute[0] == "123");
}

TEST_F(RadonNodeTest, DiffClassesForFiber) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node1 = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node1->CreateElementIfNeeded();
  auto radon_node2 = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node2->CreateElementIfNeeded();
  radon_node1->AddClass("class1");
  radon_node2->AddClass("class1");
  DispatchOption option(page_proxy.get());
  radon_node1->class_dirty_ = radon_node1->classes() != radon_node2->classes();
  EXPECT_FALSE(radon_node1->ShouldFlushStyle(radon_node2.get(), option));
  radon_node1->AddClass("class2");
  radon_node1->class_dirty_ = radon_node1->classes() != radon_node2->classes();
  EXPECT_TRUE(radon_node1->ShouldFlushStyle(radon_node2.get(), option));
}

TEST_F(RadonNodeTest, DiffStylesForFiber) {
  page_proxy->element_manager()->SetEnableFiberElementForRadonDiff(
      TernaryBool::TRUE_VALUE);
  auto radon_node1 = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node1->CreateElementIfNeeded();
  auto radon_node2 = std::make_unique<RadonNode>(page_proxy.get(), "view", 123);
  radon_node2->CreateElementIfNeeded();
  radon_node1->SetInlineStyle(
      CSSPropertyID::kPropertyIDBackgroundColor, base::String("black"),
      page_proxy->element_manager()->GetCSSParserConfigs());
  radon_node2->SetInlineStyle(
      CSSPropertyID::kPropertyIDBackgroundColor, base::String("red"),
      page_proxy->element_manager()->GetCSSParserConfigs());
  DispatchOption option(page_proxy.get());
  const auto* fiber_element =
      static_cast<FiberElement*>(radon_node1->element());
  const auto& fiber_inline_styles = fiber_element->current_raw_inline_styles_;
  EXPECT_TRUE(fiber_inline_styles.empty());
  EXPECT_TRUE(radon_node1->ShouldFlushStyle(radon_node2.get(), option));

  EXPECT_FALSE(fiber_inline_styles.empty());
  EXPECT_TRUE(
      fiber_inline_styles.contains(CSSPropertyID::kPropertyIDBackgroundColor));
  auto val =
      fiber_inline_styles.find(CSSPropertyID::kPropertyIDBackgroundColor);
  EXPECT_TRUE(val->second == lepus::Value("black"));
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
