// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/painting_context.h"

namespace lynx {
namespace tasm {

void PaintingContext::SetTimingCollectorPlatform(
    const std::shared_ptr<shell::TimingCollectorPlatform>& timing) {
  timing_collector_platform_ = timing;
  platform_impl_->SetTimingCollectorPlatform(timing);
};

void PaintingContext::SetUIOperationQueue(
    const std::shared_ptr<shell::DynamicUIOperationQueue>& queue) {
  ui_operation_queue_ = queue;
  platform_impl_->SetUIOperationQueue(queue);
}

void PaintingContext::OnNodeReady(int tag) {
  patching_node_ready_ids_.emplace_back(tag);
}

void PaintingContext::OnNodeReload(int tag) {
  patching_node_reload_ids_.emplace_back(tag);
}

void PaintingContext::InsertPaintingNode(int parent, int child, int index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "InsertPaintingNode");
  if (platform_impl_->HasEnableUIOperationBatching()) {
    platform_impl_->InsertPaintingNode(parent, child, index);
  } else {
    Enqueue(
        [platform_ref = platform_impl_->GetPlatformRef(), parent, child,
         index]() { platform_ref->InsertPaintingNode(parent, child, index); });
  }
}

// The `is_move` flag indicates that this is a part of a move operation. For
// move operations, we can skip the detach lifecycle and maintain the view
// state without resetting it (such as the focus state).
//
// To move a painting node, you are required to promptly add the view back.
// For example:
//   RemovePaintingNode(parent, child, index, true);
//   InsertPaintingNode(new_parent, child, new_index);
void PaintingContext::RemovePaintingNode(int parent, int child, int index,
                                         bool is_move) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "PaintingContext::RemovePaintingNode");
  if (platform_impl_->HasEnableUIOperationBatching()) {
    platform_impl_->RemovePaintingNode(parent, child, index, is_move);
  } else {
    Enqueue([platform_ref = platform_impl_->GetPlatformRef(), parent, child,
             index, is_move]() {
      platform_ref->RemovePaintingNode(parent, child, index, is_move);
    });
  }

  if (!is_move) {
    // only add child sign to remove_ids_ vector when it is not a move
    patching_node_remove_ids_.emplace_back(child);
  }
}

void PaintingContext::DestroyPaintingNode(int parent, int child, int index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "DestroyPaintingNode");
  if (platform_impl_->HasEnableUIOperationBatching()) {
    platform_impl_->DestroyPaintingNode(parent, child, index);
  } else {
    Enqueue(
        [platform_ref = platform_impl_->GetPlatformRef(), parent, child,
         index]() { platform_ref->DestroyPaintingNode(parent, child, index); });
  }
}

void PaintingContext::UpdateNodeReadyPatching() {
  if (platform_impl_->HasEnableUIOperationBatching()) {
    platform_impl_->UpdateNodeReadyPatching(patching_node_ready_ids_,
                                            patching_node_remove_ids_);
  } else {
    Enqueue([platform_ref = platform_impl_->GetPlatformRef(),
             ready_ids = patching_node_ready_ids_,
             remove_ids = patching_node_remove_ids_]() {
      platform_ref->UpdateNodeReadyPatching(std::move(ready_ids),
                                            std::move(remove_ids));
    });
  }

  patching_node_ready_ids_.clear();
  patching_node_remove_ids_.clear();
}

void PaintingContext::UpdateNodeReloadPatching() {
  // high_priority task.
  EnqueueHighPriorityUIOperation(
      [platform_ref = platform_impl_->GetPlatformRef(),
       reload_ids = patching_node_reload_ids_]() {
        platform_ref->UpdateNodeReloadPatching(std::move(reload_ids));
      });

  patching_node_reload_ids_.clear();
}

void PaintingContext::OnCollectExtraUpdates(int32_t id) {
  // Remove this funciton later.
  Enqueue([platform_ref = platform_impl_->GetPlatformRef(), id]() {
    platform_ref->OnCollectExtraUpdates(id);
  });
}

void PaintingContext::UpdateScrollInfo(int32_t container_id, bool smooth,
                                       float estimated_offset, bool scrolling) {
  Enqueue([platform_ref = platform_impl_->GetPlatformRef(), container_id,
           smooth, estimated_offset, scrolling]() mutable {
    platform_ref->UpdateScrollInfo(container_id, smooth, estimated_offset,
                                   scrolling);
  });
}

void PaintingContext::SetGestureDetectorState(int64_t id, int32_t gesture_id,
                                              int32_t state) {
  Enqueue([platform_ref = platform_impl_->GetPlatformRef(), id, gesture_id,
           state]() mutable {
    platform_ref->SetGestureDetectorState(id, gesture_id, state);
  });
}

void PaintingContext::UpdateEventInfo(bool has_touch_pseudo) {
  Enqueue(
      [platform_ref = platform_impl_->GetPlatformRef(), has_touch_pseudo]() {
        platform_ref->UpdateEventInfo(has_touch_pseudo);
      });
}

void PaintingContext::UpdateFlattenStatus(int id, bool flatten) {
  Enqueue([platform_ref = platform_impl_->GetPlatformRef(), id, flatten]() {
    platform_ref->UpdateFlattenStatus(id, flatten);
  });
}

void PaintingContext::ListReusePaintingNode(int id,
                                            const base::String& item_key) {
  Enqueue([platform_ref = platform_impl_->GetPlatformRef(), id, item_key]() {
    platform_ref->ListReusePaintingNode(id, item_key.str());
  });
}

void PaintingContext::ListCellWillAppear(int id, const base::String& item_key) {
  Enqueue([platform_ref = platform_impl_->GetPlatformRef(), id, item_key]() {
    platform_ref->ListCellWillAppear(id, item_key.str());
  });
}

void PaintingContext::ListCellDisappear(int id, bool isExist,
                                        const base::String& item_key) {
  Enqueue([platform_ref = platform_impl_->GetPlatformRef(), id, isExist,
           item_key]() {
    platform_ref->ListCellDisappear(id, isExist, item_key.str());
  });
}

void PaintingContext::InsertListItemPaintingNode(int32_t list_id,
                                                 int32_t child_id) {
  Enqueue(
      [platform_ref = platform_impl_->GetPlatformRef(), list_id, child_id]() {
        platform_ref->InsertListItemPaintingNode(list_id, child_id);
      });
}

void PaintingContext::RemoveListItemPaintingNode(int32_t list_id,
                                                 int32_t child_id) {
  Enqueue(
      [platform_ref = platform_impl_->GetPlatformRef(), list_id, child_id]() {
        platform_ref->RemoveListItemPaintingNode(list_id, child_id);
      });
}

void PaintingContext::UpdateContentOffsetForListContainer(
    int32_t container_id, float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset) {
  Enqueue([platform_ref = platform_impl_->GetPlatformRef(), container_id,
           content_size, delta_x, delta_y, is_init_scroll_offset] {
    platform_ref->UpdateContentOffsetForListContainer(
        container_id, content_size, delta_x, delta_y, is_init_scroll_offset);
  });
}

void PaintingContext::Enqueue(shell::UIOperation op, bool high_priority) {
  if (!platform_impl_->EnableUIOperationQueue() || !ui_operation_queue_) {
    op();
    return;
  }

  auto task = platform_impl_->ExecuteOperationSafely(std::move(op));
  if (high_priority) {
    ui_operation_queue_->EnqueueHighPriorityUIOperation(std::move(task));
  } else {
    ui_operation_queue_->EnqueueUIOperation(std::move(task));
  }
}

void PaintingContext::MarkUIOperationQueueFlushTiming(
    tasm::TimingKey key, const tasm::PipelineID& pipeline_id) {
  if (pipeline_id.empty()) {
    return;
  }

  std::weak_ptr<shell::TimingCollectorPlatform> weak_timing_collector_platform =
      timing_collector_platform_;
  Enqueue(
      [weak_timing_collector_platform, key = std::move(key), pipeline_id]() {
        TRACE_EVENT(LYNX_TRACE_CATEGORY,
                    "UIOperationQueue::MarkUIOperationQueueFlushTimingTask");
        if (auto timing_collector_platform =
                weak_timing_collector_platform.lock()) {
          timing_collector_platform->MarkTiming(pipeline_id, key);
        }
      });
}

void PaintingContext::SetNeedMarkDrawEndTiming(
    const tasm::PipelineID& pipeline_id) {
  if (pipeline_id.empty()) {
    return;
  }

  std::weak_ptr<shell::TimingCollectorPlatform> weak_timing_collector =
      timing_collector_platform_;
  Enqueue([platform_ref = platform_impl_->GetPlatformRef(),
           weak_timing_collector, pipeline_id]() {
    platform_ref->SetNeedMarkDrawEndTiming(std::move(weak_timing_collector),
                                           pipeline_id);
  });
}

void PaintingContext::MarkLayoutUIOperationQueueFlushStartIfNeed() {
  for (const auto& option_for_timing : options_for_timing_) {
    if (option_for_timing.need_timestamps) {
      MarkUIOperationQueueFlushTiming(
          tasm::timing::kLayoutUiOperationExecuteStart,
          option_for_timing.pipeline_id);
    }
  }
}

void PaintingContext::FinishLayoutOperation(const PipelineOptions& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "FinishLayoutOperation");
  if (has_first_screen_) {
    platform_impl_->FinishLayoutOperation(options);
  }
  // timing
  // Pass the opions to the tasm thread through the tasm queue, and mount
  // them on the PaintingContext. The UI Flush stage reads the opions from
  // the PaintingContext for collecting timing, and clears the opions at the
  // end.
  for (const auto& option_for_timing : options_for_timing_) {
    if (option_for_timing.need_timestamps) {
      MarkUIOperationQueueFlushTiming(
          tasm::timing::kLayoutUiOperationExecuteEnd,
          option_for_timing.pipeline_id);
    }
    if (option_for_timing.need_timestamps &&
        !option_for_timing.pipeline_id.empty()) {
      SetNeedMarkDrawEndTiming(option_for_timing.pipeline_id);
    }
  }
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "CleanOptionsForTiming");
    // clean
    ClearOptionsForTiming();
  }
}

void PaintingContext::SetContextHasAttached() {
  platform_impl_->SetContextHasAttached();
}

}  // namespace tasm
}  // namespace lynx
