// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/dom/element.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/css/css_keyframes_token.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/tasm/react/testing/mock_painting_context.h"
#include "core/runtime/js/bindings/java_script_element.h"
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

class ElementTest : public ::testing::Test {
 public:
  ElementTest() {}
  ~ElementTest() override {}
  std::unique_ptr<lynx::tasm::ElementManager> manager;
  std::shared_ptr<::testing::NiceMock<test::MockTasmDelegate>> tasm_mediator;

  void SetUp() override {
    LynxEnvConfig lynx_env_config(kWidth, kHeight, kDefaultLayoutsUnitPerPx,
                                  kDefaultPhysicalPixelsPerLayoutUnit);
    tasm_mediator = std::make_shared<
        ::testing::NiceMock<lynx::tasm::test::MockTasmDelegate>>();
    manager = std::make_unique<lynx::tasm::ElementManager>(
        std::make_unique<MockPaintingContext>(), tasm_mediator.get(),
        lynx_env_config);
    auto config = std::make_shared<PageConfig>();
    config->SetEnableZIndex(true);
    manager->SetConfig(config);
  }
};

TEST_F(ElementTest, CheckHasFilterProps) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableComponentLayoutOnly(true);
  config->SetDefaultOverflowVisible(true);
  config->SetEnableZIndex(true);
  manager->SetConfig(config);
  auto comp = std::shared_ptr<RadonComponent>(
      new RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());

  bool ret =
      element->CheckTransitionProps(CSSPropertyID::kPropertyIDTransition);
  EXPECT_TRUE(element->has_transition_props_changed_);
  EXPECT_TRUE(ret);

  ret = element->CheckKeyframeProps(CSSPropertyID::kPropertyIDAnimation);
  EXPECT_TRUE(element->has_keyframe_props_changed_);
  EXPECT_TRUE(ret);

  element->computed_css_style()->SetEnableZIndex(true);

  CSSParserConfigs configs;
  lepus::Value z_index_raw_value = lepus::Value("1");
  auto z_index =
      UnitHandler::Process(kPropertyIDZIndex, z_index_raw_value, configs);
  element->computed_css_style()->SetValue(kPropertyIDZIndex,
                                          z_index[kPropertyIDZIndex]);
  EXPECT_TRUE(element->has_z_props());
  EXPECT_TRUE(ret);

  element->computed_css_style()->SetValue(
      CSSPropertyID::kPropertyIDOpacity,
      CSSValue(0.1, CSSValuePattern::NUMBER));
  EXPECT_TRUE(element->computed_css_style()->HasOpacity());

  element->CheckHasNonFlattenCSSProps(CSSPropertyID::kPropertyIDFilter);
  EXPECT_TRUE(element->has_non_flatten_attrs_);
}

TEST_F(ElementTest, CheckWillDestroy) {
  auto config = std::make_shared<PageConfig>();
  config->SetEnableFiberArch(true);
  manager->SetConfig(config);

  auto page = manager->CreateFiberPage("11", 20);
  manager->SetRoot(page.get());
  manager->SetRootOnLayout(page->impl_id());
  page->FlushProps();

  auto element = manager->CreateFiberNode("view");
  element->SetStyleInternal(CSSPropertyID::kPropertyIDOverflow,
                            tasm::CSSValue::MakePlainString("visible"));
  element->FlushProps();

  EXPECT_FALSE(element->will_destroy());

  // force delete node manager
  manager = nullptr;
  EXPECT_TRUE(element->will_destroy());
}

TEST_F(ElementTest, GetCSSKeyframesToken) {
  auto element0 = manager->CreateFiberElement("view");
  EXPECT_TRUE(element0->GetCSSKeyframesToken("test") == nullptr);
  auto comp = std::shared_ptr<RadonComponent>(
      new RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element1 = manager->CreateFiberElement("view");
  element1->SetAttributeHolder(comp.get()->attribute_holder());
  EXPECT_TRUE(element1->GetCSSKeyframesToken("test") == nullptr);
}

TEST_F(ElementTest, ResolveCSSKeyframesByNames) {
  tasm::CSSParserConfigs configs;
  starlight::ComputedCSSStyle computedCssStyle(
      kDefaultLayoutsUnitPerPx, kDefaultPhysicalPixelsPerLayoutUnit);
  tasm::CssMeasureContext length_context = computedCssStyle.GetMeasureContext();
  tasm::CSSKeyframesTokenMap keyframes;

  const char* c_name0 = "a";
  const char* c_name1 = "b";
  const char* c_name2 = "c";
  const char* c_name3 = "1";
  lepus_value name = lepus::Value(c_name0);
  auto array = lepus::CArray::Create();
  array->push_back(lepus_value(c_name1));
  array->push_back(lepus_value(c_name2));
  array->push_back(lepus_value(1));
  lepus_value names = lepus::Value(array);

  lepus_value result;
  auto test_element = manager->CreateFiberElement("view");
  result = test_element->ResolveCSSKeyframesByNames(
      name, keyframes, length_context, configs, false);
  EXPECT_TRUE(result.IsTable() && result.Table()->size() == 0);
  result = test_element->ResolveCSSKeyframesByNames(
      names, keyframes, length_context, configs, false);
  EXPECT_TRUE(result.IsTable() && result.Table()->size() == 0);

  fml::RefPtr<tasm::CSSKeyframesToken> token0 =
      fml::AdoptRef(new tasm::CSSKeyframesToken(configs));
  fml::RefPtr<tasm::CSSKeyframesToken> token1 =
      fml::AdoptRef(new tasm::CSSKeyframesToken(configs));
  fml::RefPtr<tasm::CSSKeyframesToken> token2 =
      fml::AdoptRef(new tasm::CSSKeyframesToken(configs));
  fml::RefPtr<tasm::CSSKeyframesToken> token3 =
      fml::AdoptRef(new tasm::CSSKeyframesToken(configs));
  keyframes.insert({c_name0, token0});
  keyframes.insert({c_name1, token1});
  keyframes.insert({c_name2, token2});
  keyframes.insert({c_name3, token3});
  result = test_element->ResolveCSSKeyframesByNames(
      name, keyframes, length_context, configs, false);
  EXPECT_TRUE(result.IsTable() && result.Table()->size() == 1 &&
              result.Table()->Contains(c_name0));

  result = test_element->ResolveCSSKeyframesByNames(
      name, keyframes, length_context, configs, false);
  EXPECT_TRUE(result.IsTable() && result.Table()->size() == 0);

  result = test_element->ResolveCSSKeyframesByNames(
      names, keyframes, length_context, configs, false);
  EXPECT_TRUE(result.IsTable() && result.Table()->size() == 2 &&
              result.Table()->Contains(c_name1) &&
              result.Table()->Contains(c_name2));

  result = test_element->ResolveCSSKeyframesByNames(
      names, keyframes, length_context, configs, false);
  EXPECT_TRUE(result.IsTable() && result.Table()->size() == 0);
}

TEST_F(ElementTest, Animate_Array) {
  auto element = manager->CreateFiberElement("view");

  auto array1 = lepus::CArray::Create();
  array1->set(0, lepus_value(0));

  auto array2 = lepus::CArray::Create();
  auto table1 = lepus::Dictionary::Create();
  table1->SetValue("left", lepus_value("10px"));
  lepus::Value test_keyframe_table_1{table1};
  array2->set(0, test_keyframe_table_1);
  auto table2 = lepus::Dictionary::Create();
  table2->SetValue("left", lepus_value("100px"));
  lepus::Value test_keyframe_table_2{table2};
  array2->set(1, test_keyframe_table_2);
  lepus::Value test_keyframes_array{array2};
  array1->set(2, test_keyframes_array);

  auto table3 = lepus::Dictionary::Create();
  table3->SetValue("name", lepus::Value("name1"));
  table3->SetValue("duration", lepus::Value(2000));
  table3->SetValue("iteration", lepus::Value(2));
  table3->SetValue("fill", lepus::Value("forwards"));
  table3->SetValue("play-state", lepus::Value("running"));
  lepus::Value test_data_table{table3};
  array1->set(3, test_data_table);

  // 1.Check that the keyframe array was passed in correctly.
  lepus::Value test_animate_args{array1};
  auto pipeline_option = std::make_shared<PipelineOptions>();
  element->Animate(test_animate_args, pipeline_option);
  auto iter = element->keyframes_map_->find("name1");
  EXPECT_EQ(iter != element->keyframes_map_->end(), true);
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0)
                ->second->find(kPropertyIDLeft)
                ->second.GetValue(),
            lepus::Value(10));
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0)
                ->second->find(kPropertyIDLeft)
                ->second.GetPattern(),
            CSSValuePattern::PX);
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(1)
                ->second->find(kPropertyIDLeft)
                ->second.GetValue(),
            lepus::Value(100));
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(1)
                ->second->find(kPropertyIDLeft)
                ->second.GetPattern(),
            CSSValuePattern::PX);
}

TEST_F(ElementTest, Animate_Table) {
  auto element = manager->CreateFiberElement("view");

  auto array1 = lepus::CArray::Create();
  array1->set(0, lepus_value(0));

  auto table1 = lepus::Dictionary::Create();
  auto table2 = lepus::Dictionary::Create();
  table2->SetValue("left", lepus_value("10px"));
  lepus::Value test_keyframe_table_1(std::move(table2));
  table1->SetValue("0%", test_keyframe_table_1);
  auto table3 = lepus::Dictionary::Create();
  table3->SetValue("left", lepus_value("55px"));
  lepus::Value test_keyframe_table_2(std::move(table3));
  table1->SetValue("50%", test_keyframe_table_2);
  auto table4 = lepus::Dictionary::Create();
  table4->SetValue("left", lepus_value("100px"));
  lepus::Value test_keyframe_table_3(std::move(table4));
  table1->SetValue("100%", test_keyframe_table_3);
  lepus::Value test_keyframes_table(table1);
  array1->set(2, test_keyframes_table);

  auto table5 = lepus::Dictionary::Create();
  table5->SetValue("name", lepus::Value("name1"));
  table5->SetValue("duration", lepus::Value(2000));
  table5->SetValue("iteration", lepus::Value(2));
  table5->SetValue("fill", lepus::Value("forwards"));
  table5->SetValue("play-state", lepus::Value("running"));
  lepus::Value test_data_table(std::move(table5));
  array1->set(3, test_data_table);

  // 1.Check that the keyframe table was passed in correctly.
  lepus::Value test_animate_args{array1};
  auto pipeline_option3 = std::make_shared<PipelineOptions>();
  element->Animate(test_animate_args, pipeline_option3);
  auto iter = element->keyframes_map_->find("name1");
  EXPECT_EQ(iter != element->keyframes_map_->end(), true);
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0)
                ->second->find(kPropertyIDLeft)
                ->second.GetValue(),
            lepus::Value(10));
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0)
                ->second->find(kPropertyIDLeft)
                ->second.GetPattern(),
            CSSValuePattern::PX);
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0.5)
                ->second->find(kPropertyIDLeft)
                ->second.GetValue(),
            lepus::Value(55));
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0.5)
                ->second->find(kPropertyIDLeft)
                ->second.GetPattern(),
            CSSValuePattern::PX);
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(1)
                ->second->find(kPropertyIDLeft)
                ->second.GetValue(),
            lepus::Value(100));
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(1)
                ->second->find(kPropertyIDLeft)
                ->second.GetPattern(),
            CSSValuePattern::PX);
}

TEST_F(ElementTest, Animate_v2_Table) {
  auto element = manager->CreateFiberElement("view");
  element->enable_new_animator_ = true;
  auto array1 = lepus::CArray::Create();
  array1->set(0, lepus_value(0));

  auto table1 = lepus::Dictionary::Create();
  auto table2 = lepus::Dictionary::Create();
  table2->SetValue("left", lepus_value("10px"));
  lepus::Value test_keyframe_table_1(std::move(table2));
  table1->SetValue("0%", test_keyframe_table_1);
  auto table3 = lepus::Dictionary::Create();
  table3->SetValue("left", lepus_value("55px"));
  lepus::Value test_keyframe_table_2(std::move(table3));
  table1->SetValue("50%", test_keyframe_table_2);
  auto table4 = lepus::Dictionary::Create();
  table4->SetValue("left", lepus_value("100px"));
  lepus::Value test_keyframe_table_3(std::move(table4));
  table1->SetValue("100%", test_keyframe_table_3);
  lepus::Value test_keyframes_table(table1);
  array1->set(2, test_keyframes_table);

  auto table5 = lepus::Dictionary::Create();
  table5->SetValue("name", lepus::Value("name1"));
  table5->SetValue("duration", lepus::Value(2000));
  table5->SetValue("iteration", lepus::Value(2));
  table5->SetValue("fill", lepus::Value("forwards"));
  table5->SetValue("play-state", lepus::Value("running"));
  lepus::Value test_data_table(std::move(table5));
  array1->set(3, test_data_table);

  // 1.Check that the keyframe table was passed in correctly.
  lepus::Value test_animate_args{array1};
  auto pipeline_option = std::make_shared<PipelineOptions>();
  element->AnimateV2(test_animate_args, pipeline_option);
  auto iter = element->keyframes_map_->find("name1");
  EXPECT_EQ(iter != element->keyframes_map_->end(), true);
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0)
                ->second->find(kPropertyIDLeft)
                ->second.GetValue(),
            lepus::Value(10));
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0)
                ->second->find(kPropertyIDLeft)
                ->second.GetPattern(),
            CSSValuePattern::PX);
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0.5)
                ->second->find(kPropertyIDLeft)
                ->second.GetValue(),
            lepus::Value(55));
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(0.5)
                ->second->find(kPropertyIDLeft)
                ->second.GetPattern(),
            CSSValuePattern::PX);
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(1)
                ->second->find(kPropertyIDLeft)
                ->second.GetValue(),
            lepus::Value(100));
  EXPECT_EQ(iter->second->GetKeyframesContent()
                .find(1)
                ->second->find(kPropertyIDLeft)
                ->second.GetPattern(),
            CSSValuePattern::PX);

  // 2.Check that the animation_data were passed in correctly.
  auto target_name = "name1";
  auto animation_data = element->computed_css_style()->animation_data_;
  auto ani_iter =
      std::find_if(animation_data->begin(), animation_data->end(),
                   [&target_name](const starlight::AnimationData& data) {
                     return data.name == target_name;
                   });
  EXPECT_EQ(ani_iter != animation_data->end(), true);
  EXPECT_EQ(ani_iter->duration, 2000);
  EXPECT_EQ(static_cast<int>(ani_iter->fill_mode), 1);
  EXPECT_EQ(static_cast<int>(ani_iter->play_state), 1);
}

TEST_F(ElementTest, LayoutResult) {
  auto element = manager->CreateFiberElement("view");

  const float width = 200.0f;
  const float height = 150.0f;
  const float left = 50.0f;
  const float top = 30.0f;

  element->width_ = width;
  element->height_ = height;
  element->left_ = left;
  element->top_ = top;

  element->paddings_[0] = 10.0f;  // left
  element->paddings_[1] = 20.0f;  // top
  element->paddings_[2] = 15.0f;  // right
  element->paddings_[3] = 25.0f;  // bottom

  element->margins_[0] = 5.0f;  // left
  element->margins_[1] = 8.0f;  // top
  element->margins_[2] = 6.0f;  // right
  element->margins_[3] = 7.0f;  // bottom

  element->borders_[0] = 1.0f;  // left
  element->borders_[1] = 2.0f;  // top
  element->borders_[2] = 3.0f;  // right
  element->borders_[3] = 4.0f;  // bottom

  auto result = element->layout_result();

  EXPECT_EQ(result.size_.width_, width);
  EXPECT_EQ(result.size_.height_, height);

  EXPECT_EQ(result.offset_.x_, left);
  EXPECT_EQ(result.offset_.y_, top);

  EXPECT_EQ(result.padding_[starlight::Direction::kLeft], 10.0f);
  EXPECT_EQ(result.padding_[starlight::Direction::kRight], 15.0f);
  EXPECT_EQ(result.padding_[starlight::Direction::kTop], 20.0f);
  EXPECT_EQ(result.padding_[starlight::Direction::kBottom], 25.0f);

  EXPECT_EQ(result.margin_[starlight::Direction::kLeft], 5.0f);
  EXPECT_EQ(result.margin_[starlight::Direction::kRight], 6.0f);
  EXPECT_EQ(result.margin_[starlight::Direction::kTop], 8.0f);
  EXPECT_EQ(result.margin_[starlight::Direction::kBottom], 7.0f);

  EXPECT_EQ(result.border_[starlight::Direction::kLeft], 1.0f);
  EXPECT_EQ(result.border_[starlight::Direction::kRight], 3.0f);
  EXPECT_EQ(result.border_[starlight::Direction::kTop], 2.0f);
  EXPECT_EQ(result.border_[starlight::Direction::kBottom], 4.0f);
}

TEST_F(ElementTest, GetCSSKeyframesToken_FromAdoptedStyleSheet) {
  // Verify that GetCSSKeyframesToken can find keyframes from adopted
  // stylesheets when the element has no related CSS fragment.
  CSSParserConfigs configs;
  auto keyframe_token = fml::AdoptRef(new CSSKeyframesToken(configs));
  CSSKeyframesTokenMap keyframes_map;
  keyframes_map.insert({base::String("fade-in"), keyframe_token});

  auto adopted_fragment = std::make_unique<SharedCSSFragment>(
      -1, std::vector<int32_t>{}, CSSParserTokenMap{}, std::move(keyframes_map),
      CSSFontFaceRuleMap{});

  auto wrapper =
      fml::AdoptRef(new SharedCSSFragmentWrapper(std::move(adopted_fragment)));
  manager->AdoptStyleSheet(wrapper);

  auto element = manager->CreateFiberElement("view");
  CSSKeyframesToken* result =
      element->GetCSSKeyframesToken(base::String("fade-in"));
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(result, keyframe_token.get());

  // Non-existent keyframes should return nullptr.
  EXPECT_EQ(element->GetCSSKeyframesToken(base::String("slide-in")), nullptr);
}

TEST_F(ElementTest, GetCSSKeyframesToken_AdoptedPrioritizedOverRegular) {
  // Verify that adopted stylesheets have higher priority than the element's
  // related CSS fragment for keyframes lookup.
  CSSParserConfigs configs;

  // Create adopted keyframe token.
  auto adopted_token = fml::AdoptRef(new CSSKeyframesToken(configs));
  CSSKeyframesTokenMap adopted_keyframes;
  adopted_keyframes.insert({base::String("bounce"), adopted_token});

  auto adopted_fragment = std::make_unique<SharedCSSFragment>(
      -1, std::vector<int32_t>{}, CSSParserTokenMap{},
      std::move(adopted_keyframes), CSSFontFaceRuleMap{});
  auto wrapper =
      fml::AdoptRef(new SharedCSSFragmentWrapper(std::move(adopted_fragment)));
  manager->AdoptStyleSheet(wrapper);

  // Create a regular fragment with a keyframe of the same name.
  auto regular_token = fml::AdoptRef(new CSSKeyframesToken(configs));
  CSSKeyframesTokenMap regular_keyframes;
  regular_keyframes.insert({base::String("bounce"), regular_token});

  auto comp = std::shared_ptr<RadonComponent>(
      new RadonComponent(nullptr, 0, nullptr, nullptr, 0, 0, 0));
  auto element = manager->CreateFiberElement("view");
  element->SetAttributeHolder(comp.get()->attribute_holder());

  // The adopted keyframe should be returned, not the regular one.
  CSSKeyframesToken* result =
      element->GetCSSKeyframesToken(base::String("bounce"));
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(result, adopted_token.get());
}

TEST_F(ElementTest, GetCSSKeyframesToken_MultipleAdoptedSheets_ReverseLookup) {
  // Verify that later adopted stylesheets have higher priority (reverse
  // iteration order).
  CSSParserConfigs configs;

  auto token_first = fml::AdoptRef(new CSSKeyframesToken(configs));
  CSSKeyframesTokenMap keyframes1;
  keyframes1.insert({base::String("spin"), token_first});
  auto fragment1 = std::make_unique<SharedCSSFragment>(
      -1, std::vector<int32_t>{}, CSSParserTokenMap{}, std::move(keyframes1),
      CSSFontFaceRuleMap{});
  auto wrapper1 =
      fml::AdoptRef(new SharedCSSFragmentWrapper(std::move(fragment1)));

  auto token_second = fml::AdoptRef(new CSSKeyframesToken(configs));
  CSSKeyframesTokenMap keyframes2;
  keyframes2.insert({base::String("spin"), token_second});
  auto fragment2 = std::make_unique<SharedCSSFragment>(
      -1, std::vector<int32_t>{}, CSSParserTokenMap{}, std::move(keyframes2),
      CSSFontFaceRuleMap{});
  auto wrapper2 =
      fml::AdoptRef(new SharedCSSFragmentWrapper(std::move(fragment2)));

  // Adopt in order: wrapper1 first, wrapper2 second.
  manager->AdoptStyleSheet(wrapper1);
  manager->AdoptStyleSheet(wrapper2);

  auto element = manager->CreateFiberElement("view");
  CSSKeyframesToken* result =
      element->GetCSSKeyframesToken(base::String("spin"));
  // The last adopted sheet (wrapper2) should be found first (reverse
  // iteration).
  EXPECT_EQ(result, token_second.get());
}

TEST_F(ElementTest, ResolveStyleValueKeepsInheritedResolvedValue) {
  auto element = manager->CreateFiberElement("view");
  const auto value = CSSValue(18, CSSValuePattern::NUMBER);

  element->ResolveStyleValue(CSSPropertyID::kPropertyIDFontSize, value);

  const auto& resolved_values =
      element->computed_css_style()->GetResolvedValues();
  auto it = resolved_values.find(CSSPropertyID::kPropertyIDFontSize);
  ASSERT_TRUE(it != resolved_values.end());
  EXPECT_EQ(it->second, value);
}

}  // namespace testing
}  // namespace tasm
}  // namespace lynx
