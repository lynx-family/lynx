// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <condition_variable>
#include <cstdio>
#include <mutex>

#define private public
#include "core/services/recorder/testbench_base_recorder.h"
#undef private

#include "core/services/recorder/recorder_controller.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace tasm {
namespace recorder {

namespace {

void WaitForRecorderTasks(fml::Thread& thread) {
  std::condition_variable condition;
  std::mutex local_mutex;
  std::unique_lock<std::mutex> lock(local_mutex);
  thread.GetTaskRunner()->PostTask([&condition, &local_mutex]() {
    std::unique_lock<std::mutex> lock(local_mutex);
    condition.notify_one();
  });
  condition.wait(lock);
}

}  // namespace

TEST(TestBenchRecorderBridge, ExternalScriptBridgePreservesBinaryData) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t record_id = 4;
  constexpr char kCompressedSource[] = {
      'e', 'J', 'z', 'z', 'q', 'c', 'y', 'r', 'Y', 'P', 'g', 'f', 'k',
      'l', 'p', 'c', 'A', 'g', 'A', 'W', 'z', 'w', 'R', 'L', '\0'};
  ark.ResetForTesting();

  const char source[] = {'L', 'y', 'n', 'x', '\0', static_cast<char>(0xff),
                         'T', 'e', 's', 't'};
  ark.StartRecord();
  LynxTestBenchRecordExternalScriptWithSize("", source, sizeof(source));
  LynxTestBenchRecordExternalScriptWithSize("empty.js", source, 0);
  LynxTestBenchRecordExternalScriptWithSize("template.js", source,
                                            sizeof(source));
  WaitForRecorderTasks(ark.thread_);

  ASSERT_EQ(ark.external_script_cache_.size(), 1);
  ASSERT_EQ(ark.external_script_cache_.count("template.js"), 1);
  EXPECT_EQ(ark.external_script_cache_["template.js"], kCompressedSource);

  rapidjson::Value& scripts = ark.GetRecordedFile(record_id)[kScripts];
  ASSERT_TRUE(scripts.HasMember("template.js"));
  EXPECT_STREQ(scripts["template.js"].GetString(), kCompressedSource);
  ark.ResetForTesting();
}

TEST(TestBenchRecorderBridge, ExternalRecordBridgeRejectsDataBeforeRecording) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t record_id = 90213;
  ark.ResetForTesting();

  EXPECT_FALSE(LynxTestBenchRecorderRecordInvokedMethod(
      record_id, "ExampleBridge", "invoke",
      R"({"argc":1,"args":["bootstrap"],"returnValue":"ready"})"));
  EXPECT_FALSE(LynxTestBenchRecorderRecordCallbackWithGeneration(
      record_id, "ExampleBridge", "invoke", 43, R"({"returnValue":"done"})",
      1));
  WaitForRecorderTasks(ark.thread_);

  EXPECT_EQ(ark.lynx_view_table_.count(record_id), 0);
}

TEST(TestBenchRecorderBridge, ExternalRecordBridgeUsesStandardRecordShape) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t record_id = 90211;
  ark.ResetForTesting();
  EXPECT_FALSE(LynxTestBenchRecorderIsRecording());

  ark.StartRecord();
  const char* params = R"({"argc":0,"args":[],"returnValue":"undefined"})";
  uint64_t recording_generation =
      LynxTestBenchRecorderRecordInvokedMethodWithGeneration(
          record_id, "ExampleBridge", "invoke", params);
  EXPECT_NE(recording_generation, 0u);
  EXPECT_FALSE(LynxTestBenchRecorderRecordInvokedMethod(0, "ExampleBridge",
                                                        "invoke", params));
  EXPECT_FALSE(LynxTestBenchRecorderRecordInvokedMethod(record_id, "", "invoke",
                                                        params));
  EXPECT_FALSE(LynxTestBenchRecorderRecordCallbackWithGeneration(
      0, "ExampleBridge", "invoke", 1, params, recording_generation));
  EXPECT_FALSE(LynxTestBenchRecorderRecordInvokedMethod(
      record_id, "ExampleBridge", "invoke", "invalid-json"));
  WaitForRecorderTasks(ark.thread_);

  ASSERT_EQ(ark.lynx_view_table_.count(record_id), 1);
  rapidjson::Value& invoked_method_data =
      ark.lynx_view_table_[record_id][kInvokedMethodData];
  ASSERT_EQ(invoked_method_data.Size(), 1);
  EXPECT_STREQ(invoked_method_data[0][kModuleName].GetString(),
               "ExampleBridge");
  EXPECT_STREQ(invoked_method_data[0][kMethodName].GetString(), "invoke");

  ark.ResetForTesting();
}

TEST(TestBenchRecorderBridge, RejectsCallbackFromPreviousRecordingGeneration) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t record_id = 90217;
  ark.ResetForTesting();
  ark.SetRecorderPath("/tmp");
  ark.StartRecord();

  uint64_t first_generation =
      LynxTestBenchRecorderRecordInvokedMethodWithGeneration(
          record_id, "ExampleBridge", "invoke",
          R"({"argc":1,"args":["getSystemInfo"],"returnValue":"undefined"})");
  ASSERT_NE(first_generation, 0u);
  WaitForRecorderTasks(ark.thread_);

  ark.EndRecord(base::MoveOnlyClosure<void, std::vector<std::string>&,
                                      std::vector<int64_t>&>(
      [](std::vector<std::string>&, std::vector<int64_t>&) {}));
  WaitForRecorderTasks(ark.thread_);
  ark.StartRecord();
  ASSERT_GT(LynxTestBenchRecorderRecordingGeneration(), first_generation);

  EXPECT_FALSE(LynxTestBenchRecorderRecordCallbackWithGeneration(
      record_id, "ExampleBridge", "invoke", 7,
      R"({"returnValue":{"errMsg":"getSystemInfo:ok"}})", first_generation));
  WaitForRecorderTasks(ark.thread_);
  EXPECT_EQ(ark.lynx_view_table_.count(record_id), 0);

  ark.ResetForTesting();
  std::remove("/tmp/90217.json");
}

TEST(TestBenchRecorderBridge, AcceptsCallbackFromCurrentRecordingGeneration) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t record_id = 90218;
  ark.ResetForTesting();
  ark.StartRecord();

  uint64_t recording_generation =
      LynxTestBenchRecorderRecordInvokedMethodWithGeneration(
          record_id, "ExampleBridge", "invoke",
          R"({"argc":1,"args":["getSystemInfo"],"returnValue":"undefined","callback":["7"]})");
  ASSERT_NE(recording_generation, 0u);
  EXPECT_TRUE(LynxTestBenchRecorderRecordCallbackWithGeneration(
      record_id, "ExampleBridge", "invoke", 7,
      R"({"returnValue":{"errMsg":"getSystemInfo:ok"}})",
      recording_generation));
  WaitForRecorderTasks(ark.thread_);

  rapidjson::Value& callback = ark.lynx_view_table_[record_id][kCallback];
  ASSERT_TRUE(callback.HasMember("7"));
  EXPECT_STREQ(callback["7"][kParams]["returnValue"]["errMsg"].GetString(),
               "getSystemInfo:ok");

  ark.ResetForTesting();
}

TEST(TestBenchRecorderBridge, RepeatedStartKeepsCurrentRecordingGeneration) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  ark.ResetForTesting();
  ark.StartRecord();
  uint64_t recording_generation = LynxTestBenchRecorderRecordingGeneration();

  ark.StartRecord();

  EXPECT_EQ(LynxTestBenchRecorderRecordingGeneration(), recording_generation);
  ark.ResetForTesting();
}

TEST(TestBenchRecorderBridge, ExternalRecordBridgeInitializesReplayConfig) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t record_id = 90216;
  ark.ResetForTesting();

  LynxTestBenchRecorderInitConfig("/tmp", -1, 390, 844, record_id);
  WaitForRecorderTasks(ark.thread_);

  EXPECT_STREQ(ark.file_path_.c_str(), "/tmp/");
  ASSERT_TRUE(ark.replay_config_map_[record_id].IsObject());
  EXPECT_EQ(ark.session_ids_[record_id], -1);
  EXPECT_FLOAT_EQ(ark.replay_config_map_[record_id]["screenWidth"].GetFloat(),
                  390);
  EXPECT_FLOAT_EQ(ark.replay_config_map_[record_id]["screenHeight"].GetFloat(),
                  844);
  EXPECT_EQ(ark.lynx_view_table_.count(record_id), 0);

  ark.ResetForTesting();
}

TEST(TestBenchRecorderBridge, InitConfigCanFollowSynchronousStartRecord) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t record_id = 90217;
  ark.ResetForTesting();

  ark.StartRecord();
  LynxTestBenchRecorderInitConfig("/tmp", -1, 390, 844, record_id);
  rapidjson::Value params(rapidjson::kObjectType);
  ark.RecordComponent("view", 1, record_id);
  ark.RecordDebugInfo(record_id, "source.js", "debug info");
  ark.RecordSharedData("shared", params, record_id);
  ark.RecordPreloadScript("preload.js", "preload source", record_id);
  ark.RecordInvokedMethodData("ExampleBridge", "invoke", params, record_id);
  WaitForRecorderTasks(ark.thread_);

  ASSERT_TRUE(ark.replay_config_map_[record_id].IsObject());
  ASSERT_EQ(ark.lynx_view_table_.count(record_id), 1);
  rapidjson::Value& record = ark.GetRecordedFile(record_id);
  EXPECT_EQ(record[kComponentList].Size(), 1);
  EXPECT_EQ(record[kDebugInfo].Size(), 1);
  EXPECT_TRUE(record[kSharedData].HasMember("shared"));
  EXPECT_TRUE(record[kPreloadScripts].HasMember("preload.js"));
  EXPECT_EQ(record[kInvokedMethodData].Size(), 1);
  ark.ResetForTesting();
}

TEST(TestBenchRecorderBridge, SynchronousStartRecordCanFollowInitConfig) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t record_id = 90218;
  ark.ResetForTesting();

  LynxTestBenchRecorderInitConfig("/tmp", -1, 390, 844, record_id);
  WaitForRecorderTasks(ark.thread_);
  ark.StartRecord();
  rapidjson::Value params(rapidjson::kObjectType);
  ark.RecordComponent("view", 1, record_id);
  ark.RecordDebugInfo(record_id, "source.js", "debug info");
  ark.RecordSharedData("shared", params, record_id);
  WaitForRecorderTasks(ark.thread_);

  ASSERT_EQ(ark.lynx_view_table_.count(record_id), 1);
  rapidjson::Value& record = ark.GetRecordedFile(record_id);
  EXPECT_EQ(record[kComponentList].Size(), 1);
  EXPECT_EQ(record[kDebugInfo].Size(), 1);
  EXPECT_TRUE(record[kSharedData].HasMember("shared"));
  ark.ResetForTesting();
}

TEST(TestBenchRecorderBridge,
     ExternalTemplateBridgeUsesReplayLoadTemplateShape) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t record_id = 90214;
  constexpr char kUrl[] = "argus://microapp/test/pages/index-template.js";
  constexpr char kTemplate[] = "abc\0def";
  ark.ResetForTesting();
  ark.StartRecord();

  LynxTestBenchRecordExternalTemplateWithSize(0, kUrl, kTemplate,
                                              sizeof(kTemplate) - 1);
  LynxTestBenchRecordExternalTemplateWithSize(record_id, "", kTemplate,
                                              sizeof(kTemplate) - 1);
  LynxTestBenchRecordExternalTemplateWithSize(record_id, kUrl, kTemplate,
                                              sizeof(kTemplate) - 1);
  WaitForRecorderTasks(ark.thread_);

  rapidjson::Value& action_list = ark.GetRecordedFile(record_id)[kActionList];
  ASSERT_EQ(action_list.Size(), 1);
  rapidjson::Value& action = action_list[0];
  EXPECT_STREQ(action[kFunctionName].GetString(), "loadTemplate");
  rapidjson::Value& params = action[kParams];
  EXPECT_STREQ(params["url"].GetString(), kUrl);
  EXPECT_STREQ(params["source"].GetString(), "YWJjAGRlZg==");
  ASSERT_TRUE(params["templateData"].IsObject());
  EXPECT_EQ(params["templateData"].MemberCount(), 0);
  EXPECT_TRUE(params["isCSR"].GetBool());

  ark.ResetForTesting();
}

TEST(TestBenchRecorderBridge, RecorderControllerPreservesLegacyABI) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t kRecordId = 7;
  constexpr int64_t kCallbackId = 8;
  constexpr char kParams[] = R"({"returnValue":"ok"})";

  ark.ResetForTesting();
  ark.StartRecord();

  EXPECT_TRUE(LynxTestBenchRecorderRecordInvokedMethod(
      kRecordId, "ExampleBridge", "invoke", kParams));
  uint64_t generation = LynxTestBenchRecorderRecordInvokedMethodWithGeneration(
      kRecordId, "ExampleBridge", "invoke", kParams);
  EXPECT_EQ(generation, ark.RecordingGeneration());
  EXPECT_TRUE(LynxTestBenchRecorderRecordCallback(
      kRecordId, "ExampleBridge", "invoke", kCallbackId, kParams));
  EXPECT_TRUE(LynxTestBenchRecorderRecordCallbackWithGeneration(
      kRecordId, "ExampleBridge", "invoke", kCallbackId + 1, kParams,
      generation));

  WaitForRecorderTasks(ark.thread_);
  EXPECT_EQ(ark.GetRecordedFile(kRecordId)[kInvokedMethodData].Size(), 2);
  EXPECT_EQ(ark.GetRecordedFile(kRecordId)[kCallback].MemberCount(), 2);
  ark.ResetForTesting();
}

TEST(TestBenchRecorderBridge, RecorderControllerRecordsActionsFromJson) {
  TestBenchBaseRecorder& ark = TestBenchBaseRecorder::GetInstance();
  constexpr int64_t kRecordId = 9;
  constexpr char kParamsJson[] =
      R"({"arguments":["__batch_event__",[{"data":"123"}]]})";

  ark.ResetForTesting();
  ark.StartRecord();

  EXPECT_FALSE(
      LynxTestBenchRecorderRecordAction(0, "sendGlobalEvent", kParamsJson));
  EXPECT_FALSE(LynxTestBenchRecorderRecordAction(kRecordId, "", kParamsJson));
  EXPECT_FALSE(
      LynxTestBenchRecorderRecordAction(kRecordId, "sendGlobalEvent", "[]"));
  EXPECT_TRUE(LynxTestBenchRecorderRecordAction(kRecordId, "sendGlobalEvent",
                                                kParamsJson));

  WaitForRecorderTasks(ark.thread_);
  rapidjson::Value& action_list = ark.GetRecordedFile(kRecordId)[kActionList];
  ASSERT_EQ(action_list.Size(), 1);
  ASSERT_TRUE(action_list[0].IsObject());
  EXPECT_STREQ(action_list[0][kFunctionName].GetString(), "sendGlobalEvent");
  const rapidjson::Value& recorded_params = action_list[0][kParams];
  ASSERT_TRUE(recorded_params.IsObject());
  ASSERT_TRUE(recorded_params.HasMember("arguments"));
  const rapidjson::Value& arguments = recorded_params["arguments"];
  ASSERT_TRUE(arguments.IsArray());
  ASSERT_EQ(arguments.Size(), 2);
  EXPECT_STREQ(arguments[0].GetString(), "__batch_event__");
  ASSERT_TRUE(arguments[1].IsArray());
  ASSERT_EQ(arguments[1].Size(), 1);
  ASSERT_TRUE(arguments[1][0].IsObject());
  EXPECT_STREQ(arguments[1][0]["data"].GetString(), "123");
  ark.ResetForTesting();
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
