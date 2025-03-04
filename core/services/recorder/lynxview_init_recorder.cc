// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/lynxview_init_recorder.h"

#include <iostream>

namespace lynx {
namespace tasm {
namespace recorder {

void LynxViewInitRecorder::RecordViewPort(int32_t layout_height_mode,
                                          int32_t layout_width_mode,
                                          double preferred_layout_height,
                                          double preferred_layout_width,
                                          double preferred_max_layout_height,
                                          double preferred_max_layout_width,
                                          double ratio, int64_t record_id) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value params_val(rapidjson::kObjectType);

  params_val.AddMember(rapidjson::StringRef(kParamLayoutHeightMode),
                       layout_height_mode, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamLayoutWidthMode),
                       layout_width_mode, allocator);

  params_val.AddMember(rapidjson::StringRef(kParamPreferredLayoutHeight),
                       preferred_layout_height, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamPreferredLayoutWidth),
                       preferred_layout_width, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamPreferredMaxLayoutHeight),
                       preferred_max_layout_height, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamPreferredMaxLayoutWidth),
                       preferred_max_layout_width, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamRatio), ratio, allocator);

  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncUpdateViewPort,
                                                    params_val, record_id);
}

void LynxViewInitRecorder::RecordThreadStrategy(int32_t threadStrategy,
                                                int64_t record_id,
                                                bool enableJSRuntime) {
  if (!TestBenchBaseRecorder::GetInstance().IsRecordingProcess()) {
    return;
  }
  rapidjson::Document::AllocatorType& allocator =
      TestBenchBaseRecorder::GetInstance().GetAllocator();

  rapidjson::Value params_val(rapidjson::kObjectType);

  params_val.AddMember(rapidjson::StringRef(kParamThreadStrategy),
                       threadStrategy, allocator);
  params_val.AddMember(rapidjson::StringRef(kParamEnableJSRuntime),
                       enableJSRuntime, allocator);

  TestBenchBaseRecorder::GetInstance().RecordAction(kFuncSetThreadStrategy,
                                                    params_val, record_id);
}

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx
