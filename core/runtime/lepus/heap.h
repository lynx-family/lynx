// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_HEAP_H_
#define CORE_RUNTIME_LEPUS_HEAP_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "core/runtime/lepus/op_code.h"
#include "core/runtime/lepus/restricted_value.h"

namespace lynx {
namespace lepus {
class Heap {
 public:
  Heap() : heap_(kBaseHeapSize) { top_ = heap_.data(); }

  bool ContainsRange(const RestrictedValue* begin, size_t slot_count) const {
    const auto heap_begin = reinterpret_cast<uintptr_t>(heap_.data());
    const auto heap_end = heap_begin + heap_.size() * sizeof(RestrictedValue);
    const auto range_begin = reinterpret_cast<uintptr_t>(begin);
    if (range_begin < heap_begin || range_begin > heap_end ||
        (range_begin - heap_begin) % sizeof(RestrictedValue) != 0) {
      return false;
    }
    return slot_count <= (heap_end - range_begin) / sizeof(RestrictedValue);
  }

  RestrictedValue* Allocate(size_t slot_count) {
    if (!ContainsRange(top_, slot_count)) {
      return nullptr;
    }
    auto* result = top_;
    top_ += slot_count;
    return result;
  }

  size_t capacity() const { return heap_.size(); }
  RestrictedValue* top_;
  RestrictedValue* base() { return heap_.data(); }

 private:
  friend class ContextBinaryWriter;
  static constexpr int kBaseHeapSize = 10240;
  std::vector<RestrictedValue> heap_;
};

struct Frame {
  RestrictedValue* register_;

  RestrictedValue* function_;

  RestrictedValue* return_;

  const Instruction* instruction_;

  const Instruction* end_;

  Frame* prev_frame_;

  int current_pc_;

  // for lepus debugger
  int32_t debugger_frame_id_;

  Frame()
      : register_(nullptr),
        function_(nullptr),
        return_(nullptr),
        instruction_(nullptr),
        end_(nullptr),
        prev_frame_(nullptr),
        current_pc_(0) {}

  Frame(RestrictedValue* reg, RestrictedValue* function, RestrictedValue* ret,
        const Instruction* ins, const Instruction* end, Frame* prev_frame,
        int current_pc)
      : register_(reg),
        function_(function),
        return_(ret),
        instruction_(ins),
        end_(end),
        prev_frame_(prev_frame),
        current_pc_(current_pc) {}
  void SetDebuggerFrameId(int32_t id) { debugger_frame_id_ = id; }
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_HEAP_H_
