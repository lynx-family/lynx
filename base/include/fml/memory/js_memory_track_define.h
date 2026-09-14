// Copyright 2026 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_MEMORY_JS_MEMORY_TRACK_DEFINE_H_
#define BASE_INCLUDE_FML_MEMORY_JS_MEMORY_TRACK_DEFINE_H_

#include "base/include/vector.h"

namespace lynx {
namespace fml {

struct JSMemoryTrackSlotType {
  enum {
    Unknown = 0,
    Common = 1,  // Runtime-wide common memory such as such as the engine object
                 // itself or common scripts like lynx_core.js
    Overflow = 2,  // When slots of a VM are exhausted, subsequent pages use
                   // this slot to record memory.
    Max = 255,
    Count = 256,
  };
};

inline bool IsValidInstanceSlot(int32_t slot) {
  return slot > fml::JSMemoryTrackSlotType::Overflow &&
         slot < fml::JSMemoryTrackSlotType::Count;
}

class JSMemoryTrackOwner {
 public:
  int32_t& GetCurrentAllocSlot() { return current_alloc_slot_; }

  void PushCurrentAllocSlot(int32_t value) {
    alloc_pointer_stack_++;
    current_alloc_slot_ = value;
  }

  void PopCurrentAllocSlot() {
    assert(alloc_pointer_stack_ > 0);
    if (--alloc_pointer_stack_ == 0) {
      current_alloc_slot_ = JSMemoryTrackSlotType::Unknown;
    }
  }

  void SetInstanceMemorySlot(int32_t instance_id, int32_t slot) {
    assert(slot >= 0 && slot <= fml::JSMemoryTrackSlotType::Max);
    instance_memory_slots_[instance_id] = current_alloc_slot_ = slot;
  }

  int32_t GetInstanceMemorySlot(int32_t instance_id) const {
    auto it = instance_memory_slots_.find(instance_id);
    return it == instance_memory_slots_.end()
               ? fml::JSMemoryTrackSlotType::Unknown
               : it->second;
  }

  void RemoveInstanceMemorySlot(int32_t instance_id) {
    instance_memory_slots_.erase(instance_id);
  }

 protected:
  int32_t alloc_pointer_stack_ = 0;
  int32_t current_alloc_slot_ = JSMemoryTrackSlotType::Unknown;
  base::LinearFlatMap<int32_t, int32_t> instance_memory_slots_;
};

}  // namespace fml
}  // namespace lynx

#endif  // BASE_INCLUDE_FML_MEMORY_JS_MEMORY_TRACK_DEFINE_H_
