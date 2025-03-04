// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/replay/lynx_replay_helper.h"

#include "core/runtime/jsi/jsi.h"
#include "testing/utils/make_js_runtime.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace replay {

TEST(LynxReplayHelper, convertRapidJsonNumberToJSIValue) {
  auto runtime = testing::utils::makeJSRuntime();
  rapidjson::Document number_value(rapidjson::kNumberType);
  number_value.SetInt(1);
  auto number_piper = piper::ReplayHelper::convertRapidJsonNumberToJSIValue(
      *runtime, number_value);
  ASSERT_TRUE(number_piper.kind() == piper::Value::ValueKind::NumberKind);
  EXPECT_FLOAT_EQ(1, number_piper.getNumber());

  number_value.SetDouble(1.45);
  number_piper = piper::ReplayHelper::convertRapidJsonNumberToJSIValue(
      *runtime, number_value);
  ASSERT_TRUE(number_piper.kind() == piper::Value::ValueKind::NumberKind);
  EXPECT_FLOAT_EQ(1.45, number_piper.getNumber());

  number_value.SetUint(1);
  number_piper = piper::ReplayHelper::convertRapidJsonNumberToJSIValue(
      *runtime, number_value);
  ASSERT_TRUE(number_piper.kind() == piper::Value::ValueKind::NumberKind);
  EXPECT_FLOAT_EQ(1, number_piper.getNumber());

  rapidjson::Document string_value(rapidjson::kStringType);
  string_value.SetString("testing");
  number_piper = piper::ReplayHelper::convertRapidJsonNumberToJSIValue(
      *runtime, string_value);
  ASSERT_TRUE(number_piper.kind() == piper::Value::ValueKind::NumberKind);
  EXPECT_FLOAT_EQ(0, number_piper.getNumber());
}

TEST(LynxReplayHelper, convertRapidJsonStringToJSIValue) {
  auto runtime = testing::utils::makeJSRuntime();

  rapidjson::Document string_value(rapidjson::kStringType);
  string_value.SetString("hello world!");
  auto string_piper = piper::ReplayHelper::convertRapidJsonStringToJSIValue(
      *runtime, string_value);
  ASSERT_TRUE(string_piper.kind() == piper::Value::ValueKind::StringKind);
  EXPECT_STREQ("hello world!",
               string_piper.getString(*runtime).utf8(*runtime).c_str());

  string_value.SetString("");
  string_piper = piper::ReplayHelper::convertRapidJsonStringToJSIValue(
      *runtime, string_value);
  ASSERT_TRUE(string_piper.kind() == piper::Value::ValueKind::StringKind);
  EXPECT_STREQ("", string_piper.getString(*runtime).utf8(*runtime).c_str());

  string_value.SetString("NaN");
  string_piper = piper::ReplayHelper::convertRapidJsonStringToJSIValue(
      *runtime, string_value);
  ASSERT_TRUE(string_piper.kind() == piper::Value::ValueKind::NumberKind);
  ASSERT_TRUE(isnan(string_piper.getNumber()));
}

TEST(LynxReplayHelper, convertRapidJsonObjectToJSIValue) {
  auto runtime = testing::utils::makeJSRuntime();
  rapidjson::Document doc;
  const std::string lynx_val_content = R"(
    {
        "lynx_val":{
            "__lynx_val__":"1234567",
            "toString":"function",
            "toJSON":"function"
        },
        "obj":{
            "name":"testbench",
            "enable":false
        },
        "index":1234,
        "title":"This is a test for testbench"
    }
  )";
  doc.Parse(lynx_val_content);
  /*
    {
        "lynx_val":"1234567",
        "obj":{
            "name":"testbench",
            "enable":false
        },
        "index":1234,
        "title":"This is a test for testbench"
    }
  */
  auto result =
      piper::ReplayHelper::convertRapidJsonObjectToJSIValue(*runtime, doc);
  ASSERT_TRUE(result.isObject());

  {  // check lynx_val field
    auto lynx_val_value =
        result.asObject(*runtime)->getProperty(*runtime, "lynx_val");
    ASSERT_TRUE(lynx_val_value.has_value());
    ASSERT_TRUE(lynx_val_value->isString());
    EXPECT_STREQ("1234567",
                 lynx_val_value->asString(*runtime)->utf8(*runtime).c_str());
  }

  {  // check obj field
    auto obj_value = result.asObject(*runtime)->getProperty(*runtime, "obj");
    ASSERT_TRUE(obj_value.has_value());
    ASSERT_TRUE(obj_value->isObject());
    auto name_value =
        obj_value->asObject(*runtime)->getProperty(*runtime, "name");
    ASSERT_TRUE(name_value.has_value());
    ASSERT_TRUE(name_value->isString());
    EXPECT_STREQ("testbench",
                 name_value->asString(*runtime)->utf8(*runtime).c_str());
    auto enable_value =
        obj_value->asObject(*runtime)->getProperty(*runtime, "enable");
    ASSERT_TRUE(enable_value.has_value());
    ASSERT_TRUE(enable_value->isBool());
    ASSERT_FALSE(enable_value->getBool());
  }
  {  // check index field

    auto index_value =
        result.asObject(*runtime)->getProperty(*runtime, "index");
    ASSERT_TRUE(index_value.has_value());
    ASSERT_TRUE(index_value->isNumber());
    EXPECT_EQ(1234, index_value->asNumber(*runtime).value());
  }

  {  // check title field
    auto title_value =
        result.asObject(*runtime)->getProperty(*runtime, "title");
    ASSERT_TRUE(title_value.has_value());
    ASSERT_TRUE(title_value->isString());
    EXPECT_STREQ("This is a test for testbench",
                 title_value->asString(*runtime)->utf8(*runtime).c_str());
  }
}

}  // namespace replay
}  // namespace tasm
}  // namespace lynx
