// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_ANIMATION_ITEM_ANIMATION_RECORDER_H_
#define CORE_LIST_ANIMATION_ITEM_ANIMATION_RECORDER_H_

#include <unordered_map>

#include "core/list/animation/animation_target.h"
#include "core/list/animation/animation_types.h"

namespace lynx {
namespace list {

// Records target snapshots and converts PRE/POST/CHANGED flag combinations to
// high-level animation callbacks.
class ItemAnimationRecorder {
 public:
  class ProcessCallback {
   public:
    virtual ~ProcessCallback() = default;

    virtual void ProcessDisappeared(AnimationTarget* target,
                                    const ItemLayoutInfo& pre_layout_info) = 0;

    virtual void ProcessAppeared(AnimationTarget* target,
                                 const ItemLayoutInfo& post_layout_info) = 0;

    virtual void ProcessPersistent(AnimationTarget* target,
                                   const ItemLayoutInfo& pre_layout_info,
                                   const ItemLayoutInfo& post_layout_info) = 0;

    virtual void ProcessChanged(AnimationTarget* target,
                                const ItemLayoutInfo& pre_layout_info,
                                const ItemLayoutInfo& post_layout_info) = 0;

    virtual void ProcessNone(AnimationTarget* target) = 0;
  };

  ItemAnimationRecorder() = default;
  ~ItemAnimationRecorder() = default;

  ItemAnimationRecorder(const ItemAnimationRecorder&) = delete;
  ItemAnimationRecorder& operator=(const ItemAnimationRecorder&) = delete;

  void RecordPreItemLayoutInfo(AnimationTarget* target,
                               ItemLayoutInfo pre_info);

  void RecordPostLayoutInfo(AnimationTarget* target, ItemLayoutInfo post_info);

  // Moves the records currently present out of the internal map and dispatches
  // them. Records added reentrantly by callbacks remain in the map for the next
  // Process() call.
  void Process(ProcessCallback& callback);

 private:
  ItemAnimationRecord& GetOrCreateRecord(AnimationTarget* target);

  std::unordered_map<AnimationTarget*, ItemAnimationRecord> records_;
};

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_ANIMATION_ITEM_ANIMATION_RECORDER_H_
