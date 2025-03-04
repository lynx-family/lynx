// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <condition_variable>
#define private public
#include "core/services/recorder/testbench_base_recorder.h"
#undef private

#include <mutex>

#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace recorder {

void wait(fml::Thread& thread) {
  std::condition_variable condition;
  std::mutex local_mutex;
  std::unique_lock<std::mutex> lock(local_mutex);
  thread.GetTaskRunner()->PostTask([&condition, &local_mutex]() {
    std::unique_lock<std::mutex> lock(local_mutex);
    condition.notify_one();
  });
  condition.wait(lock);
}

TEST(TestBenchBaseRecorder, RecordScripts) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  // enable record
  ark.is_recording_ = true;
  std::string url = "url";
  std::string content = "content";
  ark.RecordScripts(url.c_str(), content.c_str());
  wait(ark.thread_);
  ASSERT_TRUE(ark.scripts_table_.IsObject());
  EXPECT_EQ(ark.scripts_table_.MemberCount(), 1);
  ASSERT_TRUE(ark.scripts_table_.HasMember(url));
  EXPECT_STREQ((ark.scripts_table_[url]).GetString(), "eJxLzs8rSc0rAQALywL8");
}

void CheckLynxViewTable(TestBenchBaseRecorder& ark, int64_t record_id) {
  rapidjson::Value& recorded_file = ark.lynx_view_table_[record_id];

  ASSERT_TRUE(recorded_file.IsObject());
  EXPECT_EQ(recorded_file.MemberCount(), 4);

  rapidjson::Value& action_list = recorded_file[kActionList];
  ASSERT_TRUE(action_list.IsArray());
  EXPECT_EQ(action_list.Size(), 0);

  rapidjson::Value& invoked_method_data = recorded_file[kInvokedMethodData];
  ASSERT_TRUE(invoked_method_data.IsArray());
  EXPECT_EQ(invoked_method_data.Size(), 0);

  rapidjson::Value& callback = recorded_file[kCallback];
  ASSERT_TRUE(callback.IsObject());
  EXPECT_EQ(callback.MemberCount(), 0);

  rapidjson::Value& component_list_value = recorded_file[kComponentList];
  ASSERT_TRUE(component_list_value.IsArray());
  EXPECT_EQ(component_list_value.Size(), 0);
}

TEST(TestBenchBaseRecorder, Clear) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  ark.Clear();
  EXPECT_EQ(ark.lynx_view_table_.size(), 0);
}

TEST(TestBenchBaseRecorder, CreateRecordedFile) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  EXPECT_EQ(ark.lynx_view_table_.size(), 0);
  int64_t record_id = 1;
  ark.CreateRecordedFile(record_id);
  EXPECT_EQ(ark.lynx_view_table_.size(), 1);
  CheckLynxViewTable(ark, record_id);
  ark.Clear();
}

TEST(TestBenchBaseRecorder, GetRecordedFile) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  ark.GetRecordedFile(1);
  CheckLynxViewTable(ark, 1);
  EXPECT_EQ(ark.lynx_view_table_.size(), 1);
  ark.GetRecordedFile(2);
  CheckLynxViewTable(ark, 2);
  EXPECT_EQ(ark.lynx_view_table_.size(), 2);
  ark.Clear();
}

TEST(TestBenchBaseRecorder, GetRecordedFileField) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  rapidjson::Value& value = ark.GetRecordedFileField(record_id, kActionList);
  ASSERT_TRUE(value.IsArray());
  EXPECT_EQ(value.Size(), 0);

  value = ark.GetRecordedFileField(record_id, kInvokedMethodData);
  ASSERT_TRUE(value.IsArray());
  EXPECT_EQ(value.Size(), 0);

  value = ark.GetRecordedFileField(record_id, kCallback);
  ASSERT_TRUE(value.IsObject());
  EXPECT_EQ(value.MemberCount(), 0);

  value = ark.GetRecordedFileField(record_id, kComponentList);
  ASSERT_TRUE(value.IsArray());
  EXPECT_EQ(value.Size(), 0);
  ark.Clear();
}

TEST(TestBenchBaseRecorder, SetRecorderPath) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  ark.SetRecorderPath("/your/local/path");
  EXPECT_STREQ(ark.file_path_.c_str(), "/your/local/path/");
}

TEST(TestBenchBaseRecorder, SetScreenSize) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  ark.SetScreenSize(record_id, 123, 456);
  EXPECT_EQ(ark.replay_config_map_.size(), 1);

  rapidjson::Value& config = ark.replay_config_map_[record_id];
  ASSERT_TRUE(config.IsObject());
  EXPECT_EQ(config.MemberCount(), 4);

  rapidjson::Value& jsb_ignored_info = config["jsbIgnoredInfo"];
  ASSERT_TRUE(jsb_ignored_info.IsArray());
  EXPECT_EQ(jsb_ignored_info.Size(), 0);

  rapidjson::Value& jsb_settings = config["jsbSettings"];
  ASSERT_TRUE(jsb_settings.IsObject());
  EXPECT_EQ(jsb_settings.MemberCount(), 1);

  rapidjson::Value& strict = jsb_settings["strict"];
  ASSERT_TRUE(strict.IsBool());
  ASSERT_TRUE(strict.GetBool());
}

TEST(TestBenchBaseRecorder, StartRecord) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  ark.StartRecord();
  ASSERT_TRUE(ark.is_recording_);
}

TEST(TestBenchBaseRecorder, RecordAction) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  rapidjson::Value& action_list = ark.GetRecordedFile(record_id)[kActionList];
  EXPECT_EQ(action_list.Size(), 0);
  ark.StartRecord();
  rapidjson::Value params(rapidjson::kObjectType);

  ark.RecordAction("TestFunction", params, record_id);
  ark.RecordAction("TestGlobalFunction", params, record_id);

  wait(ark.thread_);

  EXPECT_EQ(action_list.Size(), 2);
  rapidjson::Value& test_action = action_list[0];
  ASSERT_TRUE(test_action.IsObject());
  EXPECT_EQ(test_action.MemberCount(), 4);
  rapidjson::Value& function_name = test_action[kFunctionName];
  EXPECT_STREQ(function_name.GetString(), "TestFunction");

  ASSERT_TRUE(test_action.HasMember(kParamRecordTime));
  ASSERT_TRUE(test_action.HasMember(kParamRecordMillisecond));
  ASSERT_TRUE(test_action.HasMember(kParams));

  rapidjson::Value& recorded_params = test_action[kParams];
  ASSERT_TRUE(recorded_params.IsObject());

  test_action = action_list[1];
  ASSERT_TRUE(test_action.IsObject());
  EXPECT_EQ(test_action.MemberCount(), 4);
  function_name = test_action[kFunctionName];
  EXPECT_STREQ(function_name.GetString(), "TestGlobalFunction");

  ASSERT_TRUE(test_action.HasMember(kParamRecordTime));
  ASSERT_TRUE(test_action.HasMember(kParamRecordMillisecond));
  ASSERT_TRUE(test_action.HasMember(kParams));

  recorded_params = test_action[kParams];
  ASSERT_TRUE(recorded_params.IsObject());

  ark.Clear();
}

TEST(TestBenchBaseRecorder, RecordInvokedMethodData) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int record_id = 1;
  ark.StartRecord();
  ark.CreateRecordedFile(record_id);
  rapidjson::Value& invoked_method_data =
      ark.GetRecordedFileField(record_id, kInvokedMethodData);
  EXPECT_EQ(invoked_method_data.Size(), 0);
  rapidjson::Value params(rapidjson::kObjectType);
  ark.RecordInvokedMethodData("bridge", "call", params, record_id);
  wait(ark.thread_);
  EXPECT_EQ(invoked_method_data.Size(), 1);
  rapidjson::Value& invoked_module = invoked_method_data[0];
  ASSERT_TRUE(invoked_module.IsObject());
  EXPECT_EQ(invoked_module.MemberCount(), 5);
  ASSERT_TRUE(invoked_module.HasMember(kModuleName));
  EXPECT_STREQ(invoked_module[kModuleName].GetString(), "bridge");

  ASSERT_TRUE(invoked_module.HasMember(kMethodName));
  EXPECT_STREQ(invoked_module[kMethodName].GetString(), "call");

  ASSERT_TRUE(invoked_module.HasMember(kParamRecordTime));
  ASSERT_TRUE(invoked_module.HasMember(kParamRecordMillisecond));
  ASSERT_TRUE(invoked_module.HasMember(kParams));
  ASSERT_TRUE(invoked_module[kParams].IsObject());

  ark.Clear();
}

TEST(TestBenchBaseRecorder, RecordCallback) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  int64_t callback_id = 2;
  ark.StartRecord();
  ark.CreateRecordedFile(record_id);
  rapidjson::Value& callback = ark.GetRecordedFileField(record_id, kCallback);
  EXPECT_EQ(callback.MemberCount(), 0);
  rapidjson::Value params(rapidjson::kObjectType);
  ark.RecordCallback("bridge", "call", params, callback_id, record_id);
  wait(ark.thread_);
  EXPECT_EQ(callback.MemberCount(), 1);
  ASSERT_TRUE(callback.HasMember("2"));
  rapidjson::Value& callback_info = callback["2"];

  ASSERT_TRUE(callback_info.IsObject());
  EXPECT_EQ(callback_info.MemberCount(), 5);
  ASSERT_TRUE(callback_info.HasMember(kModuleName));
  EXPECT_STREQ(callback_info[kModuleName].GetString(), "bridge");

  ASSERT_TRUE(callback_info.HasMember(kMethodName));
  EXPECT_STREQ(callback_info[kMethodName].GetString(), "call");

  ASSERT_TRUE(callback_info.HasMember(kParamRecordTime));
  ASSERT_TRUE(callback_info.HasMember(kParamRecordMillisecond));
  ASSERT_TRUE(callback_info.HasMember(kParams));
  ASSERT_TRUE(callback_info[kParams].IsObject());
  ark.Clear();
}

TEST(TestBenchBaseRecorder, RecordComponent) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  int64_t record_id = 1;
  int64_t type = 66;
  const char* component_name = "test-ui";
  ark.StartRecord();
  rapidjson::Value& component_list =
      ark.GetRecordedFileField(record_id, kComponentList);
  EXPECT_EQ(component_list.Size(), 0);

  ark.RecordComponent(component_name, type, record_id);
  wait(ark.thread_);
  EXPECT_EQ(component_list.Size(), 1);
  rapidjson::Value& component = component_list[0];
  ASSERT_TRUE(component.IsObject());
  EXPECT_EQ(component.MemberCount(), 2);
  ASSERT_TRUE(component.HasMember(kComponentName));
  EXPECT_STREQ(component[kComponentName].GetString(), component_name);
  ASSERT_TRUE(component.HasMember(kComponentType));
  EXPECT_EQ(component[kComponentType].GetInt(), type);
  ark.Clear();
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
