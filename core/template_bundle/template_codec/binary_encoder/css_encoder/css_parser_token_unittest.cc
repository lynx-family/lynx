// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/template_bundle/template_codec/binary_encoder/css_encoder/css_parser_token.h"

#include "core/base/json/json_util.h"
#include "core/base/thread/once_task.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace encoder {

TEST(EncodeCSSParseToken, GetAttributes) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);
  tokens->raw_attributes().insert_or_assign(
      tasm::CSSPropertyID::kPropertyIDFontSize,
      tasm::CSSValue(lepus::Value("18px")));

  EXPECT_FALSE(tokens->raw_attributes().empty());

  const auto& attrs = tokens->GetAttributes();
  EXPECT_TRUE(tokens->raw_attributes().empty());

  tasm::StyleMap expected;
  tasm::UnitHandler::Process(tasm::CSSPropertyID::kPropertyIDFontSize,
                             lepus::Value("18px"), expected, parser_configs);
  EXPECT_EQ(attrs.find(tasm::CSSPropertyID::kPropertyIDFontSize)->second,
            expected.find(tasm::CSSPropertyID::kPropertyIDFontSize)->second);
}

TEST(EncodeCSSParseToken, ParallelGetAttributes) {
  tasm::CSSParserConfigs parser_configs;
  tasm::StyleMap expected;
  tasm::UnitHandler::Process(tasm::CSSPropertyID::kPropertyIDFontSize,
                             lepus::Value("18px"), expected, parser_configs);
  tasm::UnitHandler::Process(tasm::CSSPropertyID::kPropertyIDHeight,
                             lepus::Value("18px"), expected, parser_configs);
  tasm::UnitHandler::Process(tasm::CSSPropertyID::kPropertyIDWidth,
                             lepus::Value("18px"), expected, parser_configs);

  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);
  tokens->SetAttributes(std::move(expected));

  using TestOnceTaskRefptr = base::OnceTaskRefptr<void>;
  std::vector<TestOnceTaskRefptr> task_vec{};

  for (int i = 0; i < 10000; ++i) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();

    auto task = fml::MakeRefCounted<base::OnceTask<void>>(
        [tokens, promise = std::move(promise)]() mutable {
          tokens->GetAttributes();
          promise.set_value();
        },
        std::move(future));

    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [task]() { task->Run(); }, base::ConcurrentTaskType::HIGH_PRIORITY);

    task_vec.emplace_back(std::move(task));
  }

  for (auto item : task_vec) {
    item->GetFuture().get();
  }
}

TEST(EncodeCSSParseToken, TestTouchPseudoToken) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  tokens->MarkAsTouchPseudoToken();
  EXPECT_TRUE(tokens->IsTouchPseudoToken());
}

TEST(EncodeCSSParseToken, TestGetStyleTokenTypeCornerCase) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);
  EXPECT_EQ(tokens->GetStyleTokenType(), 0);

  tokens->sheets_.push_back(nullptr);
  EXPECT_EQ(tokens->GetStyleTokenType(), 0);
}

TEST(EncodeCSSParseToken, TestIsPseudoStyleTokenCornerCase) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);
  tokens->sheets_.push_back(nullptr);

  EXPECT_FALSE(tokens->IsPseudoStyleToken());
}

TEST(EncodeCSSParseToken, TestIsGlobalPseudoStyleToken) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  tokens->sheets_.push_back(nullptr);
  EXPECT_FALSE(tokens->IsGlobalPseudoStyleToken());

  tokens->sheets_.push_back(std::make_shared<tasm::CSSSheet>(":active"));
  EXPECT_TRUE(tokens->IsGlobalPseudoStyleToken());

  tokens->sheets_.push_back(std::make_shared<tasm::CSSSheet>(".ab"));
  EXPECT_FALSE(tokens->IsGlobalPseudoStyleToken());
}

TEST(EncodeCSSParseToken, TestSplitRules) {
  std::string str = "";
  std::string pattern = " ";
  std::vector<std::string> res;

  CSSParseToken::SplitRules(str, pattern, res);
  EXPECT_TRUE(res.empty());

  res.clear();
  str = "a";
  CSSParseToken::SplitRules(str, pattern, res);
  EXPECT_EQ(res.size(), 1);
  EXPECT_EQ(res[0], "a");

  res.clear();
  str = "a b";
  CSSParseToken::SplitRules(str, pattern, res);
  EXPECT_EQ(res.size(), 2);
  EXPECT_EQ(res[0], "a");
  EXPECT_EQ(res[1], "b");
}

TEST(EncodeCSSParseToken, TestCreatSheet) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  auto sheet = tokens->CreatSheet("a", nullptr);
  EXPECT_EQ(sheet->parent_, nullptr);

  auto sheet_1 = tokens->CreatSheet("b", sheet);
  EXPECT_EQ(sheet_1->parent_, sheet.get());
}

TEST(EncodeCSSParseToken, TestSplitSelector0) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string select = ".a";
  tokens->SplitSelector(select);
  EXPECT_EQ(tokens->sheets_.size(), 1);
  EXPECT_EQ(tokens->sheets_[0]->name_.str(), "a");
  EXPECT_EQ(tokens->sheets_[0]->GetType(),
            tasm::CSSSheet::SheetType::CLASS_SELECT);
}

TEST(EncodeCSSParseToken, TestSplitSelector1) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string select = "#a";
  tokens->SplitSelector(select);
  EXPECT_EQ(tokens->sheets_.size(), 1);
  EXPECT_EQ(tokens->sheets_[0]->name_.str(), "a");
  EXPECT_EQ(tokens->sheets_[0]->GetType(),
            tasm::CSSSheet::SheetType::ID_SELECT);
}

TEST(EncodeCSSParseToken, TestSplitSelector2) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string select = ".a .b";
  tokens->SplitSelector(select);
  EXPECT_EQ(tokens->sheets_.size(), 2);
  EXPECT_EQ(tokens->sheets_[0]->name_.str(), "a");
  EXPECT_EQ(tokens->sheets_[1]->name_.str(), "b");
  EXPECT_EQ(tokens->sheets_[0]->GetType(),
            tasm::CSSSheet::SheetType::CLASS_SELECT);
  EXPECT_EQ(tokens->sheets_[1]->GetType(),
            tasm::CSSSheet::SheetType::CLASS_SELECT);
}

TEST(EncodeCSSParseToken, TestSplitSelector3) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string select = ".a .b .c  .d";
  tokens->SplitSelector(select);
  EXPECT_EQ(tokens->sheets_.size(), 4);
  EXPECT_EQ(tokens->sheets_[0]->name_.str(), "a");
  EXPECT_EQ(tokens->sheets_[1]->name_.str(), "b");
  EXPECT_EQ(tokens->sheets_[2]->name_.str(), "c");
  EXPECT_EQ(tokens->sheets_[3]->name_.str(), "d");
  EXPECT_EQ(tokens->sheets_[0]->GetType(),
            tasm::CSSSheet::SheetType::CLASS_SELECT);
  EXPECT_EQ(tokens->sheets_[1]->GetType(),
            tasm::CSSSheet::SheetType::CLASS_SELECT);
  EXPECT_EQ(tokens->sheets_[2]->GetType(),
            tasm::CSSSheet::SheetType::CLASS_SELECT);
  EXPECT_EQ(tokens->sheets_[3]->GetType(),
            tasm::CSSSheet::SheetType::CLASS_SELECT);
}

TEST(EncodeCSSParseToken, TestSplitSelector4) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string select = ":not";
  tokens->SplitSelector(select);
  EXPECT_EQ(tokens->sheets_.size(), 0);
}

TEST(EncodeCSSParseToken, TestSplitSelector5) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string select = ":not(.a)x";
  tokens->SplitSelector(select);
  EXPECT_EQ(tokens->sheets_.size(), 0);
}

TEST(EncodeCSSParseToken, TestSplitSelector6) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string select = ":notX(.a)";
  tokens->SplitSelector(select);
  EXPECT_EQ(tokens->sheets_.size(), 0);
}

TEST(EncodeCSSParseToken, TestSplitSelector7) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string select = ".a:not(.a)";
  tokens->SplitSelector(select);
  EXPECT_EQ(tokens->sheets_.size(), 0);
}

TEST(EncodeCSSParseToken, TestSplitSelector8) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string select = ".a:not(.b)";
  tokens->SplitSelector(select);
  EXPECT_EQ(tokens->sheets_.size(), 1);
  EXPECT_EQ(tokens->sheets_[0]->GetType(),
            tasm::CSSSheet::SheetType::NOT_SELECT |
                tasm::CSSSheet::SheetType::CLASS_SELECT);
}

TEST(EncodeCSSParseToken, TestParseStyleVariables0) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string json_input = R"({
      "--key1" : "value1",
      "--key2" : "value2"
    })";

  auto json = base::strToJson(json_input.c_str());

  tokens->ParseStyleVariables(json);
  EXPECT_EQ(tokens->style_variables_.size(), 2);
  EXPECT_EQ(tokens->style_variables_["--key1"], "value1");
  EXPECT_EQ(tokens->style_variables_["--key2"], "value2");
}

TEST(EncodeCSSParseToken, TestParseStyleVariables1) {
  tasm::CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<encoder::CSSParseToken>(parser_configs);

  std::string json_input = R"([{
      "--key1" : "value1",
      "--key2" : "value2"
    }])";

  auto json = base::strToJson(json_input.c_str());

  tokens->ParseStyleVariables(json);
  EXPECT_EQ(tokens->style_variables_.size(), 0);
}

}  // namespace encoder
}  // namespace lynx
