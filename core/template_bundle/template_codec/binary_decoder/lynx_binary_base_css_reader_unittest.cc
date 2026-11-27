// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/template_bundle/template_codec/binary_decoder/lynx_binary_base_css_reader.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/base/json/json_util.h"
#include "core/renderer/css/css_variable_handler.h"
#include "core/renderer/dom/vdom/radon/radon_node.h"
#include "core/runtime/lepus/binary_input_stream.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/template_bundle/template_codec/binary_decoder/template_binary_reader.h"
#include "core/template_bundle/template_codec/binary_encoder/csr_element_binary_writer.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser.h"
#include "core/template_bundle/template_codec/binary_encoder/template_binary_writer.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {
namespace {

class CSSFragmentBinaryWriter : public TemplateBinaryWriter {
 public:
  explicit CSSFragmentBinaryWriter(const CompileOptions& options)
      : TemplateBinaryWriter(nullptr, runtime::ContextType::VMContextType, true,
                             nullptr, nullptr, nullptr, nullptr, nullptr,
                             nullptr, "", "", "", "", "", "", {}, options,
                             lepus::Value(), lepus::Value(), {}, nullptr) {}

  using TemplateBinaryWriter::EncodeCSSFragmentToVector;
};

class CSSFragmentBinaryReader : public LynxBinaryBaseCSSReader {
 public:
  CSSFragmentBinaryReader(std::vector<uint8_t> data,
                          const CompileOptions& options)
      : LynxBinaryBaseCSSReader(
            std::make_unique<lepus::ByteArrayInputStream>(std::move(data))) {
    compile_options_ = options;
    decode_string_directly_ = true;
  }

  using LynxBinaryBaseCSSReader::DecodeCSSFragment;
};

class NestedTouchPseudoDecodeTest : public ::testing::TestWithParam<bool> {};

TEST_P(NestedTouchPseudoDecodeTest, MarksTouchPseudoFromNotSelector) {
  CompileOptions options;
  options.target_sdk_version_ = "3.2";
  options.enable_css_selector_ = true;
  options.enable_css_rule_ = GetParam();

  auto rules = base::strToJson(R"json([
    {
      "type": "StyleRule",
      "selectorText": {
        "value": ".button:not(:active)",
        "loc": { "line": 1, "column": 1 }
      },
      "style": [
        {
          "name": "opacity",
          "value": "0.5",
          "keyLoc": { "line": 1, "column": 24 },
          "valLoc": { "line": 1, "column": 33 }
        }
      ],
      "variables": {}
    }
  ])json");

  CSSParser parser(options);
  auto encoded_fragment =
      parser.ParseExternalFragment(rules, "/nested-active.ttss");
  ASSERT_NE(encoded_fragment, nullptr);

  CSSFragmentBinaryWriter writer(options);
  auto data = writer.EncodeCSSFragmentToVector(encoded_fragment.get());
  ASSERT_FALSE(data.empty());
  const size_t fragment_size = data.size();

  CSSFragmentBinaryReader reader(std::move(data), options);
  SharedCSSFragment decoded_fragment;
  ASSERT_TRUE(reader.DecodeCSSFragment(&decoded_fragment, fragment_size));
  EXPECT_TRUE(decoded_fragment.HasTouchPseudoToken());
}

INSTANTIATE_TEST_SUITE_P(CSSSelectorAndCSSRule, NestedTouchPseudoDecodeTest,
                         ::testing::Bool());

class LazyElementTemplateTestReader : public TemplateBinaryReader {
 public:
  using TemplateBinaryReader::TemplateBinaryReader;

  bool InitForLazyDecode(
      const CompileOptions& options,
      const std::shared_ptr<std::vector<base::String>>& string_list,
      size_t templates_offset, size_t parsed_styles_offset) {
    compile_options_ = options;
    enable_css_parser_ = true;
    enable_css_variable_ = true;
    enable_css_variable_multi_default_value_ = true;
    enable_css_inline_variables_ = true;
    SetStringList(string_list);
    stream_->Seek(templates_offset);
    if (!DecodeElementTemplatesRouter()) {
      return false;
    }
    stream_->Seek(parsed_styles_offset);
    return DecodeParsedStylesRouter();
  }
};

TEST(LazyElementTemplateDecodeTest,
     MatchesRecyclerSemanticsWhenCSSInlineVariablesEnabled) {
  CompileOptions options;
  options.target_sdk_version_ = "3.2";
  options.enable_css_parser_ = true;
  options.enable_css_variable_ = true;

  auto templates_doc = base::strToJson(R"json({
    "$tpl": [
      {
        "type": "view",
        "parsedStyle": [{
          "style": [
            {"name": "height", "type": "css_var",
             "value": "calc({{--main-height}} - 100px)",
             "defaultValue": "calc(300px - 100px)"},
            {"name": "border", "type": "css_var",
             "value": "2px {{--var-style}} {{--var-color}}",
             "defaultValue": "2px double red",
             "defaultValueMap": {"--var-style": "double", "--var-color": "red"}}
          ],
          "variables": {}
        }]
      },
      {"type": "view", "parsedStyleKey": "shared_calc"}
    ]
  })json");
  auto parsed_styles_doc = base::strToJson(R"json({
    "shared_calc": [{
      "style": [
        {"name": "width", "type": "css_var",
         "value": "calc({{--main-width}} - 20px)",
         "defaultValue": "calc(200px - 20px)"}
      ],
      "variables": {}
    }]
  })json");

  lepus::VMContext vm_context;
  CSRElementBinaryWriter writer(&vm_context, options);
  const size_t templates_offset = writer.Offset();
  writer.EncodeTemplatesToBinary(&templates_doc);
  const size_t parsed_styles_offset = writer.Offset();
  writer.EncodeParsedStylesToBinary(&parsed_styles_doc);

  auto string_list = std::make_shared<std::vector<base::String>>(
      vm_context.string_table()->string_list);
  LazyElementTemplateTestReader lazy_reader(
      std::make_unique<lepus::ByteArrayInputStream>(writer.byte_array()));
  ASSERT_TRUE(lazy_reader.InitForLazyDecode(
      options, string_list, templates_offset, parsed_styles_offset));

  auto recycler_delegate = lazy_reader.CreateRecycler();
  ASSERT_NE(recycler_delegate, nullptr);
  auto lazy_info =
      lazy_reader.DecodeElementTemplateInRender("$tpl", string_list);
  auto recycler_info =
      static_cast<TemplateBinaryReader*>(recycler_delegate.get())
          ->DecodeTemplatesInfoWithKey("$tpl");
  ASSERT_NE(lazy_info, nullptr);
  ASSERT_NE(recycler_info, nullptr);
  ASSERT_EQ(lazy_info->elements_.size(), 2u);
  ASSERT_EQ(recycler_info->elements_.size(), 2u);

  const auto& lazy_inline = lazy_info->elements_[0].parsed_styles_->first;
  const auto& recycler_inline =
      recycler_info->elements_[0].parsed_styles_->first;
  const auto& lazy_shared = lazy_info->elements_[1].parsed_styles_->first;
  const auto& recycler_shared =
      recycler_info->elements_[1].parsed_styles_->first;
  EXPECT_EQ(lazy_inline, recycler_inline);
  EXPECT_EQ(lazy_shared, recycler_shared);
  ASSERT_NE(lazy_inline.find(kPropertyIDHeight), lazy_inline.end());
  ASSERT_NE(lazy_inline.find(kPropertyIDBorder), lazy_inline.end());
  ASSERT_NE(lazy_shared.find(kPropertyIDWidth), lazy_shared.end());
  EXPECT_FALSE(
      lazy_inline.find(kPropertyIDHeight)->second.NeedsVariableResolution());
  EXPECT_FALSE(
      lazy_inline.find(kPropertyIDBorder)->second.NeedsVariableResolution());
  EXPECT_FALSE(
      lazy_shared.find(kPropertyIDWidth)->second.NeedsVariableResolution());

  CSSVariableHandler handler(true);
  CSSParserConfigs parser_configs;
  RadonNode node(nullptr, "view", 0);
  node.UpdateCSSVariable("--var-style", "solid");

  StyleMap lazy_resolved = lazy_inline;
  StyleMap recycler_resolved = recycler_inline;
  EXPECT_TRUE(handler.HandleCSSVariables(
      lazy_resolved, node.attribute_holder().get(), parser_configs));
  EXPECT_TRUE(handler.HandleCSSVariables(
      recycler_resolved, node.attribute_holder().get(), parser_configs));
  EXPECT_EQ(lazy_resolved, recycler_resolved);
  ASSERT_NE(lazy_resolved.find(kPropertyIDHeight), lazy_resolved.end());
  EXPECT_EQ(lazy_resolved.find(kPropertyIDHeight)->second.AsStdString(),
            "calc(300px - 100px)");
  ASSERT_NE(lazy_resolved.find(kPropertyIDBorderTopWidth), lazy_resolved.end());
  ASSERT_NE(lazy_resolved.find(kPropertyIDBorderTopStyle), lazy_resolved.end());
  ASSERT_NE(lazy_resolved.find(kPropertyIDBorderTopColor), lazy_resolved.end());
  EXPECT_EQ(lazy_resolved.find(kPropertyIDBorderTopWidth)->second.GetNumber(),
            2);
  EXPECT_EQ(lazy_resolved.find(kPropertyIDBorderTopStyle)
                ->second.GetEnum<starlight::BorderStyleType>(),
            starlight::BorderStyleType::kSolid);
  EXPECT_EQ(
      static_cast<uint32_t>(
          lazy_resolved.find(kPropertyIDBorderTopColor)->second.GetNumber()),
      0xFFFF0000u);

  StyleMap lazy_shared_resolved = lazy_shared;
  StyleMap recycler_shared_resolved = recycler_shared;
  EXPECT_TRUE(handler.HandleCSSVariables(
      lazy_shared_resolved, node.attribute_holder().get(), parser_configs));
  EXPECT_TRUE(handler.HandleCSSVariables(
      recycler_shared_resolved, node.attribute_holder().get(), parser_configs));
  EXPECT_EQ(lazy_shared_resolved, recycler_shared_resolved);
  ASSERT_NE(lazy_shared_resolved.find(kPropertyIDWidth),
            lazy_shared_resolved.end());
  EXPECT_EQ(lazy_shared_resolved.find(kPropertyIDWidth)->second.AsStdString(),
            "calc(200px - 20px)");
}

}  // namespace
}  // namespace test
}  // namespace tasm
}  // namespace lynx
