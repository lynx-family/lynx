// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/public/tasm_codec.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/css/css_parser_token.h"
#include "core/renderer/css/ng/parser/media_query_parser.h"
#include "core/renderer/css/ng/style/condition_rule.h"
#include "core/renderer/css/ng/supports/supports_condition.h"
#include "core/renderer/css/shared_css_fragment.h"
#include "core/renderer/template_entry.h"
#include "core/template_bundle/lynx_template_bundle.h"
#include "core/template_bundle/lynx_template_bundle_converter.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_lazy_reader_delegate.h"

namespace {

void TestEncodeSuccess() {
  std::cout << "TestEncodeSuccess..." << std::endl;
  std::string valid_json = R"({"source":"<div>test</div>","config":{}})";
  auto res = lynx::tasm::codec::Encode(valid_json);
  assert(res.status == 0);
  assert(res.buffer.size() > 0);
  std::cout << "  PASSED" << std::endl;
}

void TestEncodeInvalidJson() {
  std::cout << "TestEncodeInvalidJson..." << std::endl;
  std::string invalid_json = "{invalid json}";
  auto res = lynx::tasm::codec::Encode(invalid_json);
  assert(res.status != 0);
  assert(!res.error_msg.empty());
  std::cout << "  PASSED" << std::endl;
}

void TestDecodeInvalidPointer() {
  std::cout << "TestDecodeInvalidPointer..." << std::endl;
  auto res = lynx::tasm::codec::Decode(nullptr, 0);
  assert(res.status != 0);
  assert(res.error_msg == "Invalid Buffer!");
  std::cout << "  PASSED" << std::endl;
}

void TestDecodeInvalidData() {
  std::cout << "TestDecodeInvalidData..." << std::endl;
  std::vector<uint8_t> invalid_data = {0x00, 0x01, 0x02};
  auto res =
      lynx::tasm::codec::Decode(invalid_data.data(), invalid_data.size());
  assert(res.status != 0);
  assert(!res.error_msg.empty());
  std::cout << "  PASSED" << std::endl;
}

void TestEncodeDecodeRoundTrip() {
  std::cout << "TestEncodeDecodeRoundTrip..." << std::endl;
  std::string valid_json =
      R"({"source":"<div>roundtrip test</div>","config":{}})";
  auto encode_res = lynx::tasm::codec::Encode(valid_json);
  assert(encode_res.status == 0);
  assert(encode_res.buffer.size() > 0);

  auto decode_res = lynx::tasm::codec::Decode(encode_res.buffer.data(),
                                              encode_res.buffer.size());
  assert(decode_res.status == 0);
  assert(!decode_res.result.empty());
  std::cout << "  PASSED (encoded=" << encode_res.buffer.size()
            << " bytes, decoded result length=" << decode_res.result.size()
            << ")" << std::endl;
}

void TestConvertWithNullPageConfig() {
  std::cout << "TestConvertWithNullPageConfig..." << std::endl;
  lynx::tasm::LynxTemplateBundle bundle;
  // Default-constructed bundle has page_configs_ == nullptr
  assert(bundle.GetPageConfig() == nullptr);
  auto result = lynx::tasm::LynxTemplateBundleConverter::
      ConvertTemplateBundleToSerializedString(bundle);
  assert(!result.empty());
  assert(result.find("\"page-config\":null") != std::string::npos);
  std::cout << "  PASSED" << std::endl;
}

void TestConvertSerializesLegacyCSS() {
  std::cout << "TestConvertSerializesLegacyCSS..." << std::endl;
  lynx::tasm::LynxTemplateBundle bundle;
  lynx::tasm::CSSParserTokenMap css_map;
  auto token = fml::MakeRefCounted<lynx::tasm::CSSParseToken>(
      lynx::tasm::CSSParserConfigs());
  lynx::tasm::StyleMap styles;
  styles.emplace(
      lynx::tasm::kPropertyIDColor,
      lynx::tasm::CSSValue("red", lynx::tasm::CSSValuePattern::STRING,
                           lynx::tasm::CSSValueType::DEFAULT));
  token->SetAttributes(std::move(styles));
  css_map.emplace(".foo", std::move(token));

  bundle.GetCSSStyleManager()->AddSharedCSSFragment(
      std::make_unique<lynx::tasm::SharedCSSFragment>(
          0, std::vector<int32_t>(), std::move(css_map),
          lynx::tasm::CSSKeyframesTokenMap(),
          lynx::tasm::CSSFontFaceRuleMap()));

  const auto result = lynx::tasm::LynxTemplateBundleConverter::
      ConvertTemplateBundleToSerializedString(bundle);
  assert(result.find("\"css\":{\"text\":\".foo { color: red; }\"}") !=
         std::string::npos);
  std::cout << "  PASSED" << std::endl;
}

void TestConvertSerializesCSSRule() {
  std::cout << "TestConvertSerializesCSSRule..." << std::endl;
  lynx::tasm::LynxTemplateBundle bundle;
  auto fragment = std::make_unique<lynx::tasm::SharedCSSFragment>(0);
  fragment->SetEnableCSSSelector();

  auto selectors = std::make_unique<lynx::css::LynxCSSSelector[]>(1);
  selectors[0].SetMatch(lynx::css::LynxCSSSelector::kClass);
  selectors[0].SetValue("foo");
  selectors[0].SetLastInTagHistory(true);
  selectors[0].SetLastInSelectorList(true);

  auto token = fml::MakeRefCounted<lynx::tasm::CSSParseToken>(
      lynx::tasm::CSSParserConfigs());
  lynx::tasm::StyleMap styles;
  styles.emplace(
      lynx::tasm::kPropertyIDOpacity,
      lynx::tasm::CSSValue(0.5, lynx::tasm::CSSValuePattern::NUMBER));
  token->SetAttributes(std::move(styles));
  fragment->AddStyleRule(std::move(selectors), std::move(token));
  bundle.GetCSSStyleManager()->AddSharedCSSFragment(std::move(fragment));

  const auto result = lynx::tasm::LynxTemplateBundleConverter::
      ConvertTemplateBundleToSerializedString(bundle);
  assert(result.find("\"css\":{\"text\":\".foo { opacity: 0.5; }\"}") !=
         std::string::npos);
  std::cout << "  PASSED" << std::endl;
}

std::unique_ptr<lynx::css::LynxCSSSelector[]> MakeClassSelector(
    const std::string& class_name) {
  auto selectors = std::make_unique<lynx::css::LynxCSSSelector[]>(1);
  selectors[0].SetMatch(lynx::css::LynxCSSSelector::kClass);
  selectors[0].SetValue(class_name);
  selectors[0].SetLastInTagHistory(true);
  selectors[0].SetLastInSelectorList(true);
  return selectors;
}

fml::RefPtr<lynx::tasm::CSSParseToken> MakeToken(lynx::tasm::StyleMap styles) {
  auto token = fml::MakeRefCounted<lynx::tasm::CSSParseToken>(
      lynx::tasm::CSSParserConfigs());
  token->SetAttributes(std::move(styles));
  return token;
}

std::string SerializeBundle(lynx::tasm::LynxTemplateBundle& bundle) {
  return lynx::tasm::LynxTemplateBundleConverter::
      ConvertTemplateBundleToSerializedString(bundle);
}

void TestConvertSerializesDeterministicDeclarations() {
  std::cout << "TestConvertSerializesDeterministicDeclarations..." << std::endl;
  lynx::tasm::LynxTemplateBundle bundle;
  auto fragment = std::make_unique<lynx::tasm::SharedCSSFragment>(0);
  fragment->SetEnableCSSSelector();

  auto token = MakeToken({
      {lynx::tasm::kPropertyIDOpacity,
       lynx::tasm::CSSValue(0.5, lynx::tasm::CSSValuePattern::NUMBER)},
      {lynx::tasm::kPropertyIDColor,
       lynx::tasm::CSSValue("red", lynx::tasm::CSSValuePattern::STRING,
                            lynx::tasm::CSSValueType::DEFAULT)},
  });
  lynx::tasm::StyleMap important_styles;
  important_styles.emplace(
      lynx::tasm::kPropertyIDWidth,
      lynx::tasm::CSSValue(10, lynx::tasm::CSSValuePattern::PX));
  token->SetImportantAttributes(std::move(important_styles));
  lynx::tasm::CSSVariableMap variables;
  variables.emplace(lynx::base::String("--theme-color"),
                    lynx::base::String("blue"));
  token->SetStyleVariables(std::move(variables));
  fragment->AddStyleRule(MakeClassSelector("ordered"), std::move(token));

  auto* layer = fragment->GetOrCreateRootLayer()->GetOrAddSubLayer({"base"});
  auto layered_token = MakeToken({
      {lynx::tasm::kPropertyIDOpacity,
       lynx::tasm::CSSValue(1, lynx::tasm::CSSValuePattern::NUMBER)},
  });
  auto layered_rule = fml::MakeRefCounted<lynx::css::StyleRule>(
      MakeClassSelector("layered"), std::move(layered_token));
  fragment->AddStyleRule(std::move(layered_rule), layer);
  bundle.GetCSSStyleManager()->AddSharedCSSFragment(std::move(fragment));

  const auto result = SerializeBundle(bundle);
  assert(
      result.find(".ordered { --theme-color: blue; color: red; opacity: 0.5; "
                  "width: 10px !important; }") != std::string::npos);
  assert(result.find("@layer base { .layered { opacity: 1; } }") !=
         std::string::npos);
  std::cout << "  PASSED" << std::endl;
}

void TestConvertSerializesKeyframes() {
  std::cout << "TestConvertSerializesKeyframes..." << std::endl;
  lynx::tasm::LynxTemplateBundle bundle;
  lynx::tasm::CSSKeyframesTokenMap keyframes;
  auto keyframe = fml::MakeRefCounted<lynx::tasm::CSSKeyframesToken>(
      lynx::tasm::CSSParserConfigs());
  lynx::tasm::CSSKeyframesContent content;
  auto to_styles = std::make_shared<lynx::tasm::StyleMap>();
  to_styles->emplace(
      lynx::tasm::kPropertyIDOpacity,
      lynx::tasm::CSSValue(1, lynx::tasm::CSSValuePattern::NUMBER));
  auto from_styles = std::make_shared<lynx::tasm::StyleMap>();
  from_styles->emplace(
      lynx::tasm::kPropertyIDOpacity,
      lynx::tasm::CSSValue(0, lynx::tasm::CSSValuePattern::NUMBER));
  content.emplace(1.f, std::move(to_styles));
  content.emplace(0.f, std::move(from_styles));
  keyframe->SetKeyframesContent(std::move(content));
  lynx::tasm::CSSKeyframesCustomPropertyContent custom_content;
  auto custom_properties = std::make_shared<lynx::tasm::CustomPropertiesMap>();
  custom_properties->emplace(
      lynx::base::String("--frame-color"),
      lynx::tasm::CSSValue("green", lynx::tasm::CSSValuePattern::STRING,
                           lynx::tasm::CSSValueType::DEFAULT));
  custom_content.emplace(0.f, std::move(custom_properties));
  keyframe->SetKeyframesCustomPropertyContent(std::move(custom_content));
  keyframes.emplace(lynx::base::String("fade"), std::move(keyframe));

  bundle.GetCSSStyleManager()->AddSharedCSSFragment(
      std::make_unique<lynx::tasm::SharedCSSFragment>(
          0, std::vector<int32_t>(), lynx::tasm::CSSParserTokenMap{},
          std::move(keyframes), lynx::tasm::CSSFontFaceRuleMap()));

  const auto result = SerializeBundle(bundle);
  assert(result.find("@keyframes fade { from { --frame-color: green; opacity: "
                     "0; } to { opacity: 1; } }") != std::string::npos);
  std::cout << "  PASSED" << std::endl;
}

void TestConvertSerializesFontFace() {
  std::cout << "TestConvertSerializesFontFace..." << std::endl;
  lynx::tasm::LynxTemplateBundle bundle;
  lynx::tasm::CSSFontFaceRuleMap font_faces;
  auto face = std::make_shared<lynx::tasm::CSSFontFaceRule>(
      "My Font", lynx::tasm::CSSFontFaceAttrsMap{{"src", "url(font.woff2)"}});
  font_faces["My Font"].push_back(std::move(face));
  bundle.GetCSSStyleManager()->AddSharedCSSFragment(
      std::make_unique<lynx::tasm::SharedCSSFragment>(
          0, std::vector<int32_t>(), lynx::tasm::CSSParserTokenMap{},
          lynx::tasm::CSSKeyframesTokenMap(), std::move(font_faces)));

  const auto result = SerializeBundle(bundle);
  assert(result.find("@font-face { font-family: \"My Font\"; src: "
                     "url(font.woff2); }") != std::string::npos);
  std::cout << "  PASSED" << std::endl;
}

void TestConvertSerializesConditionalRules() {
  std::cout << "TestConvertSerializesConditionalRules..." << std::endl;
  lynx::tasm::LynxTemplateBundle bundle;
  auto fragment = std::make_unique<lynx::tasm::SharedCSSFragment>(0);
  fragment->SetEnableCSSSelector();

  auto media_rule =
      fml::MakeRefCounted<lynx::css::ConditionRule>(fragment.get());
  media_rule->SetMediaQueries(
      lynx::css::MediaQueryParser::ParseMediaQuerySet("(min-width: 100px)"));
  media_rule->AddStyleRule(fml::MakeRefCounted<lynx::css::StyleRule>(
      MakeClassSelector("media"),
      MakeToken(
          {{lynx::tasm::kPropertyIDOpacity,
            lynx::tasm::CSSValue(0.5, lynx::tasm::CSSValuePattern::NUMBER)}})));
  fragment->AddConditionRule(std::move(media_rule));

  auto supports_rule =
      fml::MakeRefCounted<lynx::css::ConditionRule>(fragment.get());
  supports_rule->SetSupportsCondition(
      fml::MakeRefCounted<lynx::css::SupportsDeclNode>("color", "red", false));
  supports_rule->AddStyleRule(fml::MakeRefCounted<lynx::css::StyleRule>(
      MakeClassSelector("supports"),
      MakeToken(
          {{lynx::tasm::kPropertyIDColor,
            lynx::tasm::CSSValue("red", lynx::tasm::CSSValuePattern::STRING,
                                 lynx::tasm::CSSValueType::DEFAULT)}})));
  fragment->AddConditionRule(std::move(supports_rule));

  bundle.GetCSSStyleManager()->AddSharedCSSFragment(std::move(fragment));
  const auto result = SerializeBundle(bundle);
  assert(
      result.find("@media (min-width: 100px) { .media { opacity: 0.5; } }") !=
      std::string::npos);
  assert(result.find("@supports (color: red) { .supports { color: red; } }") !=
         std::string::npos);
  std::cout << "  PASSED" << std::endl;
}

void TestConvertSerializesNthSelector() {
  std::cout << "TestConvertSerializesNthSelector..." << std::endl;
  lynx::tasm::LynxTemplateBundle bundle;
  auto fragment = std::make_unique<lynx::tasm::SharedCSSFragment>(0);
  fragment->SetEnableCSSSelector();

  auto selectors = std::make_unique<lynx::css::LynxCSSSelector[]>(1);
  selectors[0].SetMatch(lynx::css::LynxCSSSelector::kPseudoClass);
  selectors[0].SetPseudoType(lynx::css::LynxCSSSelector::kPseudoNthChild);
  selectors[0].SetNth(2, 1);
  selectors[0].SetLastInTagHistory(true);
  selectors[0].SetLastInSelectorList(true);
  fragment->AddStyleRule(
      std::move(selectors),
      MakeToken(
          {{lynx::tasm::kPropertyIDOpacity,
            lynx::tasm::CSSValue(0.5, lynx::tasm::CSSValuePattern::NUMBER)}}));
  bundle.GetCSSStyleManager()->AddSharedCSSFragment(std::move(fragment));

  const auto result = SerializeBundle(bundle);
  assert(result.find(":nth-child(2n+1) { opacity: 0.5; }") !=
         std::string::npos);
  std::cout << "  PASSED" << std::endl;
}

void TestFromBinary() {
  std::cout << "TestFromBinary..." << std::endl;
  std::string valid_json =
      R"({"source":"<div>lazy reader test</div>","config":{}})";
  auto encode_res = lynx::tasm::codec::Encode(valid_json);
  assert(encode_res.status == 0);

  lynx::tasm::LynxTemplateBundle bundle;
  std::string error =
      bundle.FromBinary(std::move(encode_res.buffer), true, "test://bundle");
  assert(error.empty());
  assert(bundle.Size() > 0);
  std::cout << "  PASSED" << std::endl;
}

}  // namespace

int main() {
  std::cout << "=== TASM Codec Facade Unit Tests ===" << std::endl;

  TestEncodeSuccess();
  TestEncodeInvalidJson();
  TestDecodeInvalidPointer();
  TestDecodeInvalidData();
  TestEncodeDecodeRoundTrip();
  TestConvertWithNullPageConfig();
  TestConvertSerializesLegacyCSS();
  TestConvertSerializesCSSRule();
  TestConvertSerializesDeterministicDeclarations();
  TestConvertSerializesKeyframes();
  TestConvertSerializesFontFace();
  TestConvertSerializesConditionalRules();
  TestConvertSerializesNthSelector();
  TestFromBinary();

  std::cout << "=== All tests passed ===" << std::endl;
  return 0;
}
