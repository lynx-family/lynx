// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_HEAP_H_
#define CORE_RUNTIME_VM_LEPUS_HEAP_H_

#include <vector>

#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/runtime/vm/lepus/op_code.h"

namespace lynx {
namespace lepus {
class Heap {
 public:
  Heap() : heap_(kBaseHeapSize) { top_ = &heap_[0]; }
  Value* top_;
  Value* base() { return &heap_[0]; }

 private:
  friend class ContextBinaryWriter;
  static constexpr int kBaseHeapSize = 10240;
  std::vector<Value> heap_;
};

struct Frame {
  Value* register_;

  Value* function_;

  Value* return_;

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

  Frame(Value* reg, Value* function, Value* ret, const Instruction* ins,
        const Instruction* end, Frame* prev_frame, int current_pc)
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

#endif  // CORE_RUNTIME_VM_LEPUS_HEAP_H_
