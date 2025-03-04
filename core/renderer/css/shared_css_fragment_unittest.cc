// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/css/shared_css_fragment.h"

#include "core/renderer/css/unit_handler.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
using namespace tasm;
namespace css {
namespace testing {
TEST(SharedCSSFragment, Handler) {
  // create the index fragment with four css classes
  StyleMap indexAttributes;
  CSSParserConfigs configs;
  auto indexTokens = std::make_shared<CSSParseToken>(configs);
  CSSParserTokenMap indexTokensMap;
  // create class text-1
  auto id = CSSPropertyID::kPropertyIDFontSize;
  auto impl = lepus::Value("18px");
  bool ret = false;
  ret = UnitHandler::Process(id, impl, indexAttributes, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDTextAlign;
  impl = lepus::Value("center");
  ret = UnitHandler::Process(id, impl, indexAttributes, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDColor;
  impl = lepus::Value("red");
  ret = UnitHandler::Process(id, impl, indexAttributes, configs);
  EXPECT_TRUE(ret);
  indexTokens.get()->SetAttributes(std::move(indexAttributes));
  indexTokensMap.insert(
      std::make_pair(std::string(".text-1"), std::move(indexTokens)));

  // create class text-3
  StyleMap indexAttributes1;
  auto indexTokens1 = std::make_shared<CSSParseToken>(configs);
  id = CSSPropertyID::kPropertyIDFontSize;
  impl = lepus::Value("18px");
  ret = UnitHandler::Process(id, impl, indexAttributes1, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDTextAlign;
  impl = lepus::Value("center");
  ret = UnitHandler::Process(id, impl, indexAttributes1, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDColor;
  impl = lepus::Value("blueviolet");
  ret = UnitHandler::Process(id, impl, indexAttributes1, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDFontWeight;
  impl = lepus::Value("bold");
  ret = UnitHandler::Process(id, impl, indexAttributes1, configs);
  EXPECT_TRUE(ret);
  indexTokens1.get()->SetAttributes(std::move(indexAttributes1));
  indexTokensMap.insert(
      std::make_pair(std::string(".text-3"), std::move(indexTokens1)));
  // create class text-4
  StyleMap indexAttributes2;
  auto indexTokens2 = std::make_shared<CSSParseToken>(configs);
  id = CSSPropertyID::kPropertyIDBackgroundColor;
  impl = lepus::Value("blue");
  ret = UnitHandler::Process(id, impl, indexAttributes2, configs);
  EXPECT_TRUE(ret);
  indexTokens2.get()->SetAttributes(std::move(indexAttributes2));
  indexTokensMap.insert(
      std::make_pair(std::string(".text-4"), std::move(indexTokens2)));
  // create class text-5
  StyleMap indexAttributes3;
  auto indexTokens3 = std::make_shared<CSSParseToken>(configs);
  id = CSSPropertyID::kPropertyIDBackgroundColor;
  impl = lepus::Value("lightblue");
  ret = UnitHandler::Process(id, impl, indexAttributes3, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDFontStyle;
  impl = lepus::Value("italic");
  ret = UnitHandler::Process(id, impl, indexAttributes3, configs);
  EXPECT_TRUE(ret);
  indexTokens3.get()->SetAttributes(std::move(indexAttributes3));
  indexTokensMap.insert(
      std::make_pair(std::string(".text-5"), std::move(indexTokens3)));

  const std::vector<int32_t> dependent_ids;
  CSSKeyframesTokenMap keyframes;
  CSSFontFaceRuleMap fontfaces;
  SharedCSSFragment indexFragment(1, dependent_ids, indexTokensMap, keyframes,
                                  fontfaces);
  indexFragment.SetEnableClassMerge(true);

  // create import fragment

  CSSParserTokenMap importTokensMap;
  // create class text-2
  StyleMap importAttributes1;
  auto importTokens1 = std::make_shared<CSSParseToken>(configs);
  id = CSSPropertyID::kPropertyIDFontSize;
  impl = lepus::Value("20px");
  ret = UnitHandler::Process(id, impl, importAttributes1, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDTextAlign;
  impl = lepus::Value("center");
  ret = UnitHandler::Process(id, impl, importAttributes1, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDColor;
  impl = lepus::Value("green");
  ret = UnitHandler::Process(id, impl, importAttributes1, configs);
  EXPECT_TRUE(ret);
  importTokens1.get()->SetAttributes(std::move(importAttributes1));
  importTokensMap.insert(
      std::make_pair(std::string(".text-2"), std::move(importTokens1)));
  // create class text-3
  StyleMap importAttributes2;
  auto importTokens2 = std::make_shared<CSSParseToken>(configs);
  id = CSSPropertyID::kPropertyIDFontWeight;
  impl = lepus::Value("100");
  ret = UnitHandler::Process(id, impl, importAttributes2, configs);
  EXPECT_TRUE(ret);
  importTokens2.get()->SetAttributes(std::move(importAttributes2));
  importTokensMap.insert(
      std::make_pair(std::string(".text-3"), std::move(importTokens2)));
  // create class text-4
  StyleMap importAttributes3;
  auto importTokens3 = std::make_shared<CSSParseToken>(configs);
  id = CSSPropertyID::kPropertyIDFontSize;
  impl = lepus::Value("18px");
  ret = UnitHandler::Process(id, impl, importAttributes3, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDTextAlign;
  impl = lepus::Value("center");
  ret = UnitHandler::Process(id, impl, importAttributes3, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDBackgroundColor;
  impl = lepus::Value("red");
  ret = UnitHandler::Process(id, impl, importAttributes3, configs);
  EXPECT_TRUE(ret);
  importTokens3.get()->SetAttributes(std::move(importAttributes3));
  importTokensMap.insert(
      std::make_pair(std::string(".text-4"), std::move(importTokens3)));
  // create class text-5
  StyleMap importAttributes4;
  auto importTokens4 = std::make_shared<CSSParseToken>(configs);
  id = CSSPropertyID::kPropertyIDFontSize;
  impl = lepus::Value("20px");
  ret = UnitHandler::Process(id, impl, importAttributes4, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDFontWeight;
  impl = lepus::Value("bold");
  ret = UnitHandler::Process(id, impl, importAttributes4, configs);
  EXPECT_TRUE(ret);
  id = CSSPropertyID::kPropertyIDColor;
  impl = lepus::Value("orange");
  ret = UnitHandler::Process(id, impl, importAttributes4, configs);
  EXPECT_TRUE(ret);
  importTokens4.get()->SetAttributes(std::move(importAttributes4));
  importTokensMap.insert(
      std::make_pair(std::string(".text-5"), std::move(importTokens4)));

  const std::vector<int32_t> import_dependent_ids;
  CSSKeyframesTokenMap import_keyframes;
  CSSFontFaceRuleMap import_fontfaces;
  SharedCSSFragment importFragment(1, import_dependent_ids, importTokensMap,
                                   import_keyframes, import_fontfaces);
  importFragment.SetEnableClassMerge(true);

  EXPECT_EQ(indexFragment.css().size(), static_cast<size_t>(4));
  EXPECT_EQ(importFragment.css().size(), static_cast<size_t>(4));
  // start to merge two fragments
  indexFragment.ImportOtherFragment(&importFragment);
  // check the fragment size
  EXPECT_EQ(indexFragment.css().size(), static_cast<size_t>(5));
  auto css = indexFragment.css();
  EXPECT_TRUE(css.find(".text-1") != css.end());
  EXPECT_TRUE(css.find(".text-2") != css.end());
  EXPECT_TRUE(css.find(".text-3") != css.end());
  EXPECT_TRUE(css.find(".text-4") != css.end());
  EXPECT_TRUE(css.find(".text-5") != css.end());
  EXPECT_TRUE(indexFragment.GetCSSStyle(".text-1"));
  // check text-1 style
  auto style = css[".text-1"]->GetAttributes();
  EXPECT_EQ(style.size(), static_cast<size_t>(3));
  id = kPropertyIDFontSize;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsNumber());
  EXPECT_EQ(style[id].GetValue().Number(), 18);
  EXPECT_TRUE(style[id].IsPx());
  id = kPropertyIDTextAlign;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsInt32());
  EXPECT_EQ((starlight::TextAlignType)style[id].GetValue().Number(),
            starlight::TextAlignType::kCenter);
  id = kPropertyIDColor;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_EQ(style[id].GetValue().UInt32(), 4294901760);
  // check text-2 style
  style = css[".text-2"]->GetAttributes();
  EXPECT_EQ(style.size(), static_cast<size_t>(3));
  id = kPropertyIDFontSize;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsNumber());
  EXPECT_EQ(style[id].GetValue().Number(), 20);
  EXPECT_TRUE(style[id].IsPx());
  id = kPropertyIDTextAlign;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsInt32());
  EXPECT_EQ((starlight::TextAlignType)style[id].GetValue().Number(),
            starlight::TextAlignType::kCenter);
  id = kPropertyIDColor;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_EQ(style[id].GetValue().UInt32(), 4278222848);
  // check text-3 style
  style = css[".text-3"]->GetAttributes();
  EXPECT_EQ(style.size(), static_cast<size_t>(4));
  id = kPropertyIDFontSize;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsNumber());
  EXPECT_EQ(style[id].GetValue().Number(), 18);
  EXPECT_TRUE(style[id].IsPx());
  id = kPropertyIDTextAlign;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsInt32());
  EXPECT_EQ((starlight::TextAlignType)style[id].GetValue().Number(),
            starlight::TextAlignType::kCenter);
  id = kPropertyIDColor;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_EQ(style[id].GetValue().UInt32(), 4287245282);
  id = kPropertyIDFontWeight;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsInt32());
  EXPECT_EQ((starlight::FontWeightType)style[id].GetValue().Number(),
            starlight::FontWeightType::kBold);
  // check text-4 style
  style = css[".text-4"]->GetAttributes();
  EXPECT_EQ(style.size(), static_cast<size_t>(3));
  id = kPropertyIDFontSize;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsNumber());
  EXPECT_EQ(style[id].GetValue().Number(), 18);
  EXPECT_TRUE(style[id].IsPx());
  id = kPropertyIDTextAlign;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsInt32());
  EXPECT_EQ((starlight::TextAlignType)style[id].GetValue().Number(),
            starlight::TextAlignType::kCenter);
  id = kPropertyIDBackgroundColor;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_EQ(style[id].GetValue().UInt32(), 4278190335);
  // check text-5 style
  style = css[".text-5"]->GetAttributes();
  EXPECT_EQ(style.size(), static_cast<size_t>(5));
  id = kPropertyIDFontSize;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsNumber());
  EXPECT_EQ(style[id].GetValue().Number(), 20);
  EXPECT_TRUE(style[id].IsPx());
  id = kPropertyIDFontWeight;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsInt32());
  EXPECT_EQ((starlight::FontWeightType)style[id].GetValue().Number(),
            starlight::FontWeightType::kBold);
  id = kPropertyIDFontStyle;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_TRUE(style[id].GetValue().IsInt32());
  EXPECT_EQ((starlight::FontStyleType)style[id].GetValue().Number(),
            starlight::FontStyleType::kItalic);
  id = kPropertyIDBackgroundColor;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_EQ(style[id].GetValue().UInt32(), 4289583334);
  id = kPropertyIDColor;
  EXPECT_FALSE(style.find(id) == style.end());
  EXPECT_EQ(style[id].GetValue().UInt32(), 4294944000);
}

TEST(SharedCSSFragment, CheckHasID) {
  bool hasIdSelector = false;
  {
    StyleMap indexAttributes;
    CSSParserConfigs configs;
    auto tokens = std::make_shared<CSSParseToken>(configs);

    CSSParserTokenMap indexTokensMap;
    auto id = CSSPropertyID::kPropertyIDFontSize;
    auto impl = lepus::Value("18px");
    UnitHandler::Process(id, impl, indexAttributes, configs);
    tokens.get()->SetAttributes(std::move(indexAttributes));

    std::string key = ".text-1";
    auto& sheets = tokens->sheets();
    auto shared_css_sheet = std::make_shared<CSSSheet>(key);
    sheets.emplace_back(shared_css_sheet);

    indexTokensMap.insert(std::make_pair(key, tokens));

    const std::vector<int32_t> dependent_ids;
    CSSKeyframesTokenMap keyframes;
    CSSFontFaceRuleMap fontfaces;
    SharedCSSFragment indexFragment(1, dependent_ids, indexTokensMap, keyframes,
                                    fontfaces);
    indexFragment.FindSpecificMapAndAdd(key, tokens);
    hasIdSelector = indexFragment.HasIdSelector();
  }
  EXPECT_FALSE(hasIdSelector);

  {
    StyleMap indexAttributes1;
    CSSParserConfigs configs;
    auto tokens1 = std::make_shared<CSSParseToken>(configs);

    CSSParserTokenMap indexTokensMap1;
    auto id1 = CSSPropertyID::kPropertyIDFontSize;
    auto impl1 = lepus::Value("28px");
    UnitHandler::Process(id1, impl1, indexAttributes1, configs);
    tokens1.get()->SetAttributes(std::move(indexAttributes1));

    std::string key1 = "#TestID";
    auto& sheets1 = tokens1->sheets();
    auto shared_css_sheet1 = std::make_shared<CSSSheet>(key1);
    sheets1.emplace_back(shared_css_sheet1);

    indexTokensMap1.insert(std::make_pair(key1, tokens1));

    const std::vector<int32_t> dependent_ids1;
    CSSKeyframesTokenMap keyframes1;
    CSSFontFaceRuleMap fontfaces1;
    SharedCSSFragment indexFragment1(1, dependent_ids1, indexTokensMap1,
                                     keyframes1, fontfaces1);
    indexFragment1.FindSpecificMapAndAdd(key1, tokens1);
    hasIdSelector = indexFragment1.HasIdSelector();
  }
  EXPECT_TRUE(hasIdSelector);
}

}  // namespace testing

}  // namespace css
}  // namespace lynx
