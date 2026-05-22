// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_VSYNC_WAITER_H_
#define CLAY_SHELL_COMMON_VSYNC_WAITER_H_

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "base/include/fml/time/time_point.h"
#include "clay/common/task_runners.h"
#include "clay/flow/frame_timings.h"
#include "clay/shell/common/variable_refresh_rate_reporter.h"
namespace clay {

/// Abstract Base Class that represents a platform specific mechanism for
/// getting callbacks when a vsync event happens.
class VsyncWaiter : public VariableRefreshRateReporter,
                    public std::enable_shared_from_this<VsyncWaiter> {
 public:
  using Callback = std::function<void(std::unique_ptr<FrameTimingsRecorder>)>;
  using AWaitVSyncCallback = std::function<void(std::shared_ptr<VsyncWaiter>,
                                                fml::RefPtr<fml::TaskRunner>)>;

  virtual ~VsyncWaiter();

  void AsyncWaitForVsync(const Callback& callback);

  /// Add a secondary callback for key |id| for the next vsync.
  ///
  /// See also |PointerDataDispatcher::ScheduleSecondaryVsyncCallback| and
  /// |Animator::ScheduleMaybeClearTraceFlowIds|.
  void ScheduleSecondaryCallback(uintptr_t id, const fml::closure& callback);

  bool HasCallback() { return !!callback_; }
  void ResetCallback() { callback_ = nullptr; }

  bool InsideVsync() const { return inside_vsync_; }

  // To be removed, only use the same one in VsyncWaiterService.
  double GetRefreshRate() const override { return 60.0; };

  // Hook the AwaitVSync, it will invoke the callback when AsyncWaitForVsync is
  // called, so we can do some customized vsync request.
  void Hook(AWaitVSyncCallback callback);

 protected:
  // On some backends, the |FireCallback| needs to be made from a static C
  // method.
  friend class VsyncWaiterHarmony;
  friend class VsyncWaiterAndroid;
  friend class VsyncWaiterEmbedder;

  const fml::RefPtr<fml::TaskRunner> task_runner_;

  explicit VsyncWaiter(fml::RefPtr<fml::TaskRunner> task_runner);

  // There are two distinct situations where VsyncWaiter wishes to awaken at
  // the next vsync. Although the functionality can be the same, the intent is
  // different, therefore it makes sense to have a method for each intent.

  // The intent of AwaitVSync() is that the Animator wishes to produce a frame.
  // The underlying implementation can choose to be aware of this intent when
  // it comes to implementing backpressure and other scheduling invariants.
  //
  // Implementations are meant to override this method and arm their vsync
  // latches when in response to this invocation. On vsync, they are meant to
  // invoke the |FireCallback| method once (and only once) with the appropriate
  // arguments. This method should not block the current thread.
  virtual void AwaitVSync() = 0;

  // The intent of AwaitVSyncForSecondaryCallback() is simply to wake up at the
  // next vsync.
  //
  // Because there is no association with frame scheduling, underlying
  // implementations do not need to worry about maintaining invariants or
  // backpressure. The default implementation is to simply follow the same logic
  // as AwaitVSync().
  virtual void AwaitVSyncForSecondaryCallback() { AwaitVSync(); }

  // Schedules the callback on the UI task runner. Needs to be invoked as close
  // to the `frame_start_time` as possible.
  void FireCallback(fml::TimePoint frame_start_time,
                    fml::TimePoint frame_target_time);

 private:
  std::mutex callback_mutex_;
  Callback callback_;
  std::unordered_map<uintptr_t, fml::closure> secondary_callbacks_;
  bool inside_vsync_ = false;
  AWaitVSyncCallback await_vsync_callback_;

  BASE_DISALLOW_COPY_AND_ASSIGN(VsyncWaiter);
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_VSYNC_WAITER_H_
