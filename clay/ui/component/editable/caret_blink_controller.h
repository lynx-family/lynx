// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_EDITABLE_CARET_BLINK_CONTROLLER_H_
#define CLAY_UI_COMPONENT_EDITABLE_CARET_BLINK_CONTROLLER_H_

#include <functional>
#include <memory>

#include "base/include/fml/task_runner.h"
#include "base/include/fml/time/timer.h"

namespace clay {

class CaretBlinkController {
 public:
  using SetCaretDisplayCallback = std::function<void(bool)>;

  CaretBlinkController(fml::RefPtr<fml::TaskRunner> task_runner,
                       SetCaretDisplayCallback set_caret_display);
  ~CaretBlinkController();

  void Start();
  void Stop();

 private:
  void Toggle();
  void SetCaretDisplay(bool display);

  fml::RefPtr<fml::TaskRunner> task_runner_;
  SetCaretDisplayCallback set_caret_display_;
  std::unique_ptr<fml::RepeatingTimer> timer_;
  bool display_ = false;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_EDITABLE_CARET_BLINK_CONTROLLER_H_
