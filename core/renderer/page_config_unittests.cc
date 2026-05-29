// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define private public
#define protected public

#include "core/template_bundle/template_codec/binary_decoder/lynx_config_decoder.h"
#include "core/template_bundle/template_codec/binary_decoder/page_config.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace test {

/**
 * 1. if target config has not been set,
 * GetValue() shoule be equal to expect_default_value;
 * 2. if target config has been set to true,
 * GetValue() shoule be equal to expect_true_value;
 * 3. if target config has been set to false,
 * GetValue() shoule be equal to expect_false_value.
 */
#define CHECK_CONFIG_VALUE(func_name, expect_default_value, expect_true_value, \
                           expect_false_value)                                 \
  PageConfig page_config;                                                      \
  EXPECT_EQ(expect_default_value, page_config.Get##func_name());               \
  page_config.Set##func_name(TernaryBool::TRUE_VALUE);                         \
  EXPECT_EQ(expect_true_value, page_config.Get##func_name());                  \
  page_config.Set##func_name(TernaryBool::FALSE_VALUE);                        \
  EXPECT_EQ(expect_false_value, page_config.Get##func_name());

TEST(PageConfigTest, EnableParallelParseElementTemplate) {
  PageConfig page_config;
  EXPECT_FALSE(page_config.GetEnableParallelParseElementTemplate());

  page_config.pipeline_scheduler_config_ = 1;
  EXPECT_TRUE(page_config.GetEnableParallelParseElementTemplate());
}

TEST(PageConfigTest, EnableUseContextPool) {
  CHECK_CONFIG_VALUE(EnableUseContextPool, true, true, false);
}

TEST(PageConfigTest, EnableFrameNativeData) {
  auto& env = LynxEnv::GetInstance();
  env.external_env_map_.erase(LynxEnv::Key::ENABLE_FRAME_NATIVE_DATA);

  rapidjson::Document empty_doc;
  empty_doc.Parse("{}");
  std::shared_ptr<PageConfig> default_config = std::make_shared<PageConfig>();
  LynxConfigDecoder::DecodePageConfig(default_config, empty_doc, "");
  EXPECT_FALSE(default_config->GetEnableFrameNativeData());

  env.external_env_map_[LynxEnv::Key::ENABLE_FRAME_NATIVE_DATA] = "true";
  std::shared_ptr<PageConfig> settings_config = std::make_shared<PageConfig>();
  LynxConfigDecoder::DecodePageConfig(settings_config, empty_doc, "");
  EXPECT_TRUE(settings_config->GetEnableFrameNativeData());

  rapidjson::Document explicit_false_doc;
  explicit_false_doc.Parse("{\"enableFrameNativeData\": false}");
  std::shared_ptr<PageConfig> explicit_false_config =
      std::make_shared<PageConfig>();
  LynxConfigDecoder::DecodePageConfig(explicit_false_config, explicit_false_doc,
                                      "");
  EXPECT_FALSE(explicit_false_config->GetEnableFrameNativeData());

  env.external_env_map_.erase(LynxEnv::Key::ENABLE_FRAME_NATIVE_DATA);
}

TEST(PageConfigTest, EnableElementApiNewRegistration) {
  auto& env = LynxEnv::GetInstance();
  env.external_env_map_.erase(
      LynxEnv::Key::ENABLE_ELEMENT_API_NEW_REGISTRATION);

  rapidjson::Document empty_doc;
  empty_doc.Parse("{}");
  std::shared_ptr<PageConfig> default_config = std::make_shared<PageConfig>();
  LynxConfigDecoder::DecodePageConfig(default_config, empty_doc, "");
  EXPECT_FALSE(default_config->GetEnableElementApiNewRegistration());

  env.external_env_map_[LynxEnv::Key::ENABLE_ELEMENT_API_NEW_REGISTRATION] =
      "true";
  std::shared_ptr<PageConfig> settings_config = std::make_shared<PageConfig>();
  LynxConfigDecoder::DecodePageConfig(settings_config, empty_doc, "");
  EXPECT_TRUE(settings_config->GetEnableElementApiNewRegistration());

  rapidjson::Document explicit_false_doc;
  explicit_false_doc.Parse("{\"enableElementApiNewRegistration\": false}");
  std::shared_ptr<PageConfig> explicit_false_config =
      std::make_shared<PageConfig>();
  LynxConfigDecoder::DecodePageConfig(explicit_false_config, explicit_false_doc,
                                      "");
  EXPECT_FALSE(explicit_false_config->GetEnableElementApiNewRegistration());

  env.external_env_map_.erase(
      LynxEnv::Key::ENABLE_ELEMENT_API_NEW_REGISTRATION);
}

TEST(PageConfigTest, EnableEventTargetInfoNodeIndex) {
  std::shared_ptr<PageConfig> page_config = std::make_shared<PageConfig>();
  EXPECT_FALSE(page_config->GetEnableEventTargetInfoNodeIndex());

  rapidjson::Document doc;
  doc.Parse("{\n  \"enableEventTargetInfoNodeIndex\" : true\n}");
  LynxConfigDecoder::DecodePageConfig(page_config, doc, "");
  EXPECT_TRUE(page_config->GetEnableEventTargetInfoNodeIndex());
}

TEST(PageConfigTest, EnableComponentAsyncDecode) {
  CHECK_CONFIG_VALUE(EnableComponentAsyncDecode, false, true, false);
}

TEST(PageConfigTest, EnableSignalAPI0) {
  PageConfig page_config;
  EXPECT_EQ(page_config.GetEnableSignalAPIBoolValue(), false);
  EXPECT_EQ(page_config.GetEnableSignalAPI(), TernaryBool::UNDEFINE_VALUE);

  page_config.DecodePageConfigFromJsonStringWhileUndefined(
      "{\n  \"enableSignalAPI\" : true\n}");
  EXPECT_EQ(page_config.GetEnableSignalAPIBoolValue(), true);
  EXPECT_EQ(page_config.GetEnableSignalAPI(), TernaryBool::TRUE_VALUE);
}

TEST(PageConfigTest, EnableSignalAPI1) {
  PageConfig page_config;
  EXPECT_EQ(page_config.GetEnableSignalAPIBoolValue(), false);
  EXPECT_EQ(page_config.GetEnableSignalAPI(), TernaryBool::UNDEFINE_VALUE);

  page_config.DecodePageConfigFromJsonStringWhileUndefined(
      "{\n  \"enableSignalAPI\" : false\n}");
  EXPECT_EQ(page_config.GetEnableSignalAPIBoolValue(), false);
  EXPECT_EQ(page_config.GetEnableSignalAPI(), TernaryBool::FALSE_VALUE);
}

TEST(PageConfigTest, GetEnableParallelElement) {
  PageConfig page_config;
  page_config.pipeline_scheduler_config_ = 0;
  page_config.enable_parallel_element_ = false;
  EXPECT_EQ(page_config.GetEnableParallelElement(), false);

  page_config.pipeline_scheduler_config_ = 0;
  page_config.enable_parallel_element_ = true;
  EXPECT_EQ(page_config.GetEnableParallelElement(), true);

  page_config.pipeline_scheduler_config_ = 0;
  page_config.enable_parallel_element_ = false;
  page_config.enable_level_order_traversing_ = TernaryBool::TRUE_VALUE;
  EXPECT_EQ(page_config.GetEnableParallelElement(), true);

  page_config.pipeline_scheduler_config_ = 0;
  page_config.enable_parallel_element_ = true;
  page_config.enable_level_order_traversing_ = TernaryBool::FALSE_VALUE;
  EXPECT_EQ(page_config.GetEnableParallelElement(), true);

  page_config.pipeline_scheduler_config_ = kEnableParallelElementMask;
  page_config.enable_parallel_element_ = false;
  EXPECT_EQ(page_config.GetEnableParallelElement(), true);

  page_config.pipeline_scheduler_config_ = kEnableParallelElementMask;
  page_config.enable_parallel_element_ = true;
  EXPECT_EQ(page_config.GetEnableParallelElement(), true);

  page_config.pipeline_scheduler_config_ = kDisableParallelElementMask;
  page_config.enable_parallel_element_ = true;
  EXPECT_EQ(page_config.GetEnableParallelElement(), false);
}

TEST(PageConfigTest, GetEnableLevelOrderTraversing) {
  PageConfig page_config;
  EXPECT_EQ(page_config.GetEnableLevelOrderTraversing(), false);

  page_config.enable_level_order_traversing_ = TernaryBool::UNDEFINE_VALUE;
  page_config.pipeline_scheduler_config_ = kEnableParallelElementLevelOrderMask;
  EXPECT_EQ(page_config.GetEnableLevelOrderTraversing(), true);

  page_config.enable_level_order_traversing_ = TernaryBool::UNDEFINE_VALUE;
  page_config.pipeline_scheduler_config_ =
      kDisableParallelElementLevelOrderMask;
  EXPECT_EQ(page_config.GetEnableLevelOrderTraversing(), false);

  LynxEnv::GetInstance()
      .external_env_map_[LynxEnv::Key::ENABLE_LEVEL_ORDER_TRAVERSING] = "true";
  page_config.enable_level_order_traversing_ = TernaryBool::UNDEFINE_VALUE;
  page_config.pipeline_scheduler_config_ =
      kDisableParallelElementLevelOrderMask;
  EXPECT_EQ(page_config.GetEnableLevelOrderTraversing(), false);

  page_config.enable_level_order_traversing_ = TernaryBool::UNDEFINE_VALUE;
  page_config.pipeline_scheduler_config_ = 0;
  EXPECT_EQ(page_config.GetEnableLevelOrderTraversing(), true);

  LynxEnv::GetInstance()
      .external_env_map_[LynxEnv::Key::ENABLE_LEVEL_ORDER_TRAVERSING] = "false";
  page_config.enable_level_order_traversing_ = TernaryBool::UNDEFINE_VALUE;
  page_config.pipeline_scheduler_config_ = 0;
  EXPECT_EQ(page_config.GetEnableLevelOrderTraversing(), false);
}

TEST(PageConfigTest, EnableNativeScheduleCreateViewAsync) {
  PageConfig page_config;
  EXPECT_EQ(page_config.GetEnableNativeScheduleCreateViewAsyncAsBool(), false);
  EXPECT_EQ(page_config.GetEnableNativeScheduleCreateViewAsync(),
            TernaryBool::UNDEFINE_VALUE);

  page_config.DecodePageConfigFromJsonStringWhileUndefined(
      "{\n  \"enableNativeScheduleCreateViewAsync\" : false\n}");
  EXPECT_EQ(page_config.GetEnableNativeScheduleCreateViewAsyncAsBool(), false);
  EXPECT_EQ(page_config.GetEnableNativeScheduleCreateViewAsync(),
            TernaryBool::FALSE_VALUE);
}

TEST(PageConfigTest, EnableUnifiedPipeline) {
  PageConfig page_config;
  EXPECT_EQ(page_config.GetEnableUnifiedPipeline(),
            TernaryBool::UNDEFINE_VALUE);

  page_config.DecodePageConfigFromJsonStringWhileUndefined(
      "{\n  \"enableNativeScheduleCreateViewAsync\" : false\n}");
  EXPECT_EQ(page_config.GetEnableUnifiedPipeline(),
            TernaryBool::UNDEFINE_VALUE);

  PageConfig page_config_0;
  page_config_0.DecodePageConfigFromJsonStringWhileUndefined(
      "{\n  \"enableUnifiedPipeline\" : true\n}");
  EXPECT_EQ(page_config_0.GetEnableUnifiedPipeline(), TernaryBool::TRUE_VALUE);

  PageConfig page_config_1;
  page_config_1.DecodePageConfigFromJsonStringWhileUndefined(
      "{\n  \"enableUnifiedPipeline\" : false\n}");
  EXPECT_EQ(page_config_1.GetEnableUnifiedPipeline(), TernaryBool::FALSE_VALUE);
}

TEST(PageConfigTest, EnableParseIntFlex) {
  PageConfig page_config;
  EXPECT_FALSE(page_config.GetEnableParseIntFlex());
  EXPECT_FALSE(page_config.GetCSSParserConfigs().enable_parse_int_flex);

  page_config.DecodePageConfigFromJsonStringWhileUndefined(
      "{\n  \"enableParseIntFlex\" : true\n}");
  EXPECT_FALSE(page_config.GetEnableParseIntFlex());
  EXPECT_FALSE(page_config.GetCSSParserConfigs().enable_parse_int_flex);
}

#undef CHECK_CONFIG_VALUE

}  // namespace test
}  // namespace tasm
}  // namespace lynx
