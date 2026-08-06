// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_RECORDER_CONTROLLER_H_
#define CORE_SERVICES_RECORDER_RECORDER_CONTROLLER_H_

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "base/include/closure.h"
#include "core/base/lynx_export.h"

#if defined(ENABLE_TESTBENCH_RECORDER) && ENABLE_TESTBENCH_RECORDER
#include "core/services/recorder/lynxview_init_recorder.h"
#include "core/services/recorder/native_module_recorder.h"
#include "core/services/recorder/template_assembler_recorder.h"
#include "core/services/recorder/testbench_base_recorder.h"
#endif

namespace lynx {
namespace tasm {
namespace recorder {

class RecorderController {
 public:
  LYNX_EXPORT_FOR_DEVTOOL static bool Enable();
  LYNX_EXPORT_FOR_DEVTOOL static void StartRecord();
  LYNX_EXPORT_FOR_DEVTOOL static void EndRecord(
      base::MoveOnlyClosure<void, std::vector<std::string>&,
                            std::vector<int64_t>&>
          send_complete);
  LYNX_EXPORT_FOR_DEVTOOL static void InitConfig(const std::string& path,
                                                 int64_t session_id,
                                                 float screen_width,
                                                 float screen_height,
                                                 int64_t record_id);
  LYNX_EXPORT_FOR_DEVTOOL static void RemoveRecord(int64_t record_id);
  LYNX_EXPORT_FOR_DEVTOOL static void* GetTestBenchBaseRecorderInstance();
  LYNX_EXPORT_FOR_DEVTOOL static void RecordDebugInfo(
      int64_t record_id, const std::string& url, const std::string& debug_info);
};
}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

extern "C" {
LYNX_EXPORT_FOR_DEVTOOL bool LynxTestBenchRecorderIsRecording();
LYNX_EXPORT_FOR_DEVTOOL uint64_t LynxTestBenchRecorderRecordingGeneration();
LYNX_EXPORT_FOR_DEVTOOL void LynxTestBenchRecorderInitConfig(
    const char* path, int64_t session_id, float screen_width,
    float screen_height, int64_t record_id);
LYNX_EXPORT_FOR_DEVTOOL bool LynxTestBenchRecorderRecordAction(
    int64_t record_id, const char* function_name, const char* params_json);
LYNX_EXPORT_FOR_DEVTOOL bool LynxTestBenchRecorderRecordInvokedMethod(
    int64_t record_id, const char* module_name, const char* method_name,
    const char* params_json);
LYNX_EXPORT_FOR_DEVTOOL uint64_t
LynxTestBenchRecorderRecordInvokedMethodWithGeneration(int64_t record_id,
                                                       const char* module_name,
                                                       const char* method_name,
                                                       const char* params_json);
LYNX_EXPORT_FOR_DEVTOOL bool LynxTestBenchRecorderRecordCallback(
    int64_t record_id, const char* module_name, const char* method_name,
    int64_t callback_id, const char* params_json);
LYNX_EXPORT_FOR_DEVTOOL bool LynxTestBenchRecorderRecordCallbackWithGeneration(
    int64_t record_id, const char* module_name, const char* method_name,
    int64_t callback_id, const char* params_json,
    uint64_t recording_generation);
}

#endif  // CORE_SERVICES_RECORDER_RECORDER_CONTROLLER_H_
