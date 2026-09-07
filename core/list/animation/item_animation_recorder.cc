// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/animation/item_animation_recorder.h"

#include <utility>

#include "core/list/decoupled_list_types.h"

namespace lynx {
namespace list {

void ItemAnimationRecorder::RecordPreItemLayoutInfo(AnimationTarget* target,
                                                    ItemLayoutInfo pre_info) {
  if (!target) {
    DLIST_LOGE("[" << this
                   << "] ItemAnimationRecorder::RecordPreItemLayoutInfo: "
                   << "target is nullptr");
    DCHECK(target);
    return;
  }
  ItemAnimationRecord& record = GetOrCreateRecord(target);
  record.pre_info = std::move(pre_info);
  record.AddFlag(ItemAnimationRecord::kPre);
}

void ItemAnimationRecorder::RecordPostLayoutInfo(AnimationTarget* target,
                                                 ItemLayoutInfo post_info) {
  if (!target) {
    DLIST_LOGE("[" << this << "] ItemAnimationRecorder::RecordPostLayoutInfo: "
                   << "target is nullptr");
    DCHECK(target);
    return;
  }
  ItemAnimationRecord& record = GetOrCreateRecord(target);
  record.post_info = std::move(post_info);
  record.AddFlag(ItemAnimationRecord::kPost);
}

ItemAnimationRecord& ItemAnimationRecorder::GetOrCreateRecord(
    AnimationTarget* target) {
  return records_.try_emplace(target).first->second;
}

void ItemAnimationRecorder::Process(ProcessCallback& callback) {
  // Move the records for this pass into a local map before dispatching. Do not
  // iterate records_ and then clear it: a callback may reenter and add a new
  // record. Such records must remain in records_ for a later Process() call.
  std::unordered_map<AnimationTarget*, ItemAnimationRecord> processing_records;
  processing_records.swap(records_);

  for (auto& [target, record] : processing_records) {
    if (!target) {
      continue;
    }
    const bool has_pre = record.HasFlag(ItemAnimationRecord::kPre);
    const bool has_post = record.HasFlag(ItemAnimationRecord::kPost);
    const bool changed = record.HasFlag(ItemAnimationRecord::kChanged);
    if (has_pre && has_post) {
      if (changed) {
        // The target has both PRE and POST snapshots and is explicitly marked
        // as changed.
        callback.ProcessChanged(target, record.pre_info, record.post_info);
      } else {
        // The target has both PRE and POST snapshots and is not marked as
        // changed. ItemAnimator decides whether its position actually changed.
        callback.ProcessPersistent(target, record.pre_info, record.post_info);
      }
    } else if (has_pre) {
      // PRE only: the POST target set does not contain this target, so treat it
      // as disappeared.
      callback.ProcessDisappeared(target, record.pre_info);
    } else if (has_post) {
      // POST only: the PRE target set does not contain this target, so treat it
      // as appeared.
      callback.ProcessAppeared(target, record.post_info);
    } else {
      // For example, the record may have kChanged but no layout snapshot.
      callback.ProcessNone(target);
    }
  }
}

}  // namespace list
}  // namespace lynx
