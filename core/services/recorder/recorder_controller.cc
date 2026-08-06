// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/recorder_controller.h"

#include <sstream>
#include <utility>

#if ENABLE_TESTBENCH_RECORDER
#include "third_party/rapidjson/document.h"
#endif

namespace lynx {
namespace tasm {
namespace recorder {

bool RecorderController::Enable() {
#if ENABLE_TESTBENCH_RECORDER
  return true;
#else
  return false;
#endif
}

void RecorderController::StartRecord() {
#if ENABLE_TESTBENCH_RECORDER
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance().StartRecord();
#endif
}

void RecorderController::EndRecord(
    base::MoveOnlyClosure<void, std::vector<std::string>&,
                          std::vector<int64_t>&>
        send_complete) {
#if ENABLE_TESTBENCH_RECORDER
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance().EndRecord(
      std::move(send_complete));
#endif
}

void RecorderController::InitConfig(const std::string& path, int64_t session_id,
                                    float screen_width, float screen_height,
                                    int64_t record_id) {
#if ENABLE_TESTBENCH_RECORDER
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance().InitConfig(
      path, session_id, screen_width, screen_height, record_id);
#endif
}

void RecorderController::RemoveRecord(int64_t record_id) {
#if ENABLE_TESTBENCH_RECORDER
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance().RemoveRecord(
      record_id);
#endif
}

void* RecorderController::GetTestBenchBaseRecorderInstance() {
#if ENABLE_TESTBENCH_RECORDER
  return static_cast<void*>(
      &lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance());
#else
  return nullptr;
#endif
}

void RecorderController::RecordDebugInfo(int64_t record_id,
                                         const std::string& url,
                                         const std::string& debug_info) {
#if ENABLE_TESTBENCH_RECORDER
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance().RecordDebugInfo(
      record_id, url, debug_info);
#endif
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

namespace {

#if ENABLE_TESTBENCH_RECORDER
bool ParseRecorderParams(const char* params_json,
                         rapidjson::Document& document) {
  if (params_json == nullptr) {
    return false;
  }

  document.Parse(params_json);
  return !document.HasParseError() && document.IsObject();
}
#endif

}  // namespace

extern "C" bool LynxTestBenchRecorderIsRecording() {
#if ENABLE_TESTBENCH_RECORDER
  return lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance()
      .IsRecordingProcess();
#else
  return false;
#endif
}

extern "C" uint64_t LynxTestBenchRecorderRecordingGeneration() {
#if ENABLE_TESTBENCH_RECORDER
  return lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance()
      .RecordingGeneration();
#else
  return 0;
#endif
}

extern "C" void LynxTestBenchRecorderInitConfig(const char* path,
                                                int64_t session_id,
                                                float screen_width,
                                                float screen_height,
                                                int64_t record_id) {
#if ENABLE_TESTBENCH_RECORDER
  if (path == nullptr || path[0] == '\0' || record_id == 0) {
    return;
  }
  lynx::tasm::recorder::RecorderController::InitConfig(
      path, session_id, screen_width, screen_height, record_id);
#endif
}

extern "C" bool LynxTestBenchRecorderRecordAction(int64_t record_id,
                                                  const char* function_name,
                                                  const char* params_json) {
#if ENABLE_TESTBENCH_RECORDER
  if (record_id == 0 || function_name == nullptr || function_name[0] == '\0' ||
      params_json == nullptr) {
    return false;
  }

  rapidjson::Document params;
  if (!ParseRecorderParams(params_json, params)) {
    return false;
  }

  auto& recorder = lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance();
  uint64_t recording_generation = recorder.RecordingGeneration();
  if (recording_generation == 0 || !recorder.IsRecordingProcess() ||
      recorder.RecordingGeneration() != recording_generation) {
    return false;
  }
  recorder.RecordActionWithGeneration(function_name, std::move(params),
                                      record_id, recording_generation);
  return true;
#else
  return false;
#endif
}

extern "C" uint64_t LynxTestBenchRecorderRecordInvokedMethodWithGeneration(
    int64_t record_id, const char* module_name, const char* method_name,
    const char* params_json) {
#if ENABLE_TESTBENCH_RECORDER
  if (record_id == 0 || module_name == nullptr || module_name[0] == '\0' ||
      method_name == nullptr || method_name[0] == '\0') {
    return 0;
  }

  auto& recorder = lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance();
  uint64_t recording_generation = recorder.RecordingGeneration();
  if (recording_generation == 0 || !recorder.IsRecordingProcess() ||
      recorder.RecordingGeneration() != recording_generation) {
    return 0;
  }
  rapidjson::Document params;
  if (!ParseRecorderParams(params_json, params)) {
    return 0;
  }
  if (!recorder.IsRecordingProcess() ||
      recorder.RecordingGeneration() != recording_generation) {
    return 0;
  }
  recorder.RecordInvokedMethodDataWithGeneration(module_name, method_name,
                                                 std::move(params), record_id,
                                                 recording_generation);
  return recording_generation;
#else
  return 0;
#endif
}

extern "C" bool LynxTestBenchRecorderRecordInvokedMethod(
    int64_t record_id, const char* module_name, const char* method_name,
    const char* params_json) {
  return LynxTestBenchRecorderRecordInvokedMethodWithGeneration(
             record_id, module_name, method_name, params_json) != 0;
}

extern "C" bool LynxTestBenchRecorderRecordCallbackWithGeneration(
    int64_t record_id, const char* module_name, const char* method_name,
    int64_t callback_id, const char* params_json,
    uint64_t recording_generation) {
#if ENABLE_TESTBENCH_RECORDER
  if (record_id == 0 || module_name == nullptr || module_name[0] == '\0' ||
      method_name == nullptr || method_name[0] == '\0' ||
      recording_generation == 0) {
    return false;
  }

  auto& recorder = lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance();
  if (!recorder.IsRecordingProcess()) {
    return false;
  }
  if (recorder.RecordingGeneration() != recording_generation) {
    return false;
  }
  rapidjson::Document params;
  if (!ParseRecorderParams(params_json, params)) {
    return false;
  }
  if (!recorder.IsRecordingProcess() ||
      recorder.RecordingGeneration() != recording_generation) {
    return false;
  }
  recorder.RecordCallbackWithGeneration(module_name, method_name,
                                        std::move(params), callback_id,
                                        record_id, recording_generation);
  return true;
#else
  return false;
#endif
}

extern "C" bool LynxTestBenchRecorderRecordCallback(int64_t record_id,
                                                    const char* module_name,
                                                    const char* method_name,
                                                    int64_t callback_id,
                                                    const char* params_json) {
  return LynxTestBenchRecorderRecordCallbackWithGeneration(
      record_id, module_name, method_name, callback_id, params_json,
      LynxTestBenchRecorderRecordingGeneration());
}
