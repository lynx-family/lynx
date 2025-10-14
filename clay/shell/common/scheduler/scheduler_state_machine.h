// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_SHELL_COMMON_SCHEDULER_SCHEDULER_STATE_MACHINE_H_
#define CLAY_SHELL_COMMON_SCHEDULER_SCHEDULER_STATE_MACHINE_H_

#include <memory>
#include <optional>
#include <utility>

namespace clay {

// The StateMachine decides how to coordinate UI thread activite like
// painting/running javascript with rendering and drawing activities on the
// Raster thread.
//
// The state machine tracks internal state but is also influenced by external
// state.  Internal state includes things like whether a frame has been
// requested, while external state includes things like the current time being
// near to the vsync time.
class SchedulerStateMachine {
 public:
  explicit SchedulerStateMachine(bool using_sync_compositor = false);
  ~SchedulerStateMachine();

  SchedulerStateMachine(const SchedulerStateMachine&) = delete;
  SchedulerStateMachine& operator=(const SchedulerStateMachine&) = delete;

  enum class OutputSurfaceState {
    LOST,
    ACTIVE,
  };

  enum class MeaningfulLayoutState {
    DIRTY,
    FINISH,  // The first meaningful layout has been completed.
  };

  enum class BeginFrameState {
    IDLE,             // A new BeginFrame can start.
    SENT,             // A BeginFrame has already been issued.
    READY_TO_COMMIT,  // The previously issued BeginFrame has been processed,
                      // and is ready to commit.
  };

  enum class RasterState {
    IDLE,         // A new Raster BeginFrame can start.
    BEGIN_FRAME,  // A Raster BeginFrame has already been issued, to handle
                  // scoll, animation of Layer on the Raster thread.
    DRAW,         // Draw LayerTree.
    SWAP_BUFFER,  // Swap buffer.
  };

  enum class ImageUploadState {
    PENDING,  // Images are pending to upload.
    READY,    // Images now can start uploading.
  };

  enum class Action : uint8_t {
    NONE,
    BEGIN_FRAME,
    COMMIT,
    ACTIVE_PENDING_TREE,
    PERFORM_RASTER_INVALIDATE,
    DRAW,
    DRAW_ABORT,
    UPLOAD_IMAGE,
  };

  void SetNeedsUIBeginFrame();
  void SetNeedsRasterBeginFrame();
  void SetOutputSurfaceState(OutputSurfaceState output_surface_state);
  void SetMeaningfulLayoutState(MeaningfulLayoutState meaningful_layout_state);

  Action NextAction() const;
  void WillSendUIBeginFrame();
  void WillCommit(bool commit_has_no_updates);
  void DidCommit();
  void WillActivate();
  void WillDraw();
  void DidDraw();
  void WillDrawLayerTree();
  void WillPerformRasterInvalidation();

  // Indicates whether the LayerTree is visible.
  void SetVisible(bool visible);
  bool Visible() const { return visible_; }

  // Indicates that the scheduler is ready to send a BeginFrame.
  void NotifyReadyToSendUIBeginFrame();

  // Call this only in response to receiving an Action::SEND_BEGIN_MAIN_FRAME
  // from NextAction.
  // Indicates that all painting is complete.
  void NotifyReadyToCommit();

  bool HasPendingTree() const;
  bool NeedsUIBeginFrame() const { return needs_ui_begin_frame_; }

  // Indicates that the pending tree is ready for activation. Returns whether
  // the notification received updated the state for the current pending tree,
  // if any.
  bool NotifyReadyToActivate();
  bool IsReadyToActivate() const;

  // We should ensure that we have done the first draw successfully for the
  // current active tree before we activate the next pending tree.
  void NotifyActiveTreeHasBeenDrawn();

  // Indicates the raster frame is ready to be executed, which means that the
  // vsync or frame deadline reached.
  void NotifyReadyToPerformRasterFrame();

  bool RedrawPending() const { return needs_redraw_; }

  // Indicates that the deadline task is posted, and the frame is ready to be
  // drawn when the deadline reached or new UI Frame reached.
  bool OnBeginRasterFrameDeadline();
  // Indicates that the deadline is arrived.
  void OnReachRasterFrameDeadline();
  bool IsInsideRasterFrameDeadline() const;
  bool ShouldIgnoreFrameDeadline() const;

  // Indicates that the first frame has been drawn, excluding the force frame.
  bool IsFirstFrameDrawn() const { return first_frame_drawn_; }

  void OnUploadTaskRegistered();
  void OnUploadImageComplete();

 private:
  bool ShouldPerformRasterInvalidation() const;
  bool ShouldAbortCurrentFrame() const;
  bool ShouldDraw() const;
  bool ShouldActivatePendingTree() const;
  bool ShouldSendUIBeginFrame() const;
  bool ShouldCommit() const;
  bool ShouldUploadImage() const;

  BeginFrameState begin_frame_state_ = BeginFrameState::IDLE;
  OutputSurfaceState output_surface_state_ = OutputSurfaceState::LOST;
  MeaningfulLayoutState meaningful_layout_state_ = MeaningfulLayoutState::DIRTY;
  RasterState raster_state_ = RasterState::IDLE;
  ImageUploadState image_upload_state_ = ImageUploadState::READY;

  // The `needs_ui_begin_frame_` flag is set when a new ui frame is requested.
  bool needs_ui_begin_frame_ = false;
  // The `needs_raster_begin_frame_` flag is set when a new raster frame is
  // requested.
  bool needs_raster_begin_frame_ = false;

  bool needs_upload_image_ = false;

  bool visible_ = true;
  bool ui_begin_frame_is_ready_to_send_ = false;
  bool needs_redraw_ = false;
  bool has_pending_tree_ = false;
  bool pending_tree_is_ready_for_activation_ = false;
  bool active_tree_has_been_drawn_ = true;
  bool did_draw_layer_tree_ = false;
  bool using_sync_compositor_ = false;
  bool inside_raster_frame_deadline_ = false;
  bool first_frame_committed_ = false;
  bool first_frame_drawn_ = false;
};

}  // namespace clay

#endif  // CLAY_SHELL_COMMON_SCHEDULER_SCHEDULER_STATE_MACHINE_H_
