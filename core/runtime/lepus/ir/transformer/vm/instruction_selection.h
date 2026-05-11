// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_INSTRUCTION_SELECTION_H_
#define CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_INSTRUCTION_SELECTION_H_

#include <utility>
#include <vector>

#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/pass_manager/pass.h"
#include "core/runtime/lepus/op_code.h"

namespace lynx {
namespace lepus {
namespace ir {
class RegisterAllocator;

class InstructionSelectionPass : public FunctionPass {
 public:
  struct Relocation {
    enum RelocationType : uint8_t {
      // A short jump instruction
      JumpType = 0,
      // A basic block
      BasicBlockType,
      // A catch instruction
      CatchType,
      // Debug info
      DebugInfo,
      // Jmp
      Jmp,
      // CmpJmp
      CmpJmp,
    };

    /// The current location of this relocation.
    uint32_t loc;
    /// Type of the relocation.
    RelocationType type;
    /// We multiplex pointer for different things under different types:
    /// If the type is jump, pointer is the target basic block;
    /// if the type is basic block, pointer is the pointer to it.
    /// if the type is catch instruction, pointer is the pointer to it.
    Value* pointer;
    Instruction* source_inst{nullptr};
    uint8_t lhs_reg{0};
    uint8_t rhs_reg{0};
  };

  InstructionSelectionPass(IRContext* ir_ctx);
  ~InstructionSelectionPass() override = default;
  bool RunOnFunction(FuncOp* func) override;
  void Reset(FuncOp* f);

 private:
  void Generate(Block* bb, Block* next);
  void GenerateEqNeqCondBranch(Instruction* inst, Value* lhs, Value* rhs,
                               Block* true_bb, Block* false_bb, Block* next_bb,
                               TypeOpCode true_opcode, TypeOpCode false_opcode);
  bool ResolveRelocationsInternal();
  void ResolveRelocations();
  void BytecodeGenerateComplete();
  void CompactConstPoolAndRewriteConstIndices();
  void CompactNoOpJumpsAndRewriteOffsets();
  void FixOutOfRangeCmpJmpRelocations();
  uint8_t GetLiteralUint8Value(Value* value, const char* inst_name) const;
  uint32_t GetLiteralUint32Value(Value* value, const char* inst_name,
                                 const char* value_desc) const;
  int FindCmpJmpFallbackRegister(Instruction* branch_inst, uint8_t lhs_reg,
                                 uint8_t rhs_reg) const;
  uint8_t RequireSpecialRegisterIndex(long reg) const;
  long ResolveUpvalueToplevelRegister(FuncOp* current_func, uint8_t index,
                                      const char* inst_name) const;
  long ResolveToplevelClosurePhysicalRegister(uint32_t original_closure_reg,
                                              const char* inst_name) const;
  uint32_t GetCurrentOffset() const;
  uint8_t EncodeValue(Value* value);
  void Generate(Instruction* ii, Block* next);
  void RegisterJmp(uint32_t loc, Block* target);
  void RegisterCmpJmp(uint32_t loc, Block* target, Instruction* source_inst,
                      uint8_t lhs_reg, uint8_t rhs_reg);
  lynx::lepus::Instruction* GetInstruction(uint32_t loc);

  static bool IsInCmpJmpRange(int32_t diff);

  // Returns true if this CallInst is lowered to TypeOp_DeepClone.
  bool TryLowerDeepCloneCall(CallInst* inst, uint8_t ret_reg);

  /// This is the header declaration for all of the methods that emit opcodes
/// for specific high-level IR instructions.
#define ENABLE_MIR_INSTR 1
#define DEF_VALUE(CLASS, PARENT) void Generate##CLASS(CLASS* inst, Block* next);
#define BEGIN_VALUE(CLASS, PARENT) DEF_VALUE(CLASS, PARENT)

#include "core/runtime/lepus/ir/value_kinds.def"

#undef DEF_VALUE
#undef ENABLE_MIR_INSTR

  fml::RefPtr<Function> lepus_function_;
  FuncOp* func_op_;
  base::Vector<lynx::lepus::Instruction> op_codes_;
  std::vector<int64_t> debug_line_col_;
  llvh::SmallVector<Relocation, 8> relocations_{};
  llvh::DenseMap<Block*, std::pair<uint32_t, Block*>> basic_block_map_{};
  RegisterAllocator* ra_;
  bool root_func_deopt_ = false;
  long extra_temp_max_reg_index_ = -1;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_TRANSFORMER_VM_INSTRUCTION_SELECTION_H_
