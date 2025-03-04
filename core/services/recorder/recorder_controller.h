// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_RECORDER_CONTROLLER_H_
#define CORE_SERVICES_RECORDER_RECORDER_CONTROLLER_H_

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/closure.h"

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
  BASE_EXPORT_FOR_DEVTOOL static bool Enable();
  BASE_EXPORT_FOR_DEVTOOL static void StartRecord();
  BASE_EXPORT_FOR_DEVTOOL static void EndRecord(
      base::MoveOnlyClosure<void, std::vector<std::string>&,
                            std::vector<int64_t>&>
          send_complete);
  BASE_EXPORT_FOR_DEVTOOL static void InitConfig(const std::string& path,
                                                 int64_t session_id,
                                                 float screen_width,
                                                 float screen_height,
                                                 int64_t record_id);
  BASE_EXPORT_FOR_DEVTOOL static void* GetTestBenchBaseRecorderInstance();
};
}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_RECORDER_CONTROLLER_H_
