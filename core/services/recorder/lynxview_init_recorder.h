// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_LYNXVIEW_INIT_RECORDER_H_
#define CORE_SERVICES_RECORDER_LYNXVIEW_INIT_RECORDER_H_

#include "core/services/recorder/recorder_constants.h"
#include "core/services/recorder/testbench_base_recorder.h"
#include "third_party/rapidjson/document.h"
namespace lynx {
namespace tasm {
namespace recorder {

class LynxViewInitRecorder {
 public:
  static LynxViewInitRecorder& GetInstance() {
    static base::NoDestructor<LynxViewInitRecorder> instance_;
    return *instance_.get();
  }

  void RecordViewPort(int32_t layout_height_mode, int32_t layout_width_mode,
                      double preferred_layout_height,
                      double preferred_layout_width,
                      double preferred_max_layout_height,
                      double preferred_max_layout_width, double ratio,
                      int64_t record_id);

  void RecordThreadStrategy(int32_t threadStrategy, int64_t record_id,
                            bool enableJSRuntime);

 private:
  friend base::NoDestructor<LynxViewInitRecorder>;
  LynxViewInitRecorder() = default;
  ~LynxViewInitRecorder() = default;
  LynxViewInitRecorder(const LynxViewInitRecorder&) = delete;
  LynxViewInitRecorder& operator=(const LynxViewInitRecorder&) = delete;
};

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_LYNXVIEW_INIT_RECORDER_H_
