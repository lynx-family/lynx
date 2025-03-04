// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <cstdint>
#include <memory>
#include <vector>

#include "core/renderer/data/template_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "third_party/rapidjson/document.h"
#define private public
#include "core/services/recorder/template_assembler_recorder.h"
#undef private

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace recorder {

lepus::Value convertRapidJsonStringToLepusValue(rapidjson::Value& value) {
  return lepus::Value(value.GetString());
}

lepus::Value convertRapidJsonNumberToLepusValue(rapidjson::Value& value) {
  double v = 0;
  if (value.IsInt()) {
    v = value.GetInt();
  } else if (value.IsFloat()) {
    v = value.GetDouble();
  } else if (value.IsDouble()) {
    v = value.GetDouble();
  } else if (value.IsInt64()) {
    v = value.GetInt64();
  } else if (value.IsUint()) {
    v = value.GetUint64();
  } else if (value.IsUint64()) {
    v = value.GetUint64();
  }
  return lepus::Value(v);
}

lepus::Value convertLepusValue(rapidjson::Value& value) {
  switch (value.GetType()) {
    case rapidjson::kStringType:
      return convertRapidJsonStringToLepusValue(value);
    case rapidjson::kNumberType:
      return convertRapidJsonNumberToLepusValue(value);
    case rapidjson::kNullType:
      return lepus::Value(lepus::Value_Nil);
    case rapidjson::kFalseType:
      return lepus::Value(false);
    case rapidjson::kTrueType:
      return lepus::Value(true);
    case rapidjson::kArrayType: {
      int length = value.Size();
      lepus::Value arr = lepus::Value(lepus::Value_Array);
      for (int index = 0; index < length; index++) {
        arr.Array()->push_back(convertLepusValue(value[index]));
      }
      return arr;
    }
    case rapidjson::kObjectType: {
      lepus::Value object = lepus::Value::CreateObject();
      for (auto p = value.GetObject().begin(); p != value.GetObject().end();
           p++) {
        const char* key = (*p).name.GetString();
        object.Table()->SetValue(key, convertLepusValue((*p).value));
      }
      return object;
    }
    default:
      return lepus::Value(lepus::Value_Undefined);
  }
}

lepus::Value buildLepusValueFromString(const std::string str) {
  rapidjson::Document json;
  json.Parse(str);
  return convertLepusValue(json);
}

TEST(TemplateAssemblerRecorder, ProcessUpdatePageOption) {
  rapidjson::Value params_page_option(rapidjson::kObjectType);
  UpdatePageOption option;
  TemplateAssemblerRecorder::ProcessUpdatePageOption(option,
                                                     params_page_option);
  ASSERT_TRUE(params_page_option["from_native"].GetBool());
  ASSERT_FALSE(params_page_option["reset_page_data"].GetBool());
  ASSERT_FALSE(params_page_option["update_first_time"].GetBool());
  ASSERT_FALSE(params_page_option["reload_template"].GetBool());
  ASSERT_FALSE(params_page_option["global_props_changed"].GetBool());
  ASSERT_FALSE(params_page_option["reload_from_js"].GetBool());
  EXPECT_EQ(params_page_option["native_update_data_order_"], 0);
}

TEST(TemplateAssemblerRecorder, CreateJSONFromTemplateData) {
  std::string template_data_value_string = R"({
      "name":"testbench",
      "forTesting":true
    }
    )";
  lepus::Value data = buildLepusValueFromString(template_data_value_string);
  std::shared_ptr<TemplateData> template_data =
      std::make_shared<TemplateData>();
  template_data->SetValue(data);
  template_data->SetPreprocessorName("test_preprocess_name");
  template_data->SetReadOnly(true);
  rapidjson::Value result =
      TemplateAssemblerRecorder::CreateJSONFromTemplateData(template_data);
  /*
  {
    "value":{
      "name" : "testbench",
      "forTesting" : true
    },
    "preprocessorName": "test_preprocess_name",
    "readOnly": true
  }
  */
  EXPECT_TRUE(result.GetObject().MemberCount() == 3);
  EXPECT_STREQ(result["preprocessorName"].GetString(), "test_preprocess_name");
  EXPECT_TRUE(result["readOnly"].GetBool());

  rapidjson::Value& value = result["value"];
  EXPECT_TRUE(value.GetObject().MemberCount() == 2);
  EXPECT_STREQ(value["name"].GetString(), "testbench");
  EXPECT_TRUE(value["forTesting"].GetBool());
}

TEST(TemplateAssemblerRecorder, CreateJSONFromGlobalProps) {
  std::string global_props_string = R"({
      "name":"testbench",
      "forTesting":true
    }
    )";
  lepus::Value data = buildLepusValueFromString(global_props_string);
  rapidjson::Value result =
      TemplateAssemblerRecorder::CreateJSONFromGlobalProps(data);
  /*
  {
    "name" : "testbench",
    "forTesting" : true
  }
  */
  EXPECT_TRUE(result.GetObject().MemberCount() == 2);
  EXPECT_STREQ(result["name"].GetString(), "testbench");
  EXPECT_TRUE(result["forTesting"].GetBool());
}

TEST(TemplateAssemblerRecorder, CreateJSONFromUpdateConfig) {
  std::string config_string = R"({
      "name":"testbench",
      "forTesting":true
    }
    )";
  lepus::Value config = buildLepusValueFromString(config_string);
  bool notice_delegate = true;
  rapidjson::Value result =
      TemplateAssemblerRecorder::CreateJSONFromUpdateConfig(config,
                                                            notice_delegate);
  /*
  "config":{
    "name" : "testbench",
    "forTesting" : true
  },
  "noticeDelegate":true
  */
  EXPECT_TRUE(result.GetObject().MemberCount() == 2);
  // EXPECT_STREQ(result["name"].GetString(), "testbench");
  EXPECT_TRUE(result["noticeDelegate"].GetBool());
  EXPECT_TRUE(result["config"].GetObject().MemberCount() == 2);

  EXPECT_STREQ(result["config"]["name"].GetString(), "testbench");
  EXPECT_TRUE(result["config"]["forTesting"].GetBool());
}

TEST(TemplateAssemblerRecorder, CreateJSONFromUpdateDataByPreParsedData) {
  std::string template_data_value_string = R"({
      "name":"testbench",
      "forTesting":true
    }
    )";
  lepus::Value data = buildLepusValueFromString(template_data_value_string);
  std::shared_ptr<TemplateData> template_data =
      std::make_shared<TemplateData>();
  template_data->SetValue(data);
  template_data->SetPreprocessorName("test_preprocess_name");
  template_data->SetReadOnly(true);
  UpdatePageOption option;
  rapidjson::Value result =
      TemplateAssemblerRecorder::CreateJSONFromUpdateDataByPreParsedData(
          template_data, option);
  /*
  {
    "value":{
      "name":"testbench",
      "forTesting":true
    },
    "readOnly":true,
    "preprocessorName":"test_preprocess_name",
    "updatePageOption":{
        "from_native":true,
        "reset_page_data":false,
        "update_first_time":false,
        "reload_template":false;
        "global_props_changed":false,
        "reload_from_js":false,
        "native_update_data_order_":0
    }
  }
  */
  EXPECT_TRUE(result.GetObject().MemberCount() == 4);
  EXPECT_TRUE(result["value"].GetObject().MemberCount() == 2);
  EXPECT_TRUE(result["readOnly"].GetBool());
  EXPECT_STREQ(result["value"]["name"].GetString(), "testbench");
  EXPECT_TRUE(result["value"]["forTesting"].GetBool());
  EXPECT_STREQ(result["preprocessorName"].GetString(), "test_preprocess_name");
  EXPECT_TRUE(result["updatePageOption"].GetObject().MemberCount() == 7);
  EXPECT_TRUE(result["updatePageOption"]["from_native"].GetBool());
  EXPECT_FALSE(result["updatePageOption"]["reset_page_data"].GetBool());
  EXPECT_FALSE(result["updatePageOption"]["update_first_time"].GetBool());
  EXPECT_FALSE(result["updatePageOption"]["reload_template"].GetBool());
  EXPECT_FALSE(result["updatePageOption"]["global_props_changed"].GetBool());
  EXPECT_FALSE(result["updatePageOption"]["reload_from_js"].GetBool());
  EXPECT_FLOAT_EQ(
      result["updatePageOption"]["native_update_data_order_"].GetFloat(), 0);
}

TEST(TemplateAssemblerRecorder, CreateJSONFromTouchEvent) {
  rapidjson::Value result = TemplateAssemblerRecorder::CreateJSONFromTouchEvent(
      "testbench", 1, 2, 3, 4, 5, 6, 7, 8);
  /*
  {
    "name": "testbench",
    "tag":1,
    "root_tag":2,
    "x":3,
    "y":4,
    "client_x":5,
    "client_y":6,
    "page_x":7,
    "page_y":8
  }
  */
  EXPECT_TRUE(result.GetObject().MemberCount() == 9);
  EXPECT_STREQ(result["name"].GetString(), "testbench");
  EXPECT_EQ(result["tag"].GetInt(), 1);
  EXPECT_FLOAT_EQ(result["root_tag"].GetFloat(), 2.0);
  EXPECT_FLOAT_EQ(result["x"].GetFloat(), 3.0);
  EXPECT_FLOAT_EQ(result["y"].GetFloat(), 4.0);
  EXPECT_FLOAT_EQ(result["client_x"].GetFloat(), 5.0);
  EXPECT_FLOAT_EQ(result["client_y"].GetFloat(), 6.0);
  EXPECT_FLOAT_EQ(result["page_x"].GetFloat(), 7.0);
  EXPECT_FLOAT_EQ(result["page_y"].GetFloat(), 8.0);
}

TEST(TemplateAssemblerRecorder, CreateJSONFromCustomEvent) {
  std::string name = "testbench";
  std::string pname = "testbench_pname";
  int tag = 1, root_tag = 2;
  std::string params_string = R"({
      "from":"testbench",
      "enable":true
    }
    )";
  lepus::Value params = buildLepusValueFromString(params_string);
  rapidjson::Value result =
      TemplateAssemblerRecorder::CreateJSONFromCustomEvent(name, tag, root_tag,
                                                           params, pname);
  /*
  {
    "tag":1,
    "root_tag":2,
    "name": "testbench",
    "pname": "testbench_pname",
    "params":{
      "from":"testbench",
      "enable":true
    }
  }
  */
  EXPECT_TRUE(result.GetObject().MemberCount() == 5);
  EXPECT_STREQ(result["name"].GetString(), "testbench");
  EXPECT_STREQ(result["pname"].GetString(), "testbench_pname");
  EXPECT_EQ(result["tag"].GetInt(), 1);
  EXPECT_EQ(result["root_tag"].GetInt(), 2);

  EXPECT_TRUE(result["params"].GetObject().MemberCount() == 2);
  EXPECT_STREQ(result["params"]["from"].GetString(), "testbench");
  EXPECT_TRUE(result["params"]["enable"].GetBool());
}

TEST(TemplateAssemblerRecorder, CreateJSONFromRequireTemplate) {
  std::string url = "http://testbench";
  bool sync_tag = true;
  rapidjson::Value result =
      TemplateAssemblerRecorder::CreateJSONFromRequireTemplate(url, sync_tag);
  /*
  {
    "url":"http://testbench",
    "sync_tag":true
  }
  */
  EXPECT_TRUE(result.GetObject().MemberCount() == 2);
  EXPECT_STREQ(result["url"].GetString(), url.c_str());
  EXPECT_TRUE(result["sync_tag"].GetBool());
}

TEST(TemplateAssemblerRecorder, CreateJSONFromLoadComponentWithCallback) {
  std::string url = "http://testbench";
  std::string encoded_str = "AQIDBAUGBw==";
  bool sync_tag = true;
  int32_t callback_id = 1;
  std::vector<uint8_t> source{1, 2, 3, 4, 5, 6, 7};
  rapidjson::Value result =
      TemplateAssemblerRecorder::CreateJSONFromLoadComponentWithCallback(
          url, source, sync_tag, callback_id);
  /*
  {
    "url":"http://testbench",
    "source": "{encoded_str}",
    "sync_tag": true,
    "callback_id": 1
  }
  */
  EXPECT_TRUE(result.GetObject().MemberCount() == 4);
  EXPECT_STREQ(result["url"].GetString(), url.c_str());
  EXPECT_STREQ(result["source"].GetString(), encoded_str.c_str());
  EXPECT_TRUE(result["sync_tag"].GetBool());
  EXPECT_EQ(result["callback_id"].GetInt(), 1);
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
