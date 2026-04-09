// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "template_bundle/template_codec/binary_decoder/lynx_config_auto_gen.h"
#define private public
#define protected public

#include "core/renderer/css/unit_handler.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_config_decoder_unittest.h"

namespace lynx {
namespace tasm {
namespace test {

TEST_F(LynxBinaryConfigDecoderTest, ReadPipelineSchedulerConfig) {
  EXPECT_FALSE(page_config_->GetEnableParallelParseElementTemplate());
  config_decoder_->DecodePageConfig("{\n  \"pipelineSchedulerConfig\" : 1\n}",
                                    page_config_);
  EXPECT_TRUE(page_config_->GetEnableParallelParseElementTemplate());
}

TEST_F(LynxBinaryConfigDecoderTest, ReadEnableAsyncResolveSubtree) {
  EXPECT_FALSE(page_config_->GetEnableAsyncResolveSubtree() ==
               TernaryBool::TRUE_VALUE);
  config_decoder_->DecodePageConfig(
      "{\n  \"enableAsyncResolveSubtree\" : true\n}", page_config_);
  EXPECT_TRUE(page_config_->GetEnableAsyncResolveSubtree() ==
              TernaryBool::TRUE_VALUE);
}

TEST_F(LynxBinaryConfigDecoderTest, ReadEnableParseIntFlex) {
  EXPECT_FALSE(page_config_->GetEnableParseIntFlex());
  EXPECT_FALSE(page_config_->GetCSSParserConfigs().enable_parse_int_flex);
  config_decoder_->DecodePageConfig("{\n  \"enableParseIntFlex\" : true\n}",
                                    page_config_);
  EXPECT_TRUE(page_config_->GetEnableParseIntFlex());
  EXPECT_TRUE(page_config_->GetCSSParserConfigs().enable_parse_int_flex);
}

TEST_F(LynxBinaryConfigDecoderTest,
       ParseIntFlexDisabledKeepsLegacyFlexParsing) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  auto impl = lepus::Value(2);
  StyleMap output;

  auto ret = UnitHandler::Process(id, impl, output,
                                  page_config_->GetCSSParserConfigs());
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 0);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 0);
}

TEST_F(LynxBinaryConfigDecoderTest, EnableParseIntFlexAffectsFlexParsing) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  auto impl = lepus::Value(2);
  StyleMap output;

  config_decoder_->DecodePageConfig("{\n  \"enableParseIntFlex\" : true\n}",
                                    page_config_);
  auto ret = UnitHandler::Process(id, impl, output,
                                  page_config_->GetCSSParserConfigs());
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexGrow].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexGrow].AsNumber(), 2);
  EXPECT_TRUE(output[kPropertyIDFlexShrink].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexShrink].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 0);
}

TEST_F(LynxBinaryConfigDecoderTest, ReadEnableFlexBasisZeroPercent) {
  EXPECT_FALSE(page_config_->GetEnableFlexBasisZeroPercent());
  EXPECT_FALSE(
      page_config_->GetCSSParserConfigs().enable_flex_basis_zero_percent);
  config_decoder_->DecodePageConfig(
      "{\n  \"enableFlexBasisZeroPercent\" : true\n}", page_config_);
  EXPECT_TRUE(page_config_->GetEnableFlexBasisZeroPercent());
  EXPECT_TRUE(
      page_config_->GetCSSParserConfigs().enable_flex_basis_zero_percent);
}

TEST_F(LynxBinaryConfigDecoderTest,
       FlexBasisZeroPercentDisabledKeepsDefaultBehavior) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  auto impl = lepus::Value(1);
  StyleMap output;

  auto ret = UnitHandler::Process(id, impl, output,
                                  page_config_->GetCSSParserConfigs());
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_TRUE(output[kPropertyIDFlexBasis].IsNumber());
  EXPECT_EQ(output[kPropertyIDFlexBasis].GetPattern(), CSSValuePattern::NUMBER);
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 0);
}

TEST_F(LynxBinaryConfigDecoderTest,
       EnableFlexBasisZeroPercentUsesPercentPattern) {
  auto id = CSSPropertyID::kPropertyIDFlex;
  auto impl = lepus::Value(1);
  StyleMap output;

  config_decoder_->DecodePageConfig(
      "{\n  \"enableFlexBasisZeroPercent\" : true\n}", page_config_);
  auto ret = UnitHandler::Process(id, impl, output,
                                  page_config_->GetCSSParserConfigs());
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), 3);
  EXPECT_EQ(output[kPropertyIDFlexBasis].GetPattern(),
            CSSValuePattern::PERCENT);
  EXPECT_EQ(output[kPropertyIDFlexBasis].AsNumber(), 0);
}

TEST_F(LynxBinaryConfigDecoderTest, ReadEnableGridPlacementShorthands) {
  EXPECT_FALSE(page_config_->GetEnableGridPlacementShorthands());
  EXPECT_FALSE(
      page_config_->GetCSSParserConfigs().enable_grid_placement_shorthands);
  config_decoder_->DecodePageConfig(
      "{\n  \"enableGridPlacementShorthands\" : true\n}", page_config_);
  EXPECT_TRUE(page_config_->GetEnableGridPlacementShorthands());
  EXPECT_TRUE(
      page_config_->GetCSSParserConfigs().enable_grid_placement_shorthands);
}

TEST_F(LynxBinaryConfigDecoderTest,
       GridPlacementShorthandsDisabledKeepsDefaultBehavior) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  auto impl = lepus::Value("1 / 4");
  StyleMap output;

  auto ret = UnitHandler::Process(id, impl, output,
                                  page_config_->GetCSSParserConfigs());
  EXPECT_FALSE(ret);
  EXPECT_TRUE(output.empty());
}

TEST_F(LynxBinaryConfigDecoderTest,
       EnableGridPlacementShorthandsUsesShorthandHandler) {
  auto id = CSSPropertyID::kPropertyIDGridColumn;
  auto impl = lepus::Value("1 / 4");
  StyleMap output;

  config_decoder_->DecodePageConfig(
      "{\n  \"enableGridPlacementShorthands\" : true\n}", page_config_);
  auto ret = UnitHandler::Process(id, impl, output,
                                  page_config_->GetCSSParserConfigs());
  EXPECT_TRUE(ret);
  EXPECT_EQ(output.size(), static_cast<size_t>(2));
  EXPECT_TRUE(output[kPropertyIDGridColumnStart].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnStart].AsNumber(), 1);
  EXPECT_TRUE(output[kPropertyIDGridColumnEnd].IsNumber());
  EXPECT_EQ(output[kPropertyIDGridColumnEnd].AsNumber(), 4);
}

TEST_F(LynxBinaryConfigDecoderTest, ReadDebugMetadataUrl) {
  config_decoder_->DecodePageConfig(
      "{\n  \"debugMetadataUrl\" : \"https://example.com/debug-info.json\"\n}",
      page_config_);
  EXPECT_EQ(page_config_->GetDebugMetadataUrl(),
            "https://example.com/debug-info.json");
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
