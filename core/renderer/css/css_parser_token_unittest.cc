// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/renderer/css/css_parser_token.h"

#include <tuple>

#include "core/base/json/json_util.h"
#include "core/base/thread/once_task.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "core/renderer/starlight/style/css_type.h"
#include "core/runtime/vm/lepus/table.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"
#include "third_party/rapidjson/document.h"

namespace lynx {
namespace tasm {

TEST(CSSParseToken, GetAttributes) {
  CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<CSSParseToken>(parser_configs);
  tokens->raw_attributes().insert_or_assign(CSSPropertyID::kPropertyIDFontSize,
                                            CSSValue(lepus::Value("18px")));

  EXPECT_FALSE(tokens->raw_attributes().empty());

  const auto& attrs = tokens->GetAttributes();

  StyleMap expected;
  UnitHandler::Process(CSSPropertyID::kPropertyIDFontSize, lepus::Value("18px"),
                       expected, parser_configs);
  EXPECT_EQ(attrs.find(CSSPropertyID::kPropertyIDFontSize)->second,
            expected.find(CSSPropertyID::kPropertyIDFontSize)->second);
}

TEST(CSSParseToken, ParallelGetAttributes) {
  CSSParserConfigs parser_configs;
  StyleMap expected;
  UnitHandler::Process(CSSPropertyID::kPropertyIDFontSize, lepus::Value("18px"),
                       expected, parser_configs);
  UnitHandler::Process(CSSPropertyID::kPropertyIDHeight, lepus::Value("18px"),
                       expected, parser_configs);
  UnitHandler::Process(CSSPropertyID::kPropertyIDWidth, lepus::Value("18px"),
                       expected, parser_configs);

  auto tokens = std::make_shared<CSSParseToken>(parser_configs);
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

TEST(CSSParseToken, ParallelProcessAttributes) {
  CSSParserConfigs parser_configs;
  StyleMap expected;
  UnitHandler::Process(CSSPropertyID::kPropertyIDFontSize, lepus::Value("18px"),
                       expected, parser_configs);
  CSSValue font_size_expected_value =
      expected.find(CSSPropertyID::kPropertyIDFontSize)->second;
  UnitHandler::Process(CSSPropertyID::kPropertyIDHeight, lepus::Value("18px"),
                       expected, parser_configs);
  CSSValue height_expected_value =
      expected.find(CSSPropertyID::kPropertyIDHeight)->second;
  UnitHandler::Process(CSSPropertyID::kPropertyIDWidth, lepus::Value("18px"),
                       expected, parser_configs);
  CSSValue width_expected_value =
      expected.find(CSSPropertyID::kPropertyIDWidth)->second;

  RawStyleMap raw_attributes;
  raw_attributes.insert_or_assign(CSSPropertyID::kPropertyIDFontSize,
                                  CSSValue(lepus::Value("18px")));
  raw_attributes.insert_or_assign(CSSPropertyID::kPropertyIDHeight,
                                  CSSValue(lepus::Value("18px")));
  raw_attributes.insert_or_assign(CSSPropertyID::kPropertyIDWidth,
                                  CSSValue(lepus::Value("18px")));

  auto tokens = std::make_shared<CSSParseToken>(parser_configs);
  tokens->raw_attributes_ = std::move(raw_attributes);

  using TestOnceTaskRefptr = base::OnceTaskRefptr<std::tuple<bool, bool, bool>>;
  std::vector<TestOnceTaskRefptr> task_vec{};

  for (int i = 0; i < 10000; ++i) {
    std::promise<std::tuple<bool, bool, bool>> promise;
    std::future<std::tuple<bool, bool, bool>> future = promise.get_future();

    auto task =
        fml::MakeRefCounted<base::OnceTask<std::tuple<bool, bool, bool>>>(
            [tokens, font_size_expected_value, width_expected_value,
             height_expected_value, promise = std::move(promise)]() mutable {
              const auto& attributes = tokens->GetAttributes();

              promise.set_value(std::make_tuple(
                  attributes.find(CSSPropertyID::kPropertyIDFontSize)->second ==
                      font_size_expected_value,
                  attributes.find(CSSPropertyID::kPropertyIDHeight)->second ==
                      height_expected_value,
                  attributes.find(CSSPropertyID::kPropertyIDWidth)->second ==
                      width_expected_value));
            },
            std::move(future));

    base::TaskRunnerManufactor::PostTaskToConcurrentLoop(
        [task]() { task->Run(); }, base::ConcurrentTaskType::HIGH_PRIORITY);

    task_vec.emplace_back(std::move(task));
  }

  for (auto item : task_vec) {
    auto compare_result = item->GetFuture().get();
    EXPECT_TRUE(std::get<0>(compare_result));
    EXPECT_TRUE(std::get<1>(compare_result));
    EXPECT_TRUE(std::get<2>(compare_result));
  }
}

TEST(CSSParseToken, TestTouchPseudoToken) {
  CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<CSSParseToken>(parser_configs);

  tokens->MarkAsTouchPseudoToken();
  EXPECT_TRUE(tokens->IsTouchPseudoToken());
}

TEST(CSSParseToken, TestGetStyleTokenTypeCornerCase) {
  CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<CSSParseToken>(parser_configs);
  EXPECT_EQ(tokens->GetStyleTokenType(), 0);

  tokens->sheets_.push_back(nullptr);
  EXPECT_EQ(tokens->GetStyleTokenType(), 0);
}

TEST(CSSParseToken, TestIsPseudoStyleTokenCornerCase) {
  CSSParserConfigs parser_configs;
  auto tokens = std::make_shared<CSSParseToken>(parser_configs);
  tokens->sheets_.push_back(nullptr);

  EXPECT_FALSE(tokens->IsPseudoStyleToken());
}
}  // namespace tasm
}  // namespace lynx
