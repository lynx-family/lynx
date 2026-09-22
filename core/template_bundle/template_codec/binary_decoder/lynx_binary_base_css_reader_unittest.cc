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
#include "core/runtime/lepus/binary_input_stream.h"
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

}  // namespace
}  // namespace test
}  // namespace tasm
}  // namespace lynx
