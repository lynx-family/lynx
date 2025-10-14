// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/shell/common/scheduler/scheduler_state_machine.h"

#include "clay/fml/logging.h"

namespace clay {

SchedulerStateMachine::SchedulerStateMachine(bool using_sync_compositor)
    : using_sync_compositor_(using_sync_compositor) {}

SchedulerStateMachine::~SchedulerStateMachine() {}

void SchedulerStateMachine::SetVisible(bool visible) {
  if (visible_ == visible) {
    return;
  }

  visible_ = visible;

  did_draw_layer_tree_ = false;
}

void SchedulerStateMachine::SetNeedsUIBeginFrame() {
  needs_ui_begin_frame_ = true;
}

void SchedulerStateMachine::SetNeedsRasterBeginFrame() {
  needs_raster_begin_frame_ = true;
}

void SchedulerStateMachine::SetOutputSurfaceState(
    OutputSurfaceState output_surface_state) {
  output_surface_state_ = output_surface_state;
}

void SchedulerStateMachine::SetMeaningfulLayoutState(
    MeaningfulLayoutState meaningful_layout_state) {
  meaningful_layout_state_ = meaningful_layout_state;
}

SchedulerStateMachine::Action SchedulerStateMachine::NextAction() const {
  if (ShouldSendUIBeginFrame()) {
    return Action::BEGIN_FRAME;
  }
  if (ShouldCommit()) {
    return Action::COMMIT;
  }
  if (ShouldActivatePendingTree()) {
    return Action::ACTIVE_PENDING_TREE;
  }
  if (ShouldPerformRasterInvalidation()) {
    return Action::PERFORM_RASTER_INVALIDATE;
  }
  if (ShouldDraw()) {
    return Action::DRAW;
  }
  if (ShouldUploadImage()) {
    return Action::UPLOAD_IMAGE;
  }

  return Action::NONE;
}

void SchedulerStateMachine::WillSendUIBeginFrame() {
  FML_DCHECK(visible_);
  FML_DCHECK(begin_frame_state_ == BeginFrameState::IDLE);
  FML_DCHECK(needs_ui_begin_frame_);
  FML_DCHECK(ui_begin_frame_is_ready_to_send_);

  begin_frame_state_ = BeginFrameState::SENT;
  needs_ui_begin_frame_ = false;
  ui_begin_frame_is_ready_to_send_ = false;
  image_upload_state_ = ImageUploadState::PENDING;
}

void SchedulerStateMachine::WillCommit(bool commit_has_no_updates) {
  FML_DCHECK(begin_frame_state_ == BeginFrameState::READY_TO_COMMIT);
  if (!commit_has_no_updates) {
    // We have a new pending tree.
    has_pending_tree_ = true;
    pending_tree_is_ready_for_activation_ = false;
    if (!first_frame_committed_) {
      first_frame_committed_ = true;
    }
    image_upload_state_ = ImageUploadState::PENDING;
  } else {
    image_upload_state_ = ImageUploadState::READY;
  }
  begin_frame_state_ = BeginFrameState::IDLE;
}

void SchedulerStateMachine::WillActivate() {
  FML_DCHECK(has_pending_tree_);

  has_pending_tree_ = false;
  pending_tree_is_ready_for_activation_ = false;
  active_tree_has_been_drawn_ = false;

  needs_raster_begin_frame_ = true;
}

void SchedulerStateMachine::WillDraw() {
  FML_DCHECK(!did_draw_layer_tree_);
  FML_DCHECK(needs_redraw_);
  // We need to reset needs_redraw_ before we draw.
  needs_redraw_ = false;
  // Reset `inside_raster_frame_deadline_`
  inside_raster_frame_deadline_ = false;
  raster_state_ = RasterState::DRAW;
  image_upload_state_ = ImageUploadState::PENDING;
}

void SchedulerStateMachine::DidDraw() {
  raster_state_ = RasterState::IDLE;
  image_upload_state_ = ImageUploadState::READY;
}

void SchedulerStateMachine::WillPerformRasterInvalidation() {}

void SchedulerStateMachine::NotifyReadyToSendUIBeginFrame() {
  if (needs_ui_begin_frame_) {
    ui_begin_frame_is_ready_to_send_ = true;
  }
}

void SchedulerStateMachine::NotifyReadyToCommit() {
  FML_DCHECK(begin_frame_state_ == BeginFrameState::SENT);
  begin_frame_state_ = BeginFrameState::READY_TO_COMMIT;
  FML_DCHECK(has_pending_tree_ || ShouldCommit());
  image_upload_state_ = ImageUploadState::PENDING;
}

bool SchedulerStateMachine::HasPendingTree() const { return has_pending_tree_; }

bool SchedulerStateMachine::NotifyReadyToActivate() {
  if (!has_pending_tree_ || pending_tree_is_ready_for_activation_) {
    return false;
  }
  pending_tree_is_ready_for_activation_ = true;
  return true;
}

bool SchedulerStateMachine::IsReadyToActivate() const {
  return pending_tree_is_ready_for_activation_;
}

void SchedulerStateMachine::NotifyActiveTreeHasBeenDrawn() {
  active_tree_has_been_drawn_ = true;
  if (!first_frame_drawn_ && first_frame_committed_) {
    // Mark the first frame has been drawn.
    first_frame_drawn_ = true;
  }
}

void SchedulerStateMachine::NotifyReadyToPerformRasterFrame() {
  if (needs_raster_begin_frame_) {
    raster_state_ = RasterState::BEGIN_FRAME;
    needs_raster_begin_frame_ = false;
    // Set the flag `needs_redraw_` which will schedule Action::DRAW.
    needs_redraw_ = true;
    image_upload_state_ = ImageUploadState::PENDING;
  }
}

bool SchedulerStateMachine::OnBeginRasterFrameDeadline() {
  if (using_sync_compositor_) {
    // Disable the deadline mechanism if we are using sync compositor, because
    // we prefer that the frame to be drawn immediately.
    return false;
  }
  if (needs_raster_begin_frame_) {
    // It has no need to send deadline if we have no raster frame.
    return false;
  }
  if (begin_frame_state_ != BeginFrameState::SENT) {
    // It has no need to send deadline if we have no UI frame.
    return false;
  }
  if (ShouldIgnoreFrameDeadline()) {
    // Ignore the deadline if we have a pending tree, we should prioritize
    // active & draw the pending tree.
    return false;
  }
  inside_raster_frame_deadline_ = true;
  return true;
}

void SchedulerStateMachine::OnReachRasterFrameDeadline() {
  inside_raster_frame_deadline_ = false;
}

bool SchedulerStateMachine::IsInsideRasterFrameDeadline() const {
  return inside_raster_frame_deadline_;
}

bool SchedulerStateMachine::ShouldIgnoreFrameDeadline() const {
  // Ignore frame deadline if we have a pending tree, we should begin raster
  // frame immediately when the frame signal arrived in that case.
  return has_pending_tree_ || !active_tree_has_been_drawn_;
}

bool SchedulerStateMachine::ShouldPerformRasterInvalidation() const {
  return false;
}

bool SchedulerStateMachine::ShouldAbortCurrentFrame() const {
  // If we're not visible, we should just abort the frame. we can simply
  // activate on becoming invisible since we don't need to draw the active tree
  // when we're in this state.
  if (!visible_) {
    return true;
  }
  // If our output surface is not ready, we should just abort the frame.
  if (output_surface_state_ == OutputSurfaceState::LOST) {
    return true;
  }
  return false;
}

bool SchedulerStateMachine::ShouldDraw() const {
  // Do not draw more than once in the deadline. Aborted draws are ok because
  // those are effectively nops.
  if (did_draw_layer_tree_) {
    return false;
  }
  // Don't draw if we are waiting for the output surface.
  if (output_surface_state_ != OutputSurfaceState::ACTIVE) {
    return false;
  }

  return needs_redraw_;
}

bool SchedulerStateMachine::ShouldActivatePendingTree() const {
  // There is nothing to activate.
  if (!has_pending_tree_) {
    return false;
  }
  if (ShouldAbortCurrentFrame()) {
    return true;
  }
  // We should ensure that we have done the first draw for the current active
  // tree.
  if (!active_tree_has_been_drawn_) {
    return false;
  }
  // At this point, only activate if we are ready to activate.
  return pending_tree_is_ready_for_activation_;
}

bool SchedulerStateMachine::ShouldSendUIBeginFrame() const {
  if (!needs_ui_begin_frame_) {
    return false;
  }
  // We can not perform commits if we are not visible.
  if (!visible_) {
    return false;
  }
  if (!ui_begin_frame_is_ready_to_send_) {
    return false;
  }
  // The first meaningful layout is not complete, it has no need to begin a ui
  // frame.
  if (meaningful_layout_state_ == MeaningfulLayoutState::DIRTY) {
    return false;
  }
  // Only send BeginUIFrame when there isn't another commit pending already.
  if (begin_frame_state_ != BeginFrameState::IDLE) {
    return false;
  }
  return true;
}

bool SchedulerStateMachine::ShouldCommit() const {
  if (begin_frame_state_ != BeginFrameState::READY_TO_COMMIT) {
    return false;
  }
  // We must not finish the commit until the pending tree is free.
  if (has_pending_tree_) {
    return false;
  }
  return true;
}

bool SchedulerStateMachine::ShouldUploadImage() const {
  return image_upload_state_ == ImageUploadState::READY && needs_upload_image_;
}

void SchedulerStateMachine::OnUploadTaskRegistered() {
  needs_upload_image_ = true;
}
void SchedulerStateMachine::OnUploadImageComplete() {
  needs_upload_image_ = false;
}

}  // namespace clay
