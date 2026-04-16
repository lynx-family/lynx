// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser.h"

#include "core/base/json/json_util.h"
#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser_token.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {
namespace test {

// TODO(songshourui.null): add unit test here.

TEST(CSSParser, MergeCSSParseTokenPreservesImportantAttributes) {
  rapidjson::Document doc;
  doc.SetObject();

  rapidjson::Value style_origin(rapidjson::kArrayType);
  rapidjson::Value entry_origin(rapidjson::kObjectType);
  entry_origin.AddMember("name", "width", doc.GetAllocator());
  entry_origin.AddMember("value", "100px !important", doc.GetAllocator());
  style_origin.PushBack(entry_origin, doc.GetAllocator());

  rapidjson::Value style_new(rapidjson::kArrayType);
  rapidjson::Value entry_new(rapidjson::kObjectType);
  entry_new.AddMember("name", "width", doc.GetAllocator());
  entry_new.AddMember("value", "200px", doc.GetAllocator());
  style_new.PushBack(entry_new, doc.GetAllocator());

  rapidjson::Value style_vars(rapidjson::kObjectType);
  std::string rule = ".container";
  tasm::CompileOptions options;
  options.enable_css_parser_ = true;
  options.target_sdk_version_ = "3.9";

  fml::RefPtr<tasm::CSSParseToken> origin =
      fml::MakeRefCounted<encoder::CSSParseToken>(style_origin, rule, "",
                                                  style_vars, options);
  fml::RefPtr<tasm::CSSParseToken> new_token =
      fml::MakeRefCounted<encoder::CSSParseToken>(style_new, rule, "",
                                                  style_vars, options);

  EXPECT_TRUE(
      origin->GetImportantAttributes().contains(tasm::kPropertyIDWidth));
  EXPECT_FALSE(origin->GetAttributes().contains(tasm::kPropertyIDWidth));
  EXPECT_TRUE(new_token->GetAttributes().contains(tasm::kPropertyIDWidth));
  EXPECT_FALSE(
      new_token->GetImportantAttributes().contains(tasm::kPropertyIDWidth));

  CSSParser::MergeCSSParseToken(origin, new_token);

  EXPECT_TRUE(origin->GetAttributes().contains(tasm::kPropertyIDWidth));
  EXPECT_TRUE(
      origin->GetImportantAttributes().contains(tasm::kPropertyIDWidth));
}

}  // namespace test
}  // namespace tasm
}  // namespace lynx
