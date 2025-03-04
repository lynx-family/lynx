// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/recorder_controller.h"

#include <sstream>
#include <utility>

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
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance()
      .AddLynxViewSessionID(record_id, session_id);
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance().SetRecorderPath(
      path);
  lynx::tasm::recorder::TestBenchBaseRecorder::GetInstance().SetScreenSize(
      record_id, screen_width, screen_height);
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

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
