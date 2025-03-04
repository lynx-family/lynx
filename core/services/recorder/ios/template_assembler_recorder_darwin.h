// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_RECORDER_IOS_TEMPLATE_ASSEMBLER_RECORDER_DARWIN_H_
#define CORE_SERVICES_RECORDER_IOS_TEMPLATE_ASSEMBLER_RECORDER_DARWIN_H_

#import <UIKit/UIKit.h>

#include <string>
#include <vector>

#import "LynxView.h"
#include "core/services/recorder/template_assembler_recorder.h"

namespace lynx {
namespace tasm {
namespace recorder {

struct UITouchRecord {
  UITouchRecord(UITouch* touch, LynxView* view);

  double time_stamp_;

  // unique id
  int64_t unique_id_;

  // UITouchPhase
  int64_t touch_phase_;

  // Tap count
  int64_t tap_count_;

  // UITouchType
  int64_t touch_type_;

  double major_radius_;
  double major_radius_tolerance_;

  // location in LynxView
  double location_in_lynx_view_x_;
  double location_in_lynx_view_y_;
};

struct UIEventRecord {
  UIEventRecord(UIEvent* event, LynxView* view);

  double time_stamp_;

  // UIEventType
  int64_t type_{-1};

  // UIEventSubType
  int64_t subtype_{-1};

  std::vector<UITouchRecord> touchs_{};
};

class TemplateAssemblerRecorderDarwin {
 public:
  static void RecordPlatformEventDarwin(UIEvent* event, LynxView* view);
};

}  // namespace recorder
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_RECORDER_IOS_TEMPLATE_ASSEMBLER_RECORDER_DARWIN_H_
