// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/scheduler/scheduler.h"

#include "base/include/auto_reset.h"
#include "clay/fml/logging.h"

namespace clay {

Scheduler::Scheduler(SchedulerClient* scheduler_client,
                     bool using_sync_compositor)
    : using_sync_compositor_(using_sync_compositor),
      state_machine_(using_sync_compositor_),
      scheduler_client_(scheduler_client) {
  FML_DCHECK(scheduler_client_);
}

Scheduler::~Scheduler() {}

void Scheduler::SetVisible(bool visible) {
  state_machine_.SetVisible(visible);
  ProcessScheduledAction();
}

bool Scheduler::OnDraw() {
  FML_DCHECK(using_sync_compositor_);
  lynx::base::AutoReset<bool> resetter(&sync_compositor_result_, false);
  // Perform the raster frame if needed, and will set the result to
  // sync_compositor_result_ if succeed.
  OnBeginRasterFrame();
  // Sync the frame result.
  return sync_compositor_result_;
}

void Scheduler::SetOutputSurfaceValid(bool valid) {
  state_machine_.SetOutputSurfaceState(
      valid ? SchedulerStateMachine::OutputSurfaceState::ACTIVE
            : SchedulerStateMachine::OutputSurfaceState::LOST);
  ProcessScheduledAction();
}

void Scheduler::SetMeaningfulLayout(bool meaningful_layout) {
  state_machine_.SetMeaningfulLayoutState(
      meaningful_layout ? SchedulerStateMachine::MeaningfulLayoutState::FINISH
                        : SchedulerStateMachine::MeaningfulLayoutState::DIRTY);
  ProcessScheduledAction();
}

void Scheduler::SetNeedsUIBeginFrame() {
  state_machine_.SetNeedsUIBeginFrame();
  ProcessScheduledAction();
}

bool Scheduler::NeedsUIBeginFrame() const {
  return state_machine_.NeedsUIBeginFrame();
}

void Scheduler::SetNeedsRasterBeginFrame() {
  state_machine_.SetNeedsRasterBeginFrame();
  ProcessScheduledAction();
}

void Scheduler::NotifyReadyToSendUIBeginFrame() {
  state_machine_.NotifyReadyToSendUIBeginFrame();
  ProcessScheduledAction();
}

void Scheduler::NotifyReadyToCommit(bool commit_has_no_updates) {
  commit_has_no_updates_ = commit_has_no_updates;
  state_machine_.NotifyReadyToCommit();
  // The new UI Frame is arrived, if the scheduler is still waiting for the
  // deadline, we ignore it and perform raster frame immediately.
  if (state_machine_.IsInsideRasterFrameDeadline()) {
    OnReachRasterFrameDeadline();
  }
  ProcessScheduledAction();
}

void Scheduler::NotifyReadyToActivate() {
  state_machine_.NotifyReadyToActivate();
  ProcessScheduledAction();
}

void Scheduler::NotifyActiveTreeHasBeenDrawn() {
  sync_compositor_result_ = true;
  state_machine_.NotifyActiveTreeHasBeenDrawn();
  ProcessScheduledAction();
}

void Scheduler::NotifyReadyToPerformRasterFrame() {
  state_machine_.NotifyReadyToPerformRasterFrame();
  ProcessScheduledAction();
}

void Scheduler::OnBeginRasterFrame() {
  // Send UI begin frame at first if needed, and it will set a deadline with the
  // SchedulerClient.
  state_machine_.NotifyReadyToSendUIBeginFrame();
  ProcessScheduledAction();
  if (!state_machine_.IsInsideRasterFrameDeadline()) {
    // The scheduler is not in frame deadline scheduling phase, try to trigger
    // raster on the active LayerTree.
    NotifyReadyToPerformRasterFrame();
  }
}

bool Scheduler::BeginRasterFrameDeadline() {
  return state_machine_.OnBeginRasterFrameDeadline();
}

void Scheduler::OnReachRasterFrameDeadline() {
  if (state_machine_.IsInsideRasterFrameDeadline()) {
    state_machine_.OnReachRasterFrameDeadline();
    NotifyReadyToPerformRasterFrame();
  }
}

bool Scheduler::IsFirstFrameDrawn() const {
  return state_machine_.IsFirstFrameDrawn();
}

void Scheduler::NotifyUploadTaskRegistered() {
  state_machine_.OnUploadTaskRegistered();
  ProcessScheduledAction();
}

void Scheduler::OnImageUploadTaskFinished(uint32_t left_tasks) {
  if (left_tasks == 0) {
    state_machine_.OnUploadImageComplete();
  }
}

void Scheduler::ProcessScheduledAction() {
  if (stopped_) {
    return;
  }

  SchedulerStateMachine::Action action;
  do {
    action = state_machine_.NextAction();
    switch (action) {
      case SchedulerStateMachine::Action::NONE:
        break;
      case SchedulerStateMachine::Action::BEGIN_FRAME:
        // Schedule UI begin frame.
        state_machine_.WillSendUIBeginFrame();
        scheduler_client_->ScheduledActionBeginFrame();
        break;
      case SchedulerStateMachine::Action::COMMIT:
        // Schedule submit UI LayerTree to Raster.
        state_machine_.WillCommit(commit_has_no_updates_);
        scheduler_client_->ScheduledActionCommit();
        break;
      case SchedulerStateMachine::Action::ACTIVE_PENDING_TREE:
        // Schedule merge Pending LayerTree to Active LayerTree.
        state_machine_.WillActivate();
        scheduler_client_->ScheduledActionActivePendingTree();
        break;
      case SchedulerStateMachine::Action::PERFORM_RASTER_INVALIDATE:
        // Schedule raster scroll, animation, deferred image decoding.
        state_machine_.WillPerformRasterInvalidation();
        scheduler_client_->ScheduledActionRasterInvalidate();
        break;
      case SchedulerStateMachine::Action::DRAW:
        // Schedule draw LayerTree.
        state_machine_.WillDraw();
        scheduler_client_->ScheduledActionDraw();
        state_machine_.DidDraw();
        break;
      case SchedulerStateMachine::Action::DRAW_ABORT:
        // No action is actually performed, but this allows the state machine to
        // drain the pipeline without actually drawing.
        break;
      case SchedulerStateMachine::Action::UPLOAD_IMAGE:
        // Schedule upload image.
        scheduler_client_->ScheduledActionUploadImage();
        break;
    }
  } while (SchedulerStateMachine::Action::NONE != action);
}

}  // namespace clay
