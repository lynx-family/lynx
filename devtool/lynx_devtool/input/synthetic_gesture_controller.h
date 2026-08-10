// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_GESTURE_CONTROLLER_H_
#define DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_GESTURE_CONTROLLER_H_

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>

#include "base/include/fml/task_runner.h"
#include "devtool/lynx_devtool/input/synthetic_gesture.h"

namespace lynx {
namespace base {
class VSyncMonitor;
}  // namespace base
namespace devtool {
namespace input {

class InputEventTarget;

// Sequence-bound to the task runner passed to Create(). Queueing gestures,
// target acknowledgements, completion callbacks, and destruction must all
// occur on that sequence.
class SyntheticGestureController
    : public std::enable_shared_from_this<SyntheticGestureController> {
 public:
  using CompletionCallback = std::function<void(SyntheticGestureResult)>;

  static std::shared_ptr<SyntheticGestureController> Create(
      std::shared_ptr<InputEventTarget> target,
      const fml::RefPtr<fml::TaskRunner>& task_runner);
  ~SyntheticGestureController();

  void QueueSyntheticGesture(std::unique_ptr<SyntheticGesture> gesture,
                             CompletionCallback callback);

 private:
  struct PendingGesture {
    std::unique_ptr<SyntheticGesture> gesture;
    CompletionCallback callback;
  };

  SyntheticGestureController(std::shared_ptr<InputEventTarget> target,
                             const fml::RefPtr<fml::TaskRunner>& task_runner);

  void StartNextGesture();
  void RequestNextFrame();
  void OnFrameRequested(uint64_t request_id);
  void OnFrame(int64_t frame_time_us);
  void OnInputProcessed(uint64_t ack_id, bool success);
  void Complete(SyntheticGestureResult result);

  std::shared_ptr<InputEventTarget> target_;
  fml::RefPtr<fml::TaskRunner> task_runner_;
  std::shared_ptr<base::VSyncMonitor> vsync_monitor_;
  std::deque<PendingGesture> pending_gestures_;
  std::unique_ptr<SyntheticGesture> active_gesture_;
  CompletionCallback active_callback_;
  bool waiting_for_input_processed_{false};
  uint64_t frame_request_id_{0};
  uint64_t pending_frame_request_id_{0};
  uint64_t input_ack_id_{0};
};

}  // namespace input
}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_INPUT_SYNTHETIC_GESTURE_CONTROLLER_H_
