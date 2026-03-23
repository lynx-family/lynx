// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_REG_ALLOC_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_REG_ALLOC_H_

#include <algorithm>
#include <iosfwd>

#include "core/runtime/lepus/exception.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ArrayRef.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/BitVector.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {

using llvh::ArrayRef;
using llvh::BitVector;
using llvh::DenseMap;
class CallInst;
class NewArrayInst;
class OpBuilder;

/// This is an instance of a bytecode register. It's just a wrapper around a
/// simple integer index. Register is passed by value and must remain a small
/// wrapper around an integer.
class Register {
  /// Markes unused/invalid register.
  static const unsigned InvalidRegister = ~0u;
  static const unsigned TombstoneRegister = ~0u - 1;

 public:
  static constexpr unsigned kMaxRegistersLimit = 0x100;

  explicit Register(unsigned val = InvalidRegister) : value_(val) {}

  /// \returns true if this is a valid result.
  bool IsValid() const { return value_ != InvalidRegister; }

  /// \returns the numeric value of the register.
  unsigned GetIndex() const { return value_; }

  /// Marks an empty register in the map.
  static Register GetTombstoneKey() { return Register(TombstoneRegister); }

 private:
  /// The numeric number of the register.
  unsigned value_;

 public:
  bool operator==(Register RHS) const { return value_ == RHS.value_; }
  bool operator!=(Register RHS) const { return !(*this == RHS); }

  /// \returns true if the register RHS comes right after this one.
  /// For example, R5 comes after R4.
  bool IsConsecutive(Register RHS) const {
    return GetIndex() + 1 == RHS.GetIndex();
  }

  /// \return the n'th consecutive register after the current register.
  Register GetConsecutive(unsigned count = 1) {
    return Register(GetIndex() + count);
  }

  static bool Compare(const Register& a, const Register& b) {
    return a.GetIndex() < b.GetIndex();
  }
};

std::ostream& operator<<(std::ostream& os, const Register& reg);

/// This class represents the register file. It keeps track of the currently
/// live registers and knows how to recycle registers.
class RegisterFile {
  // During allocation the register file typically grows monotonically, which
  // is how we track the max physical register usage.
  //
  // NOTE: Post-RA passes may rebuild (and shrink) it after renumbering or
  // rewrites to keep max-usage consistent. See RegisterFile::Rebuild().
  llvh::BitVector registers_;

 public:
  RegisterFile(const RegisterFile&) = delete;
  void operator=(const RegisterFile&) = delete;
  RegisterFile() = default;

  /// \returns true if the register \r is used;
  bool IsUsed(Register r);

  /// \returns true if the register \r is free;
  bool IsFree(Register r);

  /// \returns a register that's currently unused.
  Register AllocateRegister();
  Register AllocateRegister(uint32_t idx, bool check = true);
  Register AllocateRegisterCluster(uint32_t idx, unsigned count);

  /// Reserves \p n consecutive registers at the end of the register file.
  /// 'n' consecutive registers will be allocated and the first one is returned.
  Register TailAllocateConsecutive(unsigned n);

  /// Free the register \p reg and make it available for re-allocation.
  void KillRegister(Register reg);

  /// \returns the number of currently allocated registers.
  unsigned GetNumLiveRegisters() {
    return registers_.size() - registers_.count();
  }

  /// \returns the number of registers that were ever created.
  unsigned GetMaxRegisterUsage() { return registers_.size(); }

  /// \returns the max register index that's been used.
  unsigned GetMaxRegisterIndex() const {
    if (registers_.empty()) return 0;
    for (int i = registers_.size() - 1; i >= 0; --i) {
      if (!registers_.test(i)) {
        return i;
      }
    }
    return 0;
  }

  /// Rebuild the register file bitmap from a set of used registers.
  ///
  /// NOTE: This method may shrink the register file. It should only be used
  /// after register allocation is finished and no further allocate/kill
  /// operations are expected, e.g. after post-RA passes that renumber
  /// registers.
  void Rebuild(unsigned new_size, ArrayRef<unsigned> used_regs);
};

/// Segment is a value type that repreents a consecutive half-open interval in
/// the range of [start, end).
struct Segment {
  size_t start_;
  size_t end_;

  explicit Segment(size_t start, size_t end) : start_(start), end_(end) {
    if (end_ < start_) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Segment has invalid range (end < start)");
    }
  }

  /// \returns the size represented by the segment.
  size_t Size() const { return end_ - start_; }

  /// \returns true if the segment is unused.
  size_t Empty() const { return Size() == 0; }

  /// \returns true if the location \p loc falls inside the current range.
  bool Contains(size_t loc) const { return loc < end_ && loc >= start_; }

  /// \return true if the segment \p other intersects with this segment.
  bool Intersects(Segment other) const {
    return !(other.start_ >= end_ || start_ >= other.end_);
  }

  /// \return true if the segment \p other touches this segment.
  bool Touches(Segment other) const {
    return other.start_ == end_ || start_ == other.end_;
  }

  /// Join the range of the other interval into the current interval.
  void Merge(Segment other) {
    if (!(Intersects(other) || Touches(other))) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Segment::Merge merging non-overlapping segment");
    }
    start_ = std::min(start_, other.start_);
    end_ = std::max(end_, other.end_);
  }
};

std::ostream& operator<<(std::ostream& os, const Segment& segment);

/// Interval is a collection of segments represents a non-consecutive half-open
/// range.
struct Interval {
  llvh::SmallVector<Segment, 2> segments_;

  explicit Interval() = default;

  explicit Interval(size_t start, size_t end) { Add(Segment(start, end)); }

  /// \return true if this interval intersects \p other.
  bool Intersects(Segment other) const {
    for (auto& s : segments_) {
      if (s.Intersects(other)) return true;
    }
    return false;
  }

  /// \return true if this interval intersects \p other.
  bool Intersects(const Interval& other) const {
    for (auto& s : segments_) {
      if (other.Intersects(s)) return true;
    }
    return false;
  }

  /// Join the range of the other interval into the current interval.
  void Add(const Interval& other) {
    for (auto& s : other.segments_) {
      Add(s);
    }
  }

  /// Join the range of the other segment into the current interval.
  void Add(Segment other) {
    for (auto& s : segments_) {
      if (s.Intersects(other) || s.Touches(other)) {
        s.Merge(other);
        return;
      }
    }
    segments_.push_back(other);
  }

  /// \returns a new compressed interval.
  Interval Compress() const {
    Interval t;
    for (auto& s : segments_) {
      t.Add(s);
    }
    return t;
  }

  /// \returns the size represented by the interval.
  size_t Size() const {
    if (segments_.size()) return End() - Start();

    return 0;
  }

  size_t Start() const {
    if (segments_.empty()) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Interval::Start called with empty segments");
    }
    size_t start = segments_[0].start_;
    for (auto& s : segments_) {
      start = std::min(start, s.start_);
    }
    return start;
  }

  size_t End() const {
    if (segments_.empty()) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Interval::End called with empty segments");
    }
    size_t start = segments_[0].end_;
    for (auto& s : segments_) {
      start = std::max(start, s.end_);
    }
    return start;
  }
};

std::ostream& operator<<(std::ostream& os, const Interval& interval);

/// A register allocator that uses livenes information to allocate registers
/// correctly.
class RegisterAllocator {
  /// Represents the liveness info for one block.
  struct BlockLifetimeInfo {
    BlockLifetimeInfo() = default;
    void Init(unsigned size) {
      gen_.resize(size);
      kill_.resize(size);
      live_in_.resize(size);
      live_out_.resize(size);
      mask_in_.resize(size);
      // Scratch space for temporary computations, to avoid per-iteration
      // allocations in global liveness propagation.
      scratch_.resize(size);
    }
    /// Which live values are used in this block.
    BitVector gen_;
    /// Which live values are defined in this block.
    BitVector kill_;
    /// Which values are marked as live-in, coming into this basic block.
    BitVector live_in_;
    /// Which values are marked as live-in, coming out of this basic block.
    BitVector live_out_;
    /// Which values are *masked* as live-in, coming into this basic block. The
    /// mask-in bit vector is used for blocking the flow in specific blocks.
    /// We use this to block the flow of phi values and enforce flow-sensitive
    /// liveness.
    BitVector mask_in_;

    /// Temporary buffer for liveness updates.
    BitVector scratch_;
  };

  /// Returns the last index allocated.
  unsigned GetMaxInstrIndex() { return instructions_by_numbers_.size(); }

  /// Computes the liveness information for block \p bb in \p liveness_info.
  void CalculateLocalLiveness(BlockLifetimeInfo& liveness_info, Block* bb);

  /// Computes the global liveness across the whole function.
  void CalculateGlobalLiveness(ArrayRef<Block*> order);

  /// Calculates the live intervals for each instruction.
  void CalculateLiveIntervals(ArrayRef<Block*> order);

  /// Coalesce registers by merging the live intervals of multiple instructions
  /// together to take advantage of holes. Updates \p map with mapping between
  /// the coalesced interval and the interval it was merged into and the
  /// register that it will adopt.
  /// The order of the basic blocks is passed in \p order.
  void Coalesce(DenseMap<Instruction*, Instruction*>& map,
                ArrayRef<Block*> order);

  void ShuffleRegistersToMinimize();
  void RemapRegByExecOrder(ArrayRef<Block*> order,
                           DenseMap<Instruction*, Instruction*>& coalesced);

 protected:
  /// Maps active slots (per bit) for each basic block.
  llvh::DenseMap<Block*, BlockLifetimeInfo> block_liveness_;
  /// Maps index numbers to instructions.
  llvh::DenseMap<Instruction*, unsigned> instruction_numbers_;
  /// Maps instructions to a index numbers.
  llvh::SmallVector<Instruction*, 32> instructions_by_numbers_;
  /// Holds the live interval of each instruction.
  llvh::SmallVector<Interval, 32> instruction_interval_;

  /// Keeps track of the already allocated values.
  llvh::DenseMap<Value*, Register> allocated_{};

  /// The register file.
  RegisterFile file_{};

  /// If the function has fewer than this number of instructions,
  /// assign registers sequentially instead of being smart about it.
  unsigned fast_pass_threshold_ = 0;

  /// Allocate the registers for the instructions in the function in a trivial,
  /// suboptimal, but very fast way.
  void AllocateFastPass(ArrayRef<Block*> order);

  // Compute the max physical register index that is live at the call-site.
  //
  // Note: many post-RA passes may rewrite operands / delete MOVs. If we need an
  // idempotent "place callee into a fresh top reg" computation, we must exclude
  // the current callee value itself; otherwise the result would keep growing
  // (because the callee occupies the current maximum live register).
  unsigned GetTargetRegForCallFunction(Instruction* call_inst,
                                       Value* exclude = nullptr);

  FuncOp* f_;

  FuncOp* root_function_;

  llvh::DenseMap<Instruction*, unsigned> inst_to_execute_order_;

  llvh::DenseMap<Instruction*, Register> inst_to_old_reg_;

  unsigned prefix_reg_ = 0;

 public:
  using RegisterType = Register;

  /// Dump the status of the allocator in a textual form.
  void Dump(std::ostream& os);

  /// \returns the computed live interval for the instruction \p I.
  Interval& GetInstructionInterval(Instruction* i);

  explicit RegisterAllocator(FuncOp* func);

  virtual ~RegisterAllocator() = default;

  void SetFastPassThreshold(unsigned max_inst_count) {
    fast_pass_threshold_ = max_inst_count;
  }

  void InitMovInstInterval(Instruction* mov_inst, Instruction* call_inst);

  /// Special handling for call_inst.
  /// Ensures the function operand is in a register above all live registers at
  /// the call site. May reuse an existing MovInst if conditions are met.
  void ProcessCallInst(OpBuilder* builder, CallInst* call_inst);

  /// \returns the index of instruction \p I.
  unsigned GetInstructionNumber(Instruction* i);

  Instruction* GetInstructionByNumber(unsigned idx) {
    if (idx < instructions_by_numbers_.size())
      return instructions_by_numbers_[idx];
    return nullptr;
  }

  /// \returns true if the instruction \p already has a number.
  bool HasInstructionNumber(Instruction* i);

  /// Checks if the instruction \p I is manipulated by the target.
  virtual bool HasTargetSpecificLowering(Instruction* i) { return false; }

  /// \returns true if the interval for \p I is allocated manually.
  bool IsManuallyAllocatedInterval(Instruction* i);

  /// Performs target specific lowering for \p I.
  virtual void HandleInstruction(Instruction* i) {}

  /// Lower the PHI nodes in the program into a sequence of MOVs in the
  /// predecessor blocks.
  void LowerPhis(ArrayRef<Block*> order);

  /// Allocate the registers for the instructions in the function. The order of
  /// the block needs to match the order which we'll use for instruction
  /// selection.
  void Allocate(ArrayRef<Block*> order);

  /// Preallocate registers for parameters.
  void Preallocate();

  /// Reserves consecutive registers that will be manually managed by the
  /// user. \p values is a list of values to be assigned consecutive
  /// registers.
  ///  nullptr values are also allocated a register but not registered.
  /// \returns the first register in the sequence.
  Register Reserve(ArrayRef<Value*> values);

  /// Reserves \n count registers that will be manually managed by the user.
  Register Reserve(unsigned count = 1);

  Register ExtendFromLast(unsigned count = 1);

  /// \return the register allocated for the value \p v.
  Register GetRegister(Value* i);

  /// Marks the value \p as being allocated to \p R.
  void UpdateRegister(Value* i, Register r);

  /// Insert the value \p as being allocated to \p R.
  void InsertRegister(Value* i, Register r);

  /// \return true if the value \p v has been allocated.
  bool IsAllocated(Value* i);

  ///  Remove Instruction from Allocator
  void RemoveFromAllocated(Value* i);

  /// \returns the highest number of registers that are used concurrently.
  /// In here we assume that the registers are allocated consecutively
  /// and that allocating this number of registers will cover all of the
  /// registers that were allocated during the lifetime of the program.
  virtual unsigned GetMaxRegisterUsage() { return file_.GetMaxRegisterUsage(); }

  llvh::DenseMap<Value*, Register>& GetAllocatedMap() { return allocated_; }

  /// Rebuild the internal RegisterFile to reflect the current `allocated`
  /// mapping. This is useful for post-RA passes that only renumber registers
  /// (UpdateRegister/InsertRegister) and want `GetMaxRegisterUsage()` to
  /// reflect the new, compacted register indices.
  void RebuildRegisterFileFromAllocated();
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

namespace llvh {
class raw_ostream;

// Print Register to llvm debug/error streams.
raw_ostream& operator<<(raw_ostream& os, const lynx::lepus::ir::Register& reg);
raw_ostream& operator<<(raw_ostream& os,
                        const lynx::lepus::ir::Interval& interval);
raw_ostream& operator<<(raw_ostream& os,
                        const lynx::lepus::ir::Segment& segment);
template <>
struct DenseMapInfo<lynx::lepus::ir::Register> {
  static inline lynx::lepus::ir::Register getEmptyKey() {
    return lynx::lepus::ir::Register();
  }
  static inline lynx::lepus::ir::Register getTombstoneKey() {
    return lynx::lepus::ir::Register::GetTombstoneKey();
  }
  static unsigned getHashValue(lynx::lepus::ir::Register val);
  static bool isEqual(lynx::lepus::ir::Register lhs,
                      lynx::lepus::ir::Register rhs);
};

}  // namespace llvh

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_REG_ALLOC_H_
