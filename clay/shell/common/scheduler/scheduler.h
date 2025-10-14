// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SCHEDULER_SCHEDULER_H_
#define CLAY_SHELL_COMMON_SCHEDULER_SCHEDULER_H_

#include <memory>

#include "clay/shell/common/scheduler/scheduler_client.h"
#include "clay/shell/common/scheduler/scheduler_state_machine.h"

namespace clay {

// The scheduler seperates "what to do next" from the updating of its internal
// state to make testing cleaner.
class Scheduler {
 public:
  explicit Scheduler(SchedulerClient* scheduler_client,
                     bool using_sync_compositor = false);
  ~Scheduler();

  void Stop() { stopped_ = true; }
  void Resume() { stopped_ = false; }
  void SetVisible(bool visible);

  // Do draw for synchronous compositor.
  bool OnDraw();

  // Indicates whether the output surface is valid, we should not do action draw
  // if it is invalid.
  void SetOutputSurfaceValid(bool valid);

  // Indicates that current page has meaningful layout, if there is no
  // meaningful layout, we have no need to send begin frame.
  void SetMeaningfulLayout(bool meaningful_layout);

  // Set |needs_ui_begin_frame_| to true, which will cause the BeginFrame
  // action with appropriate timing.
  void SetNeedsUIBeginFrame();

  bool NeedsUIBeginFrame() const;

  // Set |needs_raster_begin_frame_| to true, which will cause the Draw action
  // this client so that this client can send a RasterFrame with appropriate
  // timing.
  void SetNeedsRasterBeginFrame();

  // Set |ui_begin_frame_is_ready_to_send_| to true, which will cause the
  // BeginFrame action with appropriate timing depends on SetNeedsUIBeginFrame.
  void NotifyReadyToSendUIBeginFrame();

  // Commit step happens after the UI thread has completed updating for a
  // BeginFrame request from the raster. Call this method when the
  // UI thread updates are completed to signal it is ready for the commmit.
  // The param `commit_has_no_updates` indicates whether there is no new layer
  // tree.
  void NotifyReadyToCommit(bool commit_has_no_updates);

  // We have 2 copies of the layer trees on the raster thread: pending_tree
  // and active_tree. When we receive a new vsync signal, call this method to
  // notify that this pending tree is ready to be activated, that is to be
  // copied to the active tree.
  void NotifyReadyToActivate();

  // We should ensure that we have done the first draw successfully for the
  // current active tree before we activate the next pending tree.
  void NotifyActiveTreeHasBeenDrawn();

  // Indicates that the raster frame is ready to be executed, which means that
  // vsync or frame deadline reached.
  void NotifyReadyToPerformRasterFrame();

  // Indicates that the Raster thread has received a new frame signal, and it's
  // ready to perform a new Frame draw. The scheduler will schedule the UI frame
  // and the Raster frame with the state machine.
  void OnBeginRasterFrame();
  bool BeginRasterFrameDeadline();
  // Indicates that the frame deadline arrived.
  void OnReachRasterFrameDeadline();

  // Indicates that the first frame has been drawn.
  bool IsFirstFrameDrawn() const;

  void NotifyUploadTaskRegistered();
  void OnImageUploadTaskFinished(uint32_t left_tasks);

 private:
  void ProcessScheduledAction();

  bool stopped_ = false;
  bool using_sync_compositor_ = false;
  bool commit_has_no_updates_ = false;
  bool sync_compositor_result_ = false;

  SchedulerStateMachine state_machine_;
  SchedulerClient* scheduler_client_ = nullptr;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SCHEDULER_SCHEDULER_H_
