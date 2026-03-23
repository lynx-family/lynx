// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_SIDE_EFFECT_H_
#define CORE_RUNTIME_LEPUS_IR_SIDE_EFFECT_H_
namespace lynx {
namespace lepus {
namespace ir {
/// Describes the potential side effects of an instruction. The side effects are
/// described by a series of bits, each of which specifies a particular way in
/// which this instruction may observe/modify the state of the world, or
/// otherwise restrict its movement/deletion.
class SideEffect {
 public:
  SideEffect() { memset(this, 0, sizeof(SideEffect)); }

  /// Define each of the fields in a SideEffect.
  /// \c ReadStack and \c WriteStack imply that an instruction may read/write to
  /// a stack location. Such an instruction must take an \c AllocStackInst
  /// operand which it can read/write to.
  /// \c ReadFrame and \c WriteFrame imply that an instruction may read/write
  /// variables in the frame.
  /// \c ReadHeap and \c WriteHeap imply that an instruction may read/write
  /// objects and other values on the heap.
  /// \c Throw implies that an instruction may throw an exception.
  /// \c ExecuteJS implies that an instruction may execute arbitrary user
  /// provided JS. An instruction with \c ExecuteJS set must also have bits set
  /// for throwing and accessing the frame and heap.
  /// \c FirstInBlock implies that an instruction must be at the start of a
  /// basic block. An instruction with \c FirstInBlock set must precede all
  /// instructions that do not have it set.
  /// \c Idempotent implies that repeated execution of an instruction will not
  /// affect the state of the world or the result of the instruction. For
  /// example, two identical instructions that have \p Idempotent set may be
  /// merged if they are only separated by instructions that have no side
  /// effects.

#define SIDE_EFFECT_FIELDS \
  FIELD(ReadStack)         \
  FIELD(WriteStack)        \
  FIELD(ReadFrame)         \
  FIELD(WriteFrame)        \
  FIELD(ReadHeap)          \
  FIELD(WriteHeap)         \
  FIELD(Throw)             \
  FIELD(ExecuteJS)         \
  FIELD(FirstInBlock)      \
  FIELD(IsCatch)           \
  FIELD(ChangeType)        \
  FIELD(Idempotent)

#define FIELD(field) \
  bool Get##field() const { return f##field##_; }
  SIDE_EFFECT_FIELDS
#undef FIELD

#define FIELD(field)         \
  SideEffect& Set##field() { \
    f##field##_ = true;      \
    return *this;            \
  }
  SIDE_EFFECT_FIELDS
#undef FIELD

  /// Create side effects for an instruction that may execute user JS. Executing
  /// JS implies several other side-effects, so this helps populate them.
  static SideEffect CreateExecute() {
    return SideEffect{}
        .SetReadFrame()
        .SetReadHeap()
        .SetWriteFrame()
        .SetWriteHeap()
        .SetExecuteJS()
        .SetThrow();
  }

  /// Determine whether the given instruction modifies the state of the world in
  /// any an observable way.
  bool HasSideEffect() const {
    // Note that we do not check ExecuteJS here because any observable
    // side-effect of a call requires one of these other bits to be set.
    return GetWriteStack() || GetWriteHeap() || GetWriteFrame() || GetThrow() ||
           GetIsCatch();
  }

  /// Determine if the instruction is pure. That is, whether separate
  /// invocations of the instruction with the same operands will always yield
  /// the same result, and will not modify the state of the world in any
  /// observable way. Note that this excludes instructions that allocate (e.g.
  /// AllocObject, CreateFunction) since they produce a different reference each
  /// time.
  bool IsPure() const {
    return !HasSideEffect() && !GetReadStack() && !GetReadHeap() &&
           !GetReadFrame() && !GetIsCatch() && GetIdempotent();
  }

  /// Helper functions to expose the SideEffectKind interface.
  bool MayExecute() const { return GetThrow() || GetExecuteJS(); }
  bool MayWriteOrWorse() const {
    return GetWriteStack() || GetWriteFrame() || GetWriteHeap() || MayExecute();
  }
  bool MayReadOrWorse() const {
    return GetReadStack() || GetReadFrame() || GetReadHeap() ||
           MayWriteOrWorse();
  }

 private:
#define FIELD(field) bool f##field##_ : 1;
  SIDE_EFFECT_FIELDS
#undef FIELD
#undef SIDE_EFFECT_FIELDS
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_SIDE_EFFECT_H_
