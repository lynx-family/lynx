// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#define private public
#include "core/renderer/dom/air/air_element/air_element.h"

#include "core/renderer/dom/air/air_element/air_component_element.h"
#include "core/renderer/dom/air/air_element/air_page_element.h"
#undef private

#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_value.h"
#include "core/renderer/dom/air/air_element/air_component_element.h"
#include "core/renderer/dom/air/air_element/air_for_element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/shell/testing/mock_tasm_delegate.h"
#include "core/template_bundle/template_codec/compile_options.h"
#include "third_party/googletest/googlemock/include/gmock/gmock.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace testing {

static constexpr int32_t kWidth = 1080;
static constexpr int32_t kHeight = 1920;
static constexpr float kDefaultLayoutsUnitPerPx = 1.f;
static constexpr double kDefaultPhysicalPixelsPerLayoutUnit = 1.f;

class AirElementTest : public ::testing::Test {
 public:
  AirElementTest() {}
  ~AirElementTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  ::testing::NiceMock<test::MockTasmDelegate> tasm_mediator;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), &tasm_mediator,
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    manager->SetConfig(config);
  }

  fml::RefPtr<AirLepusRef> CreateAirNode(const base::String &tag,
                                         int32_t lepus_id) {
    std::shared_ptr<AirElement> element =
        std::make_shared<AirElement>(kAirNormal, manager.get(), tag, lepus_id);
    manager->air_node_manager()->Record(element->impl_id(), element);
    manager->air_node_manager()->RecordForLepusId(
        lepus_id, static_cast<uint64_t>(lepus_id),
        AirLepusRef::Create(element));
    return AirLepusRef::Create(element);
  }

  fml::RefPtr<AirLepusRef> CreateAirNodeWithoutRecordLepus(
      const base::String &tag, int32_t lepus_id) {
    std::shared_ptr<AirElement> element =
        std::make_shared<AirElement>(kAirNormal, manager.get(), tag, lepus_id);
    manager->air_node_manager()->Record(element->impl_id(), element);
    return AirLepusRef::Create(element);
  }

  fml::RefPtr<AirLepusRef> CreateAirPage(int32_t lepus_id) {
    std::shared_ptr<AirPageElement> page =
        std::make_shared<AirPageElement>(manager.get(), lepus_id);
    manager->air_node_manager()->Record(page->impl_id(), page);
    manager->air_node_manager()->RecordForLepusId(
        lepus_id, static_cast<uint64_t>(lepus_id), AirLepusRef::Create(page));
    return AirLepusRef::Create(page);
  }

  fml::RefPtr<AirLepusRef> CreateAirComponent(int32_t lepus_id) {
    std::shared_ptr<AirComponentElement> element =
        std::make_shared<AirComponentElement>(manager.get(), 0, lepus_id, -1,
                                              nullptr);
    manager->air_node_manager()->Record(element->impl_id(), element);
    manager->air_node_manager()->RecordForLepusId(
        lepus_id, static_cast<uint64_t>(lepus_id),
        AirLepusRef::Create(element));
    return AirLepusRef::Create(element);
  }

  fml::RefPtr<AirLepusRef> CreateAirFor(int32_t lepus_id) {
    std::shared_ptr<AirForElement> for_element =
        std::make_shared<AirForElement>(manager.get(), lepus_id);
    manager->air_node_manager()->Record(for_element->impl_id(), for_element);
    manager->air_node_manager()->RecordForLepusId(
        lepus_id, static_cast<uint64_t>(lepus_id),
        AirLepusRef::Create(for_element));
    return AirLepusRef::Create(for_element);
  }

  lepus::Value CreateStyle(int32_t pattern, int32_t value) {
    lepus::Value result((lepus::Dictionary::Create()));
    result.SetProperty("pattern", lepus::Value(pattern));
    result.SetProperty("value", lepus::Value(value));
    return result;
  }

  lepus::Value CreateStyle(int32_t pattern, const std::string &value) {
    lepus::Value result((lepus::Dictionary::Create()));
    result.SetProperty("pattern", lepus::Value(pattern));
    result.SetProperty("value", lepus::Value(value));
    return result;
  }

  StyleMap ConvertStyle(const lepus::Value &origin_css_style,
                        const CSSParserConfigs &raw_css_parser_config) {
    // Convert from LepusValue to CSSValue, the format of LepusValue is as
    // follows
    //
    // .selector_name {
    //    css_property_id:{pattern:parsed_pattern_value , value:
    //    parsed_css_value} not_parsed_css_name:{raw:not_parsed_css_value};
    // }
    //
    // CSS properties that are not parsed at compile time will be parsed at
    // runtime using UnitHandler
    static constexpr const char *kCSSValue = "value";
    static constexpr const char *kCSSPattern = "pattern";
    static constexpr const char *kCSSRaw = "raw";
    StyleMap result;
    ForEachLepusValue(origin_css_style, [&result, &raw_css_parser_config](
                                            const lepus::Value &css_id_str,
                                            const lepus::Value &css_struct) {
      if (css_struct.IsObject()) {
        int key_id = std::stoi(css_id_str.StdString());
        bool is_parsed = key_id != -1;
        // CSS properties that have been parsed during compilation
        // css_property_id:{pattern:parsed_pattern_value , value:
        // parsed_css_value}
        if (is_parsed) {
          CSSPropertyID id = static_cast<CSSPropertyID>(key_id);
          lepus::Value css_parsed_value = css_struct.GetProperty(kCSSValue);
          CSSValue css_value;
          css_value.SetValue(css_parsed_value);
          css_value.SetPattern(static_cast<CSSValuePattern>(
              css_struct.GetProperty(kCSSPattern).Number()));
          result[id] = css_value;
        } else {
          // CSS properties that have not been parsed during compilation
          // not_parsed_css_name:{raw:not_parsed_css_value};
          UnitHandler::Process(CSSProperty::GetPropertyID(css_id_str.String()),
                               css_struct.GetProperty(kCSSRaw), result,
                               raw_css_parser_config);
        }
      }
    });
    return result;
  }
};

TEST_F(AirElementTest, UpdateComponentInLepus) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 0;
  AirPageElement *page =
      static_cast<AirPageElement *>(CreateAirPage(lepus_id++)->Get());
  AirComponentElement *component =
      static_cast<AirComponentElement *>(CreateAirComponent(lepus_id++)->Get());
  page->InsertNode(component);
  lepus::Value data((lepus::Dictionary::Create()));
  data.SetProperty("testString", lepus::Value("testValue"));
  data.SetProperty("testNumber", lepus::Value(1));
  component->data_ = lepus::Value::Clone(data, false);
  lepus::Value update_string((lepus::Dictionary::Create()));
  update_string.SetProperty("testString", lepus::Value("updateValue"));
  EXPECT_TRUE(component->UpdateComponentInLepus(update_string));

  lepus::Value update_number((lepus::Dictionary::Create()));
  update_number.SetProperty("testNumber", lepus::Value(2));
  EXPECT_TRUE(component->UpdateComponentInLepus(update_number));
}

TEST_F(AirElementTest, InsertNode) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  auto element = CreateAirNode("view", lepus_id++)->Get();
  tasm::CSSValue css_value(lepus::Value("visible"));
  element->SetStyle(CSSPropertyID::kPropertyIDOverflow, css_value);
  parent->InsertNode(element);

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(parent->GetChildAt(0), element);
}

TEST_F(AirElementTest, SetStyle) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);

  auto element = CreateAirNode("view", 1)->Get();
  tasm::CSSValue css_value_visible(lepus::Value("visible"));
  EXPECT_CALL(
      tasm_mediator,
      UpdateLayoutNodeStyle(element->impl_id(),
                            CSSPropertyID::kPropertyIDOverflow, ::testing::_))
      .Times(0);
  element->SetStyle(CSSPropertyID::kPropertyIDOverflow, css_value_visible);

  tasm::CSSValue css_value_width(lepus::Value(10), CSSValuePattern::NUMBER);
  if (!element->EnableAsyncCalc()) {
    EXPECT_CALL(
        tasm_mediator,
        UpdateLayoutNodeStyle(element->impl_id(),
                              CSSPropertyID::kPropertyIDWidth, ::testing::_))
        .Times(1);
  }
  element->SetStyle(CSSPropertyID::kPropertyIDWidth, css_value_width);
}

TEST_F(AirElementTest, InsertNodeBefore) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  auto element = CreateAirNode("view", lepus_id++)->Get();
  tasm::CSSValue style(lepus::Value("visible"),
                       lynx::tasm::CSSValuePattern::STRING);
  element->SetStyle(CSSPropertyID::kPropertyIDOverflow, style);
  parent->InsertNode(element);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  auto second_element = CreateAirNode("view", lepus_id++)->Get();

  parent->InsertNodeBefore(second_element, element);
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 2);

  EXPECT_EQ(parent->GetChildAt(0), second_element);
  EXPECT_EQ(parent->GetChildAt(1), element);
}

TEST_F(AirElementTest, DestroyPlatformNode) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  uint32_t lepus_id = 1;
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  parent->FlushProps();
  auto element = CreateAirNode("view", lepus_id++)->Get();
  parent->InsertNode(element);
  parent->FlushRecursively();

  parent->Destroy();
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  // parent element destructor will cause child's platform node destroy
  // recursively
  auto parent_new = CreateAirNode("view", lepus_id++)->Get();
  auto element_new = CreateAirNode("view", lepus_id++)->Get();
  parent_new->InsertNode(element_new);
  parent_new = nullptr;
  EXPECT_FALSE(element_new->HasElementContainer());
}

TEST_F(AirElementTest, RemoveNodeAndFlush) {
  uint32_t lepus_id = 1;
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);

  AirElement *page = CreateAirNode("view", lepus_id++)->Get();
  auto parent = CreateAirNode("view", lepus_id++)->Get();
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 0);

  // insert parent to page
  page->InsertNode(parent);
  EXPECT_CALL(tasm_mediator, InsertLayoutNode(page->impl_id(),
                                              parent->impl_id(), ::testing::_));

  // insert first_element to parent
  auto first_element = CreateAirNode("view", lepus_id++)->Get();
  tasm::CSSValue style(lepus::Value("visible"),
                       lynx::tasm::CSSValuePattern::STRING);
  first_element->SetStyle(CSSPropertyID::kPropertyIDOverflow, style);
  parent->InsertNode(first_element);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNode(parent->impl_id(), first_element->impl_id(),
                               ::testing::_));

  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);

  // insert second element before first element to parent
  auto second_element = CreateAirNode("view", lepus_id++)->Get();

  parent->InsertNodeBefore(second_element, first_element);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNode(parent->impl_id(), second_element->impl_id(),
                               ::testing::_));
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 2);

  EXPECT_EQ(parent->GetChildAt(0), second_element);
  EXPECT_EQ(parent->GetChildAt(1), first_element);

  page->FlushRecursively();

  // remove the second element
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(parent->impl_id(), second_element->impl_id()));
  parent->RemoveNode(second_element);
  // Take care: here do not modify Element tree , pending to Remove it in
  // FlushAction!!
  EXPECT_EQ(static_cast<int>(parent->GetChildCount()), 1);
  EXPECT_EQ(parent->GetChildAt(0), first_element);

  page->FlushRecursively();
}

TEST_F(AirElementTest, CreateAndRemoveElement) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;

  AirElement *page = CreateAirNode("view", lepus_id++)->Get();
  auto element0 = CreateAirNode("view", lepus_id++)->Get();
  auto element_before_black = CreateAirNode("view", lepus_id++)->Get();
  auto element = CreateAirNode("view", lepus_id++)->Get();
  auto text = CreateAirNode("text", lepus_id++)->Get();

  page->InsertNode(element0);
  EXPECT_CALL(
      tasm_mediator,
      InsertLayoutNode(page->impl_id(), element0->impl_id(), ::testing::_));
  page->InsertNode(element_before_black);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNode(page->impl_id(), element_before_black->impl_id(),
                               ::testing::_));
  page->InsertNode(element);
  EXPECT_CALL(
      tasm_mediator,
      InsertLayoutNode(page->impl_id(), element->impl_id(), ::testing::_));
  page->InsertNode(text);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNode(page->impl_id(), text->impl_id(), ::testing::_));
  page->FlushRecursively();

  // Append after wrapper
  auto element_after_yellow = CreateAirNode("view", lepus_id++)->Get();
  page->InsertNode(element_after_yellow);
  EXPECT_CALL(tasm_mediator,
              InsertLayoutNode(page->impl_id(), element_after_yellow->impl_id(),
                               ::testing::_));
  page->FlushRecursively();

  // remove Wrapper's sibling
  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(),
                                              element_before_black->impl_id()));
  page->RemoveNode(element_before_black);
  EXPECT_CALL(tasm_mediator, RemoveLayoutNode(page->impl_id(),
                                              element_after_yellow->impl_id()));
  page->RemoveNode(element_after_yellow);
  page->FlushRecursively();

  // Insert node
  auto text1 = CreateAirNode("text", lepus_id++)->Get();
  page->InsertNode(text1);
  EXPECT_CALL(tasm_mediator, InsertLayoutNode(page->impl_id(), text1->impl_id(),
                                              ::testing::_));
  page->FlushRecursively();

  // Remove node
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(page->impl_id(), text->impl_id()));
  page->RemoveNode(text);
  page->FlushRecursively();
}

TEST_F(AirElementTest, AddAndRemoveElement) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 0;

  AirElement *parent = CreateAirNode("view", lepus_id++)->Get();
  AirElement *child1 = CreateAirNode("view", lepus_id++)->Get();
  AirElement *child2 = CreateAirNode("view", lepus_id++)->Get();
  parent->InsertNode(child1);
  EXPECT_CALL(tasm_mediator, InsertLayoutNode(parent->impl_id(),
                                              child1->impl_id(), ::testing::_));
  parent->InsertNode(child2);
  EXPECT_CALL(tasm_mediator, InsertLayoutNode(parent->impl_id(),
                                              child2->impl_id(), ::testing::_));
  parent->FlushRecursively();

  EXPECT_EQ(parent->GetChildCount(), 2);

  AirElement *child_new = CreateAirNode("view", lepus_id++)->Get();
  parent->InsertNodeBefore(child_new, child1);
  EXPECT_CALL(
      tasm_mediator,
      InsertLayoutNode(parent->impl_id(), child_new->impl_id(), ::testing::_));
  EXPECT_CALL(tasm_mediator,
              RemoveLayoutNode(parent->impl_id(), child1->impl_id()));
  parent->RemoveNode(child1);
  parent->FlushRecursively();

  EXPECT_EQ(parent->GetChildCount(), 2);
  EXPECT_EQ(parent->GetChildAt(0), child_new);
  EXPECT_EQ(parent->GetChildAt(1), child2);
}

TEST_F(AirElementTest, StyleGet) {
  std::unordered_map<std::string, std::shared_ptr<StyleMap>> parsed_styles;
  lepus::Value styles(lepus::Dictionary::Create());
  // class1 style
  lepus::Value class1_style(lepus::Dictionary::Create());
  class1_style.SetProperty("3", CreateStyle(2, 2));
  parsed_styles[".class1"] = std::make_shared<StyleMap>(
      ConvertStyle(class1_style, manager->GetCSSParserConfigs()));
  // class2 style
  lepus::Value class2_style(lepus::Dictionary::Create());
  class2_style.SetProperty("3", CreateStyle(2, 3));
  class2_style.SetProperty("4", CreateStyle(1, 2));
  parsed_styles[".class2"] = std::make_shared<StyleMap>(
      ConvertStyle(class2_style, manager->GetCSSParserConfigs()));
  // id style
  lepus::Value id1_style(lepus::Dictionary::Create());
  id1_style.SetProperty("4", CreateStyle(1, 2));
  parsed_styles["#id1"] = std::make_shared<StyleMap>(
      ConvertStyle(id1_style, manager->GetCSSParserConfigs()));
  // tag style
  lepus::Value tag_style(lepus::Dictionary::Create());
  tag_style.SetProperty("5", CreateStyle(1, 2));
  parsed_styles["view"] = std::make_shared<StyleMap>(
      ConvertStyle(tag_style, manager->GetCSSParserConfigs()));
  // global style
  lepus::Value global_style(lepus::Dictionary::Create());
  global_style.SetProperty("4", CreateStyle(1, 2));
  parsed_styles["*"] = std::make_shared<StyleMap>(
      ConvertStyle(global_style, manager->GetCSSParserConfigs()));
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 1;

  AirPageElement *page =
      static_cast<AirPageElement *>(CreateAirPage(lepus_id++)->Get());

  page->SetParsedStyles(parsed_styles);
  auto element0 = CreateAirNode("view", lepus_id++)->Get();
  page->InsertNode(element0);
  StyleMap result_map;
  element0->GetStableStyleMap("", result_map);
  EXPECT_TRUE(result_map.contains(CSSPropertyID::kPropertyIDBottom));

  result_map.clear();
  element0->GetStableStyleMap("view", result_map);
  EXPECT_TRUE(result_map.contains(CSSPropertyID::kPropertyIDPosition));

  result_map.clear();
  element0->GetIdStyleMap("id1", result_map);
  EXPECT_TRUE(result_map.contains(CSSPropertyID::kPropertyIDBottom));

  result_map.clear();
  element0->GetClassStyleMap({"class1", "class2"}, result_map);
  EXPECT_TRUE(result_map.contains(CSSPropertyID::kPropertyIDBottom));
  EXPECT_TRUE(result_map.contains(CSSPropertyID::kPropertyIDRight));
  EXPECT_EQ(result_map[CSSPropertyID::kPropertyIDRight].GetPattern(),
            CSSValuePattern::NUMBER);
  EXPECT_EQ(result_map[CSSPropertyID::kPropertyIDRight].GetValue(),
            lepus::Value(3));
  EXPECT_EQ(result_map.size(), 2);
}

TEST_F(AirElementTest, RefreshStylesAndUpdatePatch) {
  std::unordered_map<std::string, std::shared_ptr<StyleMap>> parsed_styles;
  int lepus_id = 0;
  AirPageElement *page =
      static_cast<AirPageElement *>(CreateAirPage(lepus_id++)->Get());
  AirElement *view = CreateAirNode("view", lepus_id++)->Get();
  page->InsertNode(view, false);
  EXPECT_TRUE(view->parent() == page);

  lepus::Value global_style(lepus::Dictionary::Create());
  global_style.SetProperty("1", CreateStyle(2, 4));

  lepus::Value tag_style(lepus::Dictionary::Create());
  tag_style.SetProperty("1", CreateStyle(3, 4));
  tag_style.SetProperty("2", CreateStyle(3, 4));

  lepus::Value class1_style(lepus::Dictionary::Create());
  class1_style.SetProperty("3", CreateStyle(1, 2));

  lepus::Value class2_style(lepus::Dictionary::Create());
  class2_style.SetProperty("4", CreateStyle(1, 2));

  lepus::Value class3_style(lepus::Dictionary::Create());
  class3_style.SetProperty("3", CreateStyle(1, 2));
  class3_style.SetProperty("5", CreateStyle(1, 2));

  lepus::Value id1_style(lepus::Dictionary::Create());
  id1_style.SetProperty("1", CreateStyle(1, 2));
  id1_style.SetProperty("6", CreateStyle(1, 2));
  id1_style.SetProperty("7", CreateStyle(1, 2));

  lepus::Value id2_style(lepus::Dictionary::Create());
  id2_style.SetProperty("3", CreateStyle(3, 4));

  parsed_styles["*"] = std::make_shared<StyleMap>(
      ConvertStyle(global_style, manager->GetCSSParserConfigs()));
  parsed_styles["view"] = std::make_shared<StyleMap>(
      ConvertStyle(tag_style, manager->GetCSSParserConfigs()));
  parsed_styles[".class1"] = std::make_shared<StyleMap>(
      ConvertStyle(class1_style, manager->GetCSSParserConfigs()));
  parsed_styles[".class2"] = std::make_shared<StyleMap>(
      ConvertStyle(class2_style, manager->GetCSSParserConfigs()));
  parsed_styles[".class3"] = std::make_shared<StyleMap>(
      ConvertStyle(class3_style, manager->GetCSSParserConfigs()));
  parsed_styles["#id1"] = std::make_shared<StyleMap>(
      ConvertStyle(id1_style, manager->GetCSSParserConfigs()));
  parsed_styles["#id2"] = std::make_shared<StyleMap>(
      ConvertStyle(id2_style, manager->GetCSSParserConfigs()));
  page->SetParsedStyles(parsed_styles);

  view->SetClasses(lepus::Value("class1 class2"));
  view->SetIdSelector(lepus::Value("id1"));
  view->SetInlineStyle(static_cast<CSSPropertyID>(6),
                       CSSValue(lepus::Value(2)));

  view->RefreshStyles();
  view->has_painting_node_ = true;
  EXPECT_EQ(view->cur_css_styles_.size(), 4);

  view->SetClasses(lepus::Value("class3"));
  view->SetIdSelector(lepus::Value("id2"));
  EXPECT_EQ(view->cur_css_styles_.size(), 4);

  AirElement::StylePatch style_patch;
  view->UpdateStylePatch(AirElement::Selector::kSTABLE, style_patch);
  view->UpdateStylePatch(AirElement::Selector::kCLASS, style_patch);
  view->UpdateStylePatch(AirElement::Selector::kID, style_patch);
  view->UpdateStylePatch(AirElement::Selector::kINLINE, style_patch);

  EXPECT_TRUE(
      style_patch.update_styles_map_.contains(static_cast<CSSPropertyID>(1)));
  EXPECT_TRUE(
      style_patch.reserve_styles_map_.contains(static_cast<CSSPropertyID>(2)));
  EXPECT_TRUE(
      style_patch.update_styles_map_.contains(static_cast<CSSPropertyID>(3)));
  EXPECT_TRUE(style_patch.reset_id_set_.find(static_cast<CSSPropertyID>(4)) !=
              style_patch.reset_id_set_.end());
  EXPECT_TRUE(
      style_patch.update_styles_map_.contains(static_cast<CSSPropertyID>(5)));
  EXPECT_TRUE(
      style_patch.update_styles_map_.contains(static_cast<CSSPropertyID>(6)));
  EXPECT_TRUE(style_patch.reset_id_set_.find(static_cast<CSSPropertyID>(7)) !=
              style_patch.reset_id_set_.end());
}

TEST_F(AirElementTest, DiffStyles) {
  AirElement *page = CreateAirNode("view", 1)->Get();

  StyleMap old_map;
  StyleMap new_map;
  AirElement::StylePatch final_patch;

  old_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                           CSSValue(lepus::Value(1)));
  old_map.insert_or_assign(CSSPropertyID::kPropertyIDHeight,
                           CSSValue(lepus::Value(2)));
  new_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                           CSSValue(lepus::Value(3)));

  page->DiffStyles(old_map, new_map, final_patch);

  // width
  EXPECT_TRUE(
      final_patch.update_styles_map_.contains(CSSPropertyID::kPropertyIDWidth));
  EXPECT_FALSE(final_patch.reserve_styles_map_.contains(
      CSSPropertyID::kPropertyIDWidth));
  EXPECT_FALSE(
      final_patch.reset_id_set_.find(CSSPropertyID::kPropertyIDWidth) !=
      final_patch.reset_id_set_.end());
  // height
  EXPECT_TRUE(
      final_patch.reset_id_set_.find(CSSPropertyID::kPropertyIDHeight) !=
      final_patch.reset_id_set_.end());
  EXPECT_FALSE(final_patch.update_styles_map_.contains(
      CSSPropertyID::kPropertyIDHeight));
  EXPECT_FALSE(final_patch.reserve_styles_map_.contains(
      CSSPropertyID::kPropertyIDHeight));

  old_map.clear();
  new_map.clear();
  final_patch.reset_id_set_.clear();
  final_patch.update_styles_map_.clear();

  old_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                           CSSValue(lepus::Value(1)));
  old_map.insert_or_assign(CSSPropertyID::kPropertyIDHeight,
                           CSSValue(lepus::Value(2)));
  new_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                           CSSValue(lepus::Value(3)));
  final_patch.reserve_styles_map_.insert_or_assign(
      CSSPropertyID::kPropertyIDWidth, CSSValue(lepus::Value(1)));
  final_patch.reserve_styles_map_.insert_or_assign(
      CSSPropertyID::kPropertyIDHeight, CSSValue(lepus::Value(2)));
  final_patch.reserve_styles_map_.insert_or_assign(
      CSSPropertyID::kPropertyStart, CSSValue(lepus::Value(2)));
  final_patch.update_styles_map_.insert_or_assign(CSSPropertyID::kPropertyEnd,
                                                  CSSValue(lepus::Value(2)));

  page->DiffStyles(old_map, new_map, final_patch, true);

  // width
  EXPECT_TRUE(
      final_patch.update_styles_map_.contains(CSSPropertyID::kPropertyIDWidth));
  EXPECT_FALSE(final_patch.reserve_styles_map_.contains(
      CSSPropertyID::kPropertyIDWidth));
  EXPECT_FALSE(
      final_patch.reset_id_set_.find(CSSPropertyID::kPropertyIDWidth) !=
      final_patch.reset_id_set_.end());
  // height
  EXPECT_TRUE(
      final_patch.reset_id_set_.find(CSSPropertyID::kPropertyIDHeight) ==
      final_patch.reset_id_set_.end());
  EXPECT_TRUE(final_patch.update_styles_map_.contains(
      CSSPropertyID::kPropertyIDHeight));
  EXPECT_FALSE(final_patch.reserve_styles_map_.contains(
      CSSPropertyID::kPropertyIDHeight));
  EXPECT_TRUE(
      final_patch.reserve_styles_map_.contains(CSSPropertyID::kPropertyStart));
  EXPECT_TRUE(
      final_patch.update_styles_map_.contains(CSSPropertyID::kPropertyEnd));

  old_map.clear();
  new_map.clear();
  final_patch.reset_id_set_.clear();
  final_patch.update_styles_map_.clear();
  final_patch.reserve_styles_map_.clear();

  final_patch.update_styles_map_.insert_or_assign(
      CSSPropertyID::kPropertyIDWidth, CSSValue(lepus::Value(2)));
  old_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                           CSSValue(lepus::Value(1)));
  new_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                           CSSValue(lepus::Value(1)));
  page->DiffStyles(old_map, new_map, final_patch, true);
  EXPECT_TRUE(final_patch.reserve_styles_map_.contains(
      CSSPropertyID::kPropertyIDWidth));
  EXPECT_FALSE(
      final_patch.update_styles_map_.contains(CSSPropertyID::kPropertyIDWidth));
  EXPECT_FALSE(
      final_patch.reset_id_set_.find(CSSPropertyID::kPropertyIDWidth) !=
      final_patch.reset_id_set_.end());

  old_map.clear();
  new_map.clear();
  final_patch.reset_id_set_.clear();
  final_patch.update_styles_map_.clear();
  final_patch.reserve_styles_map_.clear();

  final_patch.update_styles_map_.insert_or_assign(
      CSSPropertyID::kPropertyIDWidth, CSSValue(lepus::Value(2)));
  final_patch.update_styles_map_.insert_or_assign(
      CSSPropertyID::kPropertyIDHeight, CSSValue(lepus::Value(2)));
  final_patch.reserve_styles_map_.insert_or_assign(
      CSSPropertyID::kPropertyStart, CSSValue(lepus::Value(2)));

  old_map.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                           CSSValue(lepus::Value(2)));
  page->DiffStyles(old_map, new_map, final_patch, true);
  EXPECT_TRUE(final_patch.update_styles_map_.contains(
      CSSPropertyID::kPropertyIDHeight));
  EXPECT_EQ(final_patch.update_styles_map_.size(), 1);
  EXPECT_TRUE(final_patch.reserve_styles_map_.contains(
      CSSPropertyID::kPropertyIDWidth));
  EXPECT_EQ(final_patch.reserve_styles_map_.size(), 2);
}

TEST_F(AirElementTest, ResolveKeyframesMap) {
  AirElement *page = CreateAirNode("view", 1)->Get();
  page->SetClasses(lepus::Value("class1 class2"));
  page->SetInlineStyle(
      "animation: translateX-ani 2s ease 0s infinite alternate both running;",
      true);
  //   std::unordered_map<std::string, std::shared_ptr<StyleMap>> keyframes_map;
  lepus::Value keyframes_map(lepus::Dictionary::Create());
  lepus::Value keyframe_map(lepus::Dictionary::Create());
  lepus::Value transform0(lepus::Dictionary::Create());
  lepus::Value transform1(lepus::Dictionary::Create());
  transform0.SetProperty("transform", lepus::Value("translateX(0)"));
  transform1.SetProperty("transform", lepus::Value("translateX(50px)"));
  keyframe_map.SetProperty("0", transform0);
  keyframe_map.SetProperty("1", transform1);
  keyframes_map.SetProperty("translateX-ani", keyframe_map);
  EXPECT_TRUE(
      page->ResolveKeyframesMap(kPropertyIDAnimationName, keyframes_map));
}

TEST_F(AirElementTest, SetClasses) {
  AirElement *page = CreateAirNode("view", 1)->Get();
  page->SetClasses(lepus::Value("class1 class2"));
  EXPECT_TRUE(page->style_dirty_ & AirElement::Selector::kCLASS);
  EXPECT_EQ(page->classes_.size(), 2);
  EXPECT_EQ(page->classes_[0], "class1");
  EXPECT_EQ(page->classes_[1], "class2");
  page->style_dirty_ = 0;

  page->SetClasses(lepus::Value("class1"));
  EXPECT_TRUE(page->style_dirty_ & AirElement::Selector::kCLASS);
  EXPECT_EQ(page->classes_.size(), 1);
  EXPECT_EQ(page->classes_[0], "class1");
  page->style_dirty_ = 0;

  page->SetClasses(lepus::Value("class1 class2"));
  EXPECT_TRUE(page->style_dirty_ & AirElement::Selector::kCLASS);
  EXPECT_EQ(page->classes_.size(), 2);
  EXPECT_EQ(page->classes_[0], "class1");
  EXPECT_EQ(page->classes_[1], "class2");
  page->style_dirty_ = 0;

  page->SetClasses(lepus::Value("class1 class4"));
  EXPECT_TRUE(page->style_dirty_ & AirElement::Selector::kCLASS);
  EXPECT_EQ(page->classes_.size(), 2);
  EXPECT_EQ(page->classes_[0], "class1");
  EXPECT_EQ(page->classes_[1], "class4");
  page->style_dirty_ = 0;

  page->SetClasses(lepus::Value("class1 class2 class3"));
  EXPECT_TRUE(page->style_dirty_ & AirElement::Selector::kCLASS);
  EXPECT_EQ(page->classes_.size(), 3);
  EXPECT_EQ(page->classes_[0], "class1");
  EXPECT_EQ(page->classes_[1], "class2");
  EXPECT_EQ(page->classes_[2], "class3");
  page->style_dirty_ = 0;
}

TEST_F(AirElementTest, ComponentGetElement) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  int32_t lepus_id = 0;
  AirPageElement *page =
      static_cast<AirPageElement *>(CreateAirPage(lepus_id++)->Get());
  AirElement *parent = CreateAirNode("view", lepus_id++)->Get();
  AirComponentElement *first_comp =
      static_cast<AirComponentElement *>(CreateAirComponent(lepus_id++)->Get());
  AirComponentElement *second_comp =
      static_cast<AirComponentElement *>(CreateAirComponent(lepus_id++)->Get());
  parent->InsertNode(first_comp);
  parent->InsertNode(second_comp);
  int32_t target_id = lepus_id;
  page->PushComponentElement(first_comp);
  uint64_t key = page->GetKeyForCreatedElement(target_id);
  EXPECT_EQ(key, static_cast<uint64_t>(first_comp->impl_id()));
  AirElement *view1 = CreateAirNodeWithoutRecordLepus("view", target_id)->Get();
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(manager->air_node_manager()->Get(view1->impl_id())));
  EXPECT_EQ(view1, manager->GetAirNode("view", target_id)->Get());
  page->PopComponentElement();
  page->PushComponentElement(second_comp);
  key = page->GetKeyForCreatedElement(target_id);
  EXPECT_EQ(key, static_cast<uint64_t>(second_comp->impl_id()));
  AirElement *view2 = CreateAirNodeWithoutRecordLepus("view", target_id)->Get();
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(manager->air_node_manager()->Get(view2->impl_id())));
  EXPECT_EQ(view2, manager->GetAirNode("view", target_id)->Get());
}

TEST_F(AirElementTest, ForNodeGetElement) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  int32_t lepus_id = 0;
  AirPageElement *page =
      static_cast<AirPageElement *>(CreateAirPage(lepus_id++)->Get());
  AirElement *parent = CreateAirNode("view", lepus_id++)->Get();
  page->InsertNode(parent);
  AirForElement *for_node =
      static_cast<AirForElement *>(CreateAirFor(lepus_id++)->Get());
  parent->InsertNode(for_node);
  page->PushForElement(for_node);
  // The for node has three child view, every child view has a grandchild view
  // which is controlled by tt:if. In the first screen, only the first and third
  // child view show its grandchild view, and in the update phase, all three
  // grandchild view are shown.
  AirElement *view1 = CreateAirNode("view", lepus_id++)->Get();
  AirElement *view2 = CreateAirNode("view", lepus_id++)->Get();
  AirElement *view3 = CreateAirNode("view", lepus_id++)->Get();
  for_node->InsertNode(view1);
  for_node->InsertNode(view2);
  for_node->InsertNode(view3);
  int32_t target_id = lepus_id;
  for_node->UpdateActiveIndex(0);
  uint64_t key = page->GetKeyForCreatedElement(target_id);
  AirElement *childitem1 =
      CreateAirNodeWithoutRecordLepus("view", target_id)->Get();
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(
          manager->air_node_manager()->Get(childitem1->impl_id())));
  AirElement *childitem3 =
      CreateAirNodeWithoutRecordLepus("view", target_id)->Get();
  for_node->UpdateActiveIndex(2);
  key = page->GetKeyForCreatedElement(target_id);
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(
          manager->air_node_manager()->Get(childitem3->impl_id())));
  view1->InsertNode(childitem1);
  view3->InsertNode(childitem3);
  for_node->UpdateActiveIndex(0);
  EXPECT_EQ(childitem1, manager->GetAirNode("view", target_id)->Get());
  for_node->UpdateActiveIndex(1);
  EXPECT_EQ(nullptr, manager->GetAirNode("view", target_id).get());
  for_node->UpdateActiveIndex(2);
  EXPECT_EQ(childitem3, manager->GetAirNode("view", target_id)->Get());

  AirElement *childitem2 =
      CreateAirNodeWithoutRecordLepus("view", target_id)->Get();
  for_node->UpdateActiveIndex(1);
  key = page->GetKeyForCreatedElement(target_id);
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(
          manager->air_node_manager()->Get(childitem2->impl_id())));
  view2->InsertNode(childitem2);
  for_node->UpdateActiveIndex(0);
  EXPECT_EQ(childitem1, manager->GetAirNode("view", target_id)->Get());
  for_node->UpdateActiveIndex(1);
  EXPECT_EQ(childitem2, manager->GetAirNode("view", target_id)->Get());
  for_node->UpdateActiveIndex(2);
  EXPECT_EQ(childitem3, manager->GetAirNode("view", target_id)->Get());
}

TEST_F(AirElementTest, UpdateInlineStyleAndFlush) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);

  auto parent = CreateAirNode("view", 1)->Get();
  auto child = CreateAirNode("view", 2)->Get();
  parent->InsertNode(child);
  tasm::CSSValue css_value_border_width(lepus::Value("2px"));
  child->SetInlineStyle(CSSPropertyID::kPropertyIDBorderTopWidth,
                        css_value_border_width);
  tasm::CSSValue css_value_border_color(lepus::Value("#fe002b"));
  child->SetInlineStyle(CSSPropertyID::kPropertyIDBorderTopColor,
                        css_value_border_color);
  parent->FlushRecursively();

  tasm::CSSValue css_value_update_border_width(lepus::Value("2px"));
  child->SetInlineStyle(CSSPropertyID::kPropertyIDBorderTopWidth,
                        css_value_update_border_width);
  AirElement::StylePatch style_patch;
  child->UpdateStylePatch(AirElement::Selector::kINLINE, style_patch);
  EXPECT_EQ(style_patch.reset_id_set_.size(), 1);
  EXPECT_EQ(style_patch.reserve_styles_map_.size(), 1);
  EXPECT_EQ(child->style_dirty_, AirElement::Selector::kINLINE);
  parent->FlushRecursively();
  EXPECT_EQ(child->style_dirty_, 0);
}

TEST_F(AirElementTest, CheckFlattenStylesAndAttributes) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);

  auto parent = CreateAirNode("view", 1)->Get();
  auto child_1 = CreateAirNode("view", 2)->Get();
  auto child_2 = CreateAirNode("view", 3)->Get();
  auto child_3 = CreateAirNode("view", 4)->Get();
  auto child_4 = CreateAirNode("view", 5)->Get();
  auto child_5 = CreateAirNode("view", 6)->Get();
  auto child_6 = CreateAirNode("view", 7)->Get();

  parent->InsertNode(child_1);
  parent->InsertNode(child_2);
  parent->InsertNode(child_3);
  parent->InsertNode(child_4);
  parent->InsertNode(child_5);
  parent->InsertNode(child_6);

  tasm::CSSValue animation(lepus::Value("translateShow 0.3s linear forwards"));
  child_1->SetStyle(CSSPropertyID::kPropertyIDAnimation, animation);
  EXPECT_TRUE(child_1->has_animate_props_);

  child_2->CheckHasNonFlattenAttr("clip-radius", lepus::Value("10px"));
  EXPECT_TRUE(child_2->has_non_flatten_attrs_);

  child_3->CheckHasNonFlattenAttr("name", lepus::Value(""));
  EXPECT_TRUE(child_3->has_non_flatten_attrs_);

  child_4->CheckHasNonFlattenCSSProps(CSSPropertyID::kPropertyIDTransition);
  EXPECT_TRUE(child_4->has_non_flatten_attrs_);

  child_5->CheckHasNonFlattenCSSProps(CSSPropertyID::kPropertyIDOpacity);
  EXPECT_TRUE(child_4->has_non_flatten_attrs_);

  child_6->CheckHasNonFlattenCSSProps(CSSPropertyID::kPropertyIDFilter);
  EXPECT_TRUE(child_4->has_non_flatten_attrs_);
}

TEST_F(AirElementTest, ForNodeUpdate) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  int32_t lepus_id = 0;
  AirPageElement *page =
      static_cast<AirPageElement *>(CreateAirPage(lepus_id++)->Get());
  AirElement *parent = CreateAirNode("view", lepus_id++)->Get();
  page->InsertNode(parent);
  AirForElement *for_node =
      static_cast<AirForElement *>(CreateAirFor(lepus_id++)->Get());
  parent->InsertNode(for_node);
  page->PushForElement(for_node);
  // The for node has three child view, every child view has a grandchild view
  // which is controlled by tt:if. In the first screen, only the first and third
  // child view show its grandchild view, and in the update phase, all three
  // grandchild view are shown.
  AirElement *view1 = CreateAirNode("view", lepus_id++)->Get();
  AirElement *view2 = CreateAirNode("view", lepus_id++)->Get();
  AirElement *view3 = CreateAirNode("view", lepus_id++)->Get();
  for_node->InsertNode(view1);
  for_node->InsertNode(view2);
  for_node->InsertNode(view3);
  int32_t target_id = lepus_id;
  for_node->UpdateActiveIndex(0);
  uint64_t key = page->GetKeyForCreatedElement(target_id);
  AirElement *childitem1 =
      CreateAirNodeWithoutRecordLepus("view", target_id)->Get();
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(
          manager->air_node_manager()->Get(childitem1->impl_id())));
  AirElement *childitem2 =
      CreateAirNodeWithoutRecordLepus("view", target_id)->Get();
  for_node->UpdateActiveIndex(1);
  key = page->GetKeyForCreatedElement(target_id);
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(
          manager->air_node_manager()->Get(childitem2->impl_id())));
  AirElement *childitem3 =
      CreateAirNodeWithoutRecordLepus("view", target_id)->Get();
  for_node->UpdateActiveIndex(2);
  key = page->GetKeyForCreatedElement(target_id);
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(
          manager->air_node_manager()->Get(childitem3->impl_id())));
  AirElement *childitem4 =
      CreateAirNodeWithoutRecordLepus("view", target_id)->Get();
  for_node->UpdateActiveIndex(3);
  key = page->GetKeyForCreatedElement(target_id);
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(
          manager->air_node_manager()->Get(childitem4->impl_id())));

  view1->InsertNode(childitem1);
  view2->InsertNode(childitem2);
  view3->InsertNode(childitem3);
  view3->InsertNode(childitem4);
  for_node->UpdateActiveIndex(0);
  EXPECT_EQ(childitem1, manager->GetAirNode("view", target_id)->Get());
  for_node->UpdateActiveIndex(1);
  EXPECT_EQ(childitem2, manager->GetAirNode("view", target_id)->Get());
  for_node->UpdateActiveIndex(2);
  EXPECT_EQ(childitem3, manager->GetAirNode("view", target_id)->Get());

  AirElement *childitem5 =
      CreateAirNodeWithoutRecordLepus("view", target_id + 1)->Get();
  for_node->UpdateActiveIndex(1);
  key = page->GetKeyForCreatedElement(target_id);
  manager->air_node_manager()->RecordForLepusId(
      target_id, key,
      AirLepusRef::Create(
          manager->air_node_manager()->Get(childitem5->impl_id())));
  view2->InsertNode(childitem5);
  for_node->UpdateActiveIndex(0);
  EXPECT_EQ(childitem1, manager->GetAirNode("view", target_id)->Get());
  for_node->UpdateActiveIndex(1);
  EXPECT_EQ(childitem2, manager->GetAirNode("view", target_id)->Get());
  for_node->UpdateActiveIndex(2);
  EXPECT_EQ(childitem3, manager->GetAirNode("view", target_id)->Get());
}

TEST_F(AirElementTest, GetDataAndGetPageDataByKey) {
  auto config = std::make_shared<PageConfig>();
  config->SetLynxAirMode(CompileOptionAirMode::AIR_MODE_STRICT);
  manager->SetConfig(config);
  uint32_t lepus_id = 0;
  AirPageElement *page =
      static_cast<AirPageElement *>(CreateAirPage(lepus_id++)->Get());
  lepus::Value data((lepus::Dictionary::Create()));
  data.SetProperty("testString", lepus::Value("testValue"));
  data.SetProperty("testNumber", lepus::Value(1));
  page->data_ = data;

  lepus::Value page_data = page->GetData();
  ASSERT_TRUE(page_data.GetProperty("testString").StringView() == "testValue");
  ASSERT_TRUE(page_data.GetProperty("testNumber").Number() == 1);

  std::vector<std::string> keysVec1;
  keysVec1.emplace_back("testString");
  lepus::Value testStr = page->GetPageDataByKey(keysVec1);
  ASSERT_TRUE(testStr.GetProperty("testString").StringView() == "testValue");

  std::vector<std::string> keysVec2;
  keysVec2.emplace_back("testNumber");
  lepus::Value testNum = page->GetPageDataByKey(keysVec2);
  ASSERT_TRUE(testNum.GetProperty("testNumber").Number() == 1);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
