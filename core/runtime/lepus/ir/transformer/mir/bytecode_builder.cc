// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/mir/bytecode_builder.h"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/runtime/lepus/ir/dialects/mir/mir_instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ArrayRef.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseMap.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/Support/Casting.h"
#include "core/runtime/lepus/ir/transformer/mir/bytecode_analysis.h"
#include "core/runtime/lepus/ir/transformer/mir/bytecode_iterator.h"
#include "core/runtime/lepus/ir/type_op.h"
#include "core/runtime/lepus/ir/utils/block_utils.h"
#include "core/runtime/lepus/vm_context.h"

namespace lynx {
namespace lepus {
namespace ir {

class OpBuilder;

class Environment {
 public:
  int register_count_;
  int param_count_;
  llvh::SmallVector<Value*, 16> values_;
  OpBuilder* builder_ = nullptr;
  uint32_t current_index_;
  int env_current_offset_;

  Environment(OpBuilder* builder, int register_count, int param_count)
      : register_count_(register_count),
        param_count_(param_count),
        builder_(builder),
        current_index_(0),
        env_current_offset_(0) {
    values_.resize(register_count_, nullptr);
  }

  void SetValue(int index, Value* value) {
    if (LEPUS_UNLIKELY(index < 0 ||
                       static_cast<size_t>(index) >= values_.size())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Environment::SetValue index out of range");
    }
    values_[index] = value;
  }

  Value* GetValue(int index) const {
    if (LEPUS_UNLIKELY(index < 0 ||
                       static_cast<size_t>(index) >= values_.size())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Environment::GetValue index out of range");
    }
    return values_[index];
  }

  void Merge(const std::shared_ptr<Environment>& other, Block* other_bb,
             const std::shared_ptr<llvh::BitVector>& live_in) {
    for (int i : live_in->set_bits()) {
      auto value = values_[i];
      auto other_value = other->values_[i];
      if (LEPUS_UNLIKELY(value == nullptr || other_value == nullptr)) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: Environment::Merge encountered unbound register "
            "value");
      }
      values_[i] = MergeValue(value, other_value, other_bb);
    }
  }

  Value* MergeValue(Value* value, Value* other, Block* other_bb) {
    if (LEPUS_UNLIKELY(!llvh::isa<PhiInst>(value))) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Environment::MergeValue expects current value to be "
          "a PhiInst");
    }

    bool already_in_phi = false;

    auto* phi_inst = llvh::dyn_cast<PhiInst>(value);
    for (auto i = 0; i < phi_inst->GetNumEntries(); i++) {
      auto value = phi_inst->GetEntry(i).first;
      auto bb = phi_inst->GetEntry(i).second;

      if (value == other && bb == other_bb) {
        already_in_phi = true;
        break;
      }
    }

    if (!already_in_phi && other != phi_inst)
      phi_inst->AddEntry(other, other_bb);
    return value;
  }

  std::shared_ptr<Environment> Clone() {
    auto env =
        std::make_shared<Environment>(builder_, register_count_, param_count_);
    env->CopyValues(this);
    return env;
  }

  void CopyValues(Environment* env) {
    values_ = env->values_;
    env_current_offset_ = env->env_current_offset_;
  }
};

class BytecodeBuilderImpl {
 private:
  IRContext* ir_ctx_;
  FuncOp* func_op_;
  std::shared_ptr<Environment> current_env_;
  llvh::DenseMap<int, std::shared_ptr<Environment>> merge_envs_;
  BytecodeAnalysis analysis_;
  int register_count_;
  int parameter_count_;
  OpBuilder* builder_;
  BytecodeIterator bytecode_iterator_;
  llvh::DenseMap<int, Block*> bb_infos_;
  llvh::DenseMap<int, int> reg_to_toplevel_offset_;
  // Sorted offsets of TypeLabel_Catch in the original bytecode.
  llvh::SmallVector<int, 8> catch_label_offsets_;

  static void CheckToplevelVarOffsetMapping(
      VMContext* vm_context,
      const std::unordered_map<base::String, long>& toplevel_vars) {
    for (const auto& item : toplevel_vars) {
      const long reg = item.second;
      auto offset = vm_context->GetToplevelVarOffset(reg);
      if (offset == -1) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: missing toplevel var first-write offset mapping");
      }
    }
  }

  void InitToplevelVarOffsetMappingIfNeeded() {
    if (!func_op_->IsToplevelFunc()) return;
    auto* vm_context = ir_ctx_->GetVMContext();
    const auto& toplevel_vars = vm_context->GetToplevelVariables();
    CheckToplevelVarOffsetMapping(vm_context, toplevel_vars);
    for (const auto& item : toplevel_vars) {
      reg_to_toplevel_offset_[static_cast<int>(item.second)] =
          vm_context->GetToplevelVarOffset(static_cast<int>(item.second));
    }
  }

 public:
  BytecodeBuilderImpl(IRContext* ir_ctx, FuncOp* func, int32_t register_count,
                      int32_t param_count)
      : ir_ctx_(ir_ctx),
        func_op_(func),
        analysis_(func, register_count),
        register_count_(register_count),
        parameter_count_(param_count),
        builder_(ir_ctx->GetOpBuilder()) {
    InitToplevelVarOffsetMappingIfNeeded();
  }

  OpBuilder* builder() { return builder_; }

  BytecodeIterator& iterator() { return bytecode_iterator_; }

  int FindNextCatchLabelOffset(int current_offset) const {
    auto it = std::upper_bound(catch_label_offsets_.begin(),
                               catch_label_offsets_.end(), current_offset);
    if (it == catch_label_offsets_.end()) return -1;
    return *it;
  }

  std::shared_ptr<Environment> environment() const { return current_env_; }
  void SetEnvironment(std::shared_ptr<Environment> env) { current_env_ = env; }

  void RecordToplevelVarReg(int reg, Value* val) {
    // for toplevel variables in code_gen, we should not change it. so we record
    // those toplevel varialbes, and assign fixed reg in reg alloc phase

    if (func_op_->IsToplevelFunc()) {
      if (val->GetToplevelVarReg() != constants::kInvalidSignedValue) return;

      auto iter = reg_to_toplevel_offset_.find(reg);
      if (iter != reg_to_toplevel_offset_.end() &&
          iter->second <= iterator().GetCurrentOffset()) {
        if (LEPUS_UNLIKELY(!llvh::isa<Instruction>(val))) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: RecordToplevelVarReg expects Value to be an "
              "Instruction");
        }
        ir_ctx_->InsertToplevelValue(llvh::cast<Instruction>(val), reg);
      }
    }
  }

  void BindRegister(int reg, Value* val) {
    if (val) {
      current_env_->SetValue(reg, val);
      RecordToplevelVarReg(reg, val);
    }
  }

  Value* GetRegister(int reg, bool strict = true) {
    auto res = current_env_->GetValue(reg);
    if (strict && LEPUS_UNLIKELY(res == nullptr)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BytecodeBuilder register not bound");
    }
    return res;
  }

  void MergeFallThrough(int current_offset) {
    if (!analysis_.IsBBHeader(current_offset)) {
      return;
    }

    if (current_env_ != nullptr) {
      // fall through
      MergeEnvironment(current_offset);
    }
  }

  void GeneratePhiInst(int current_offset, int target_offset) {
    auto* current_bb = GetBasicBlock(current_offset);
    auto* target_bb = GetBasicBlock(target_offset);
    auto merge_env = GetOrCreateEnvironment(target_offset);
    OpBuilderRestoreInsertPointerRAII guard(builder());
    {
      builder()->SetInsertionPointToStart(target_bb);
      for (int i : analysis_.GetLiveIn(target_offset)->set_bits()) {
        auto* phi_inst = builder()->Create<PhiInst>(0, PhiInst::ValueListType(),
                                                    PhiInst::BlockListType());
        phi_inst->SetType(TypeOp::CreateAnyType(builder()));
        auto val = current_env_->values_[i];
        if (val && val != phi_inst) {
          phi_inst->AddEntry(val, current_bb);
        }
        merge_env->values_[i] = phi_inst;
      }
    }
  }

  void MergeEnvironment(int target_offset) {
    auto merge_env = GetOrCreateEnvironment(target_offset);
    if (!merge_env || !current_env_) return;

    if (merge_env->current_index_ == 0) {
      if (analysis_.IsLoopHeader(target_offset) ||
          analysis_.MultipleIncoming(target_offset)) {
        auto current_offset = current_env_->env_current_offset_;
        GeneratePhiInst(current_offset, target_offset);
      }
    } else if (analysis_.MultipleIncoming(target_offset)) {
      auto current_offset = current_env_->env_current_offset_;
      auto other_bb = GetBasicBlock(current_offset);
      auto live_in = analysis_.GetLiveIn(target_offset);
      merge_env->Merge(current_env_, other_bb, live_in);
    }
    merge_env->current_index_++;
  }

  std::shared_ptr<Environment> GetOrCreateEnvironment(int target_offset) {
    auto it = merge_envs_.find(target_offset);
    if (it != merge_envs_.end()) return it->second;

    if (LEPUS_UNLIKELY(current_env_ == nullptr)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BytecodeBuilder cannot clone environment before "
          "initialization");
    }
    auto env = current_env_->Clone();
    env->env_current_offset_ = target_offset;
    merge_envs_.insert({target_offset, env});
    return env;
  }

  Block* GetBasicBlock(int current_offset) {
    auto it = bb_infos_.find(current_offset);
    if (LEPUS_UNLIKELY(it == bb_infos_.end() || !it->second)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BytecodeBuilder requested basic block before "
          "creation");
    }
    return it->second;
  }

  Block* CreateBasicBlock(int current_offset) {
    if (LEPUS_UNLIKELY(bb_infos_.find(current_offset) != bb_infos_.end())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BytecodeBuilder attempted to create duplicate basic "
          "block");
    }

    auto* bb =
        builder()->CreateBlock(builder()->GetRegion(), BlockType::BT_INST,
                               iterator().GetCurrentLineCol());
    bb_infos_.insert({current_offset, bb});
    return bb;
  }

  void SwitchToTargetEnv(int current_offset) {
    if (!analysis_.IsBBHeader(current_offset)) return;

    if (analysis_.IsBBHeader(current_offset)) {
      // need to generate basic block(each basic block has a terminatorInst)
      auto* bb = GetBasicBlock(current_offset);
      InsertAfterPhi(builder(), bb);
    }
    auto cur_env = environment();
    auto merge_env = GetOrCreateEnvironment(current_offset);

    if (cur_env != nullptr) {
      cur_env->CopyValues(merge_env.get());
    } else {
      SetEnvironment(merge_env->Clone());
    }
  }

  void CreateAllBasicBlocks(int bytecode_size) {
    auto* entry_bb = func_op_->GetSingleRegion()->GetEntryBlock();

    auto* cur_bb = entry_bb;

    // Use analysis results to avoid scanning all bytecode instructions.
    // The analysis may contain a synthetic end block at offset==bytecode_size.
    auto headers = analysis_.GetBBHeaderOffsets();
    for (int offset : headers) {
      if (offset <= 0) continue;
      if (offset >= bytecode_size) continue;

      iterator().SkipTo(offset);
      auto* bb = CreateBasicBlock(offset);
      {
        OpBuilderRestoreInsertPointerRAII guard(builder());
        builder()->SetInsertionPointToEnd(cur_bb);
        // connect to next bb for now.
        builder()->Create<BranchInst>(0, bb);
      }
      cur_bb = bb;
    }
  }

  void VisitInstructions(lynx::lepus::Instruction* ptr, size_t size,
                         llvh::ArrayRef<int64_t> line_cols) {
    auto end = ptr + size;
    // printFuncInstruction(ptr, size);

    // Pre-scan to record all catch label offsets so we can attach the correct
    // handler successor for ThrowInst.
    catch_label_offsets_.clear();
    {
      BytecodeIterator scan;
      scan.Reset(ptr, end, line_cols);
      while (!scan.Done()) {
        if (scan.GetOpcode() == LepusOpcode::OP_TypeLabel_Catch) {
          catch_label_offsets_.push_back(scan.GetCurrentOffset());
        }
        scan.Next();
      }
    }

    analysis_.Run(ptr, end, line_cols);
    iterator().Reset(ptr, end, line_cols);

    CreateAllBasicBlocks(static_cast<int>(size));

    iterator().Reset(ptr, end, line_cols);

    while (!iterator().Done()) {
      auto offset = iterator().GetCurrentOffset();

      SwitchToTargetEnv(offset);

      auto opcode = iterator().GetOpcode();
      switch (opcode) {
        case LepusOpcode::OP_PLACEHOLDER:
          BuildOp_Placeholder();
          break;
#define DEF_OPCODE(name)       \
  case LepusOpcode::OP_##name: \
    Build##name();             \
    break;
#include "core/runtime/lepus/lepus_bytecode_def.h"
#undef DEF_OPCODE
        default:
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: BytecodeBuilder encountered unknown opcode");
      }
      iterator().Next();

      if (!Bytecode::IsTerminate(opcode)) {
        auto next_offset = iterator().GetCurrentOffset();
        MergeFallThrough(next_offset);
      }
    }
  }

  void Build(lynx::lepus::Instruction* ptr, int size,
             llvh::ArrayRef<int64_t> line_cols) {
    bb_infos_.clear();
    merge_envs_.clear();

    auto* entry_block = func_op_->GetSingleRegion()->GetEntryBlock();
    if (LEPUS_UNLIKELY(!entry_block)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BytecodeBuilder expected non-null entry block");
    }
    bb_infos_.insert({0, entry_block});

    auto env = std::make_shared<Environment>(builder(), register_count_,
                                             parameter_count_);
    SetEnvironment(env);

    builder_->SetInsertionPointToEnd(entry_block);
    for (int i = 0; i < parameter_count_; i++) {
      auto param = func_op_->CreateParam(i);
      BindRegister(i, param);
    }
    VisitInstructions(ptr, size, line_cols);
    // createEndBB??
  }

  void BuildOp_Placeholder() {
    builder()->Create<FunctionEndInst>(iterator().GetCurrentLineCol());
  }

  void BuildTypeOp_LoadNil() {
    auto dst_reg = iterator().GetOperand0();
    auto type = iterator().GetOperand1();
    auto loc = iterator().GetCurrentLineCol();

    if (type == 0 || type == 1) {
      if (ir_ctx_->GetVMContext()->GetNullPropAsUndef() == false) {
        // if GetNullPropAsUndef() is false, lepus vm treats undefined as null.
        // we use null instead.
        BindRegister(dst_reg, builder()->Create<LoadNullOrUndefinedInst>(
                                  loc, builder()->GetLiteralInt8(0)));
      } else {
        BindRegister(dst_reg, builder()->Create<LoadNullOrUndefinedInst>(
                                  loc, builder()->GetLiteralInt8(type)));
      }
    } else if (type == 2) {
      BindRegister(dst_reg, builder()->Create<LoadToplevelVarsInst>(loc));
    } else if (type == 3) {
      BindRegister(dst_reg, builder()->Create<GetGlobalLynxInst>(loc));
    } else {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BuildTypeOp_LoadNil got invalid type operand");
    }
  }

  void BuildTypeOp_LoadConst() {
    auto dst = iterator().GetOperand0();
    auto idx = iterator().GetOperand1x();

    auto lepus_func = func_op_ ? func_op_->GetLepusFunction() : nullptr;
    if (!lepus_func || idx >= lepus_func->GetConstValue().size()) {
      std::string msg =
          std::string(
              "Lepus IR error: BuildTypeOp_LoadConst got out-of-range "
              "const index: ") +
          std::to_string(idx);
      throw ::lynx::lepus::CompileException(msg.c_str());
    }
    auto* lepus_val = lepus_func->GetConstValue(idx);
    BindRegister(dst, builder()->Create<LoadConstInst>(
                          iterator().GetCurrentLineCol(),
                          builder()->GetLiteralUint32(idx),
                          TypeOp::GetValueTypeOp(builder(), lepus_val)));
  }

  void BuildTypeOp_Move() {
    auto dst = iterator().GetOperand0();
    auto src_reg = iterator().GetOperand1();
    auto src_val = GetRegister(src_reg);

    // Check if dst is a toplevel closure variable and mark it
    if (func_op_->IsToplevelFunc()) {
      // Always create MovInst for Move bytecode in toplevel function.
      // A later pass (ProcessSpecialMovPass) will remove normal MovInst and
      // lower special ones into toplevel/closure var loads and stores.
      auto* mov_inst =
          builder()->Create<MovInst>(iterator().GetCurrentLineCol(), src_val);

      auto& toplevel_closure_regs = func_op_->GetToplevelClosureVarRegs();
      if (toplevel_closure_regs.find(dst) != toplevel_closure_regs.end()) {
        // Mark that this MovInst's destination is a toplevel closure variable.
        mov_inst->SetClosureVarReg(dst);
      }
      BindRegister(dst, mov_inst);
    } else {
      BindRegister(dst, src_val);
    }
  }
  void BuildTypeOp_GetUpvalue() {
    auto dst_reg = iterator().GetOperand0();
    auto upvalue_index = iterator().GetOperand1();

    auto* get_upvalue_inst = builder()->Create<GetUpvalueInst>(
        iterator().GetCurrentLineCol(), func_op_,
        builder()->GetLiteralUint8(upvalue_index));
    BindRegister(dst_reg, get_upvalue_inst);
  }

  void BuildTypeOp_SetUpvalue() {
    auto src = GetRegister(iterator().GetOperand0());
    auto upvalue_index = iterator().GetOperand1();

    builder()->Create<SetUpvalueInst>(iterator().GetCurrentLineCol(), func_op_,
                                      builder()->GetLiteralUint8(upvalue_index),
                                      src);
  }

  void BuildTypeOp_GetGlobal() {
    auto dst_reg = iterator().GetOperand0();

    auto global_index = iterator().GetOperand1x();
    TypeOp* global_type = TypeOp::GetValueTypeOp(
        builder(), ir_ctx_->GetVMContext()->global()->Get(global_index));

    auto* get_global_inst = builder()->Create<GetGlobalInst>(
        iterator().GetCurrentLineCol(),
        builder()->GetLiteralUint32(global_index), global_type);
    BindRegister(dst_reg, get_global_inst);
  }

  void BuildTypeOp_SetGlobal() {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: bytecode SetGlobal is not supported in MIR builder");
  }

  void RecordClosureReg(long func_index) {
    if (!func_op_->IsToplevelFunc()) return;

    auto child_lepus_func =
        func_op_->GetLepusFunction()->GetChildFunction(func_index);
    if (LEPUS_UNLIKELY(func_op_->GetLepusFunction()->GetFunctionName() !=
                       "<anonymous>")) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: RecordClosureReg expects toplevel function name to "
          "be <anonymous>");
    }

    for (int i = 0, e = child_lepus_func->UpvaluesSize(); i < e; i++) {
      auto info = child_lepus_func->GetUpvalue(i);
      if (info->in_parent_vars_) {
        auto* closure_val = GetRegister(info->register_);
        if (closure_val) {
          closure_val->SetClosureVarReg(info->register_);
          // closure var may change type any place. set anyType
          if (!llvh::isa<LoadConstInst>(closure_val)) {
            closure_val->SetType(TypeOp::CreateAnyType(builder()));
          }
          func_op_->RecordClosureVarRegAndValue(info->register_, closure_val);
        }
      } else {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: RecordClosureReg encountered unexpected upvalue "
            "scope");
      }
    }
  }

  void BuildTypeOp_Closure() {
    auto dst_reg = iterator().GetOperand0();
    auto func_index = iterator().GetOperand1x();
    auto* create_closure_inst = builder()->Create<CreateClosureInst>(
        iterator().GetCurrentLineCol(), func_index);
    BindRegister(dst_reg, create_closure_inst);

    RecordClosureReg(func_index);
  }

  void BuildTypeOp_Call() {
    auto dst_reg = iterator().GetOperand2();
    auto func_reg = GetRegister(iterator().GetOperand0());
    auto argc = iterator().GetOperand1();

    ArgList args;
    args.reserve(argc);
    for (size_t i = 0; i < argc; i++) {
      args.push_back(GetRegister(iterator().GetOperand0() + i + 1));
    }

    auto* call_inst = builder()->Create<CallInst>(
        iterator().GetCurrentLineCol(), func_reg, args);
    BindRegister(dst_reg, call_inst);
  }

  void BuildTypeOp_Ret() {
    auto* bb = builder()->GetBlock();
    if (LEPUS_UNLIKELY(!bb || !bb->HasTerminalInst())) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: BuildTypeOp_Ret expects current block to have a "
          "terminator");
    }
    auto original_terminator = bb->GetTerminator();

    [[maybe_unused]] auto* return_inst = builder()->Create<ReturnInst>(
        iterator().GetCurrentLineCol(), GetRegister(iterator().GetOperand0()));
    original_terminator->EraseFromParent();
    // exit_list_.push_back(return_inst);
  }

  void BuildTypeOp_JmpTrue() {
    auto src_val = GetRegister(iterator().GetOperand0());
    int target_offset = iterator().GetCurrentOffset() + iterator().JumpOffset();
    auto* cur_bb = builder()->GetBlock();

    MergeEnvironment(target_offset);
    auto* target_bb = GetBasicBlock(target_offset);
    {
      OpBuilderRestoreInsertPointerRAII guard(builder());
      builder()->SetInsertionPointToEnd(cur_bb);
      if (auto* terminator = cur_bb->GetTerminator()) {
        if (terminator->GetKind() == ValueKind::BranchInstKind) {
          auto* branch_inst = static_cast<BranchInst*>(terminator);
          auto* next_bb = branch_inst->GetBranchDest();
          auto* cond_br = builder()->Create<CondBranchInst>(
              iterator().GetCurrentLineCol(), src_val, target_bb, next_bb);
          cond_br->SetSmallJmp(iterator().JumpOffset() >= -128 &&
                               iterator().JumpOffset() <= 127);
          branch_inst->EraseFromParent();
        } else {
          // Handle other terminators or create new one
          auto* next_bb = GetBasicBlock(iterator().GetCurrentOffset() + 1);
          auto* cond_br = builder()->Create<CondBranchInst>(
              iterator().GetCurrentLineCol(), src_val, target_bb, next_bb);
          cond_br->SetSmallJmp(iterator().JumpOffset() >= -128 &&
                               iterator().JumpOffset() <= 127);
        }
      } else {
        // If no terminator exists, create a CondBranchInst and potentially a
        // placeholder for false branch
        auto* next_bb = GetBasicBlock(iterator().GetCurrentOffset() + 1);
        auto* cond_br = builder()->Create<CondBranchInst>(
            iterator().GetCurrentLineCol(), src_val, target_bb, next_bb);
        cond_br->SetSmallJmp(iterator().JumpOffset() >= -128 &&
                             iterator().JumpOffset() <= 127);
      }
    }
  }

  void BuildTypeOp_JmpFalse() {
    auto src_val = GetRegister(iterator().GetOperand0());
    int target_offset = iterator().GetCurrentOffset() + iterator().JumpOffset();
    auto* cur_bb = builder()->GetBlock();

    MergeEnvironment(target_offset);
    auto* target_bb = GetBasicBlock(target_offset);
    {
      OpBuilderRestoreInsertPointerRAII guard(builder());
      builder()->SetInsertionPointToEnd(cur_bb);
      if (auto* terminator = cur_bb->GetTerminator()) {
        if (terminator->GetKind() == ValueKind::BranchInstKind) {
          auto* branch_inst = static_cast<BranchInst*>(terminator);
          auto* next_bb = branch_inst->GetBranchDest();
          auto* cond_br = builder()->Create<CondBranchInst>(
              iterator().GetCurrentLineCol(), src_val, next_bb, target_bb);
          cond_br->SetSmallJmp(iterator().JumpOffset() >= -128 &&
                               iterator().JumpOffset() <= 127);
          branch_inst->EraseFromParent();
        } else {
          auto* next_bb = GetBasicBlock(iterator().GetCurrentOffset() + 1);
          auto* cond_br = builder()->Create<CondBranchInst>(
              iterator().GetCurrentLineCol(), src_val, next_bb, target_bb);
          cond_br->SetSmallJmp(iterator().JumpOffset() >= -128 &&
                               iterator().JumpOffset() <= 127);
        }
      } else {
        auto* next_bb = GetBasicBlock(iterator().GetCurrentOffset() + 1);
        auto* cond_br = builder()->Create<CondBranchInst>(
            iterator().GetCurrentLineCol(), src_val, next_bb, target_bb);
        cond_br->SetSmallJmp(iterator().JumpOffset() >= -128 &&
                             iterator().JumpOffset() <= 127);
      }
    }
  }

  void BuildTypeOp_Jmp() {
    int target_offset = iterator().GetCurrentOffset() + iterator().JumpOffset();
    MergeEnvironment(target_offset);
    auto* bb = builder()->GetBlock();
    auto* target_bb = GetBasicBlock(target_offset);
    if (auto* terminator = bb->GetTerminator()) {
      if (terminator->GetKind() == ValueKind::BranchInstKind) {
        static_cast<BranchInst*>(terminator)->SetBranchDest(target_bb);
      } else {
        builder()->Create<BranchInst>(iterator().GetCurrentLineCol(),
                                      target_bb);
      }
    } else {
      builder()->Create<BranchInst>(iterator().GetCurrentLineCol(), target_bb);
    }
  }

  void BuildCompare(LepusCmpKind cmp_kind) {
    auto dst_reg = iterator().GetOperand0();
    auto* left_val = GetRegister(iterator().GetOperand1());
    auto* right_val = GetRegister(iterator().GetOperand2());
    TypeOp* dst_type = TypeOp::CreateBoolean(builder());

    ValueKind kind;
    switch (cmp_kind) {
      case LepusCmpKind::Eq:
        kind = ValueKind::BinaryStrictlyEqualInstKind;
        break;
      case LepusCmpKind::Neq:
        kind = ValueKind::BinaryStrictlyNotEqualInstKind;
        break;
      case LepusCmpKind::Lt:
        kind = ValueKind::BinaryLessThanInstKind;
        break;
      case LepusCmpKind::Lte:
        kind = ValueKind::BinaryLessThanOrEqualInstKind;
        break;
      case LepusCmpKind::Gt:
        kind = ValueKind::BinaryGreaterThanInstKind;
        break;
      case LepusCmpKind::Gte:
        kind = ValueKind::BinaryGreaterThanOrEqualInstKind;
        break;
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: BuildCompare encountered unsupported compare op");
    }

    BindRegister(dst_reg, builder()->Create<BinaryOperatorInst>(
                              iterator().GetCurrentLineCol(), left_val,
                              right_val, kind, dst_type));
  }

  void BuildBinary(LepusBinaryOpKind binary_op_kind) {
    auto dst_reg = iterator().GetOperand0();
    auto* left_val = GetRegister(iterator().GetOperand1());
    auto* right_val = GetRegister(iterator().GetOperand2());

    ValueKind kind;
    switch (binary_op_kind) {
      case LepusBinaryOpKind::Add:
        kind = ValueKind::BinaryAddInstKind;
        break;
      case LepusBinaryOpKind::Sub:
        kind = ValueKind::BinarySubInstKind;
        break;
      case LepusBinaryOpKind::Mul:
        kind = ValueKind::BinaryMulInstKind;
        break;
      case LepusBinaryOpKind::Div:
        kind = ValueKind::BinaryDivInstKind;
        break;
      case LepusBinaryOpKind::Pow:
        kind = ValueKind::BinaryPowInstKind;
        break;
      case LepusBinaryOpKind::Mod:
        kind = ValueKind::BinaryModInstKind;
        break;
      case LepusBinaryOpKind::BitAnd:
        kind = ValueKind::BinaryBitAndInstKind;
        break;
      case LepusBinaryOpKind::BitOr:
        kind = ValueKind::BinaryBitOrInstKind;
        break;
      case LepusBinaryOpKind::BitXor:
        kind = ValueKind::BinaryBitXorInstKind;
        break;
      case LepusBinaryOpKind::And:
        kind = ValueKind::BinaryAndInstKind;
        break;
      case LepusBinaryOpKind::Or:
        kind = ValueKind::BinaryOrInstKind;
        break;
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: BuildBinary encountered unsupported binary op");
    }

    BindRegister(dst_reg,
                 builder()->Create<BinaryOperatorInst>(
                     iterator().GetCurrentLineCol(), left_val, right_val, kind,
                     TypeOp::CreateAnyType(builder())));
  }

  void BuildUnary(LepusUnaryOpKind unary_op_kind) {
    auto dst_reg = iterator().GetOperand0();
    auto* src_val = GetRegister(iterator().GetOperand0());

    ValueKind kind;
    switch (unary_op_kind) {
      case LepusUnaryOpKind::Neg:
        kind = ValueKind::UnaryNegInstKind;
        break;
      case LepusUnaryOpKind::Pos:
        kind = ValueKind::UnaryPosInstKind;
        break;
      case LepusUnaryOpKind::Not:
        kind = ValueKind::UnaryNotInstKind;
        break;
      case LepusUnaryOpKind::Inc:
        kind = ValueKind::UnaryIncInstKind;
        break;
      case LepusUnaryOpKind::Dec:
        kind = ValueKind::UnaryDecInstKind;
        break;
      case LepusUnaryOpKind::Typeof:
        kind = ValueKind::UnaryTypeofInstKind;
        break;
      case LepusUnaryOpKind::BitNot:
        kind = ValueKind::UnaryBitNotInstKind;
        break;
      default:
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: BuildUnary encountered unsupported unary op");
    }

    auto* unary_inst = builder()->Create<UnaryOperatorInst>(
        iterator().GetCurrentLineCol(), src_val, kind);
    if (unary_op_kind == LepusUnaryOpKind::Not) {
      unary_inst->SetType(TypeOp::CreateBoolean(builder()));
    }
    BindRegister(dst_reg, unary_inst);
  }

  // unary
  void BuildTypeOp_Neg() { BuildUnary(LepusUnaryOpKind::Neg); }
  void BuildTypeOp_Not() { BuildUnary(LepusUnaryOpKind::Not); }
  void BuildTypeOp_Pos() { BuildUnary(LepusUnaryOpKind::Pos); }
  void BuildTypeOp_Inc() { BuildUnary(LepusUnaryOpKind::Inc); }
  void BuildTypeOp_Dec() { BuildUnary(LepusUnaryOpKind::Dec); }
  void BuildTypeOp_BitNot() { BuildUnary(LepusUnaryOpKind::BitNot); }
  void BuildTypeOp_Typeof() { BuildUnary(LepusUnaryOpKind::Typeof); }

  // binary
  void BuildTypeOp_Add() { BuildBinary(LepusBinaryOpKind::Add); }
  void BuildTypeOp_Sub() { BuildBinary(LepusBinaryOpKind::Sub); }
  void BuildTypeOp_Mul() { BuildBinary(LepusBinaryOpKind::Mul); }
  void BuildTypeOp_Div() { BuildBinary(LepusBinaryOpKind::Div); }
  void BuildTypeOp_Mod() { BuildBinary(LepusBinaryOpKind::Mod); }
  void BuildTypeOp_Pow() { BuildBinary(LepusBinaryOpKind::Pow); }
  void BuildTypeOp_BitOr() { BuildBinary(LepusBinaryOpKind::BitOr); }
  void BuildTypeOp_BitAnd() { BuildBinary(LepusBinaryOpKind::BitAnd); }
  void BuildTypeOp_BitXor() { BuildBinary(LepusBinaryOpKind::BitXor); }
  void BuildTypeOp_And() { BuildBinary(LepusBinaryOpKind::And); }
  void BuildTypeOp_Or() { BuildBinary(LepusBinaryOpKind::Or); }

  // cmp
  void BuildTypeOp_Less() { BuildCompare(LepusCmpKind::Lt); }
  void BuildTypeOp_Greater() { BuildCompare(LepusCmpKind::Gt); }
  void BuildTypeOp_Equal() { BuildCompare(LepusCmpKind::Eq); }
  void BuildTypeOp_UnEqual() { BuildCompare(LepusCmpKind::Neq); }
  void BuildTypeOp_LessEqual() { BuildCompare(LepusCmpKind::Lte); }
  void BuildTypeOp_GreaterEqual() { BuildCompare(LepusCmpKind::Gte); }
  void BuildTypeOp_AbsUnEqual() { BuildCompare(LepusCmpKind::Neq); }
  void BuildTypeOp_AbsEqual() { BuildCompare(LepusCmpKind::Eq); }

  // useless
  void BuildTypeOp_Noop() {}
  void BuildTypeOp_Len() {}
  void BuildTypeLabel_Catch() {
    // Preserve the catch label in IR; the VM relies on it for exception scan.
    builder()->Create<CatchInst>(iterator().GetCurrentLineCol());
  }

  void BuildTypeOp_NewTable() {
    auto dst_reg = iterator().GetOperand0();
    auto* new_table_inst =
        builder()->Create<NewTableInst>(iterator().GetCurrentLineCol());
    BindRegister(dst_reg, new_table_inst);
  }

  void BuildTypeOp_SetTable() {
    auto* obj = GetRegister(iterator().GetOperand0());
    auto* key_val = GetRegister(iterator().GetOperand1());
    auto* value_val = GetRegister(iterator().GetOperand2());
    builder()->Create<SetTableInst>(iterator().GetCurrentLineCol(), obj,
                                    key_val, value_val);
  }

  void BuildTypeOp_GetTable() {
    auto dst_reg = iterator().GetOperand0();
    auto* obj = GetRegister(iterator().GetOperand1());
    auto* key_val = GetRegister(iterator().GetOperand2());
    auto* get_table_inst = builder()->Create<GetTableInst>(
        iterator().GetCurrentLineCol(), obj, key_val);
    BindRegister(dst_reg, get_table_inst);
  }
  void BuildTypeOp_Switch() {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: bytecode Switch is not supported in MIR builder");
  }
  void BuildTypeOp_NewArray() {
    auto dst_reg = iterator().GetOperand0();
    auto array_size = iterator().GetOperand1();

    ArgList items;
    items.reserve(array_size);
    for (auto i = 0; i < array_size; i++) {
      items.push_back(GetRegister(iterator().GetOperand0() + i + 1));
    }

    BindRegister(dst_reg, builder()->Create<NewArrayInst>(
                              iterator().GetCurrentLineCol(), items));
  }

  void BuildTypeOp_GetBuiltin() {
    auto dst_reg = iterator().GetOperand0();

    auto builtin_index = iterator().GetOperand1x();
    TypeOp* builtin_type = TypeOp::GetValueTypeOp(
        builder(), ir_ctx_->GetVMContext()->builtin()->Get(builtin_index));

    auto* get_global_inst = builder()->Create<GetBuiltinInst>(
        iterator().GetCurrentLineCol(),
        builder()->GetLiteralUint32(builtin_index), builtin_type);
    BindRegister(dst_reg, get_global_inst);
  }

  void BuildTypeOp_SetCatchId() {
    auto dst_reg = iterator().GetOperand0();
    auto set_catch_id_inst =
        builder()->Create<SetCatchIdInst>(iterator().GetCurrentLineCol());
    BindRegister(dst_reg, set_catch_id_inst);
  }

  void BuildTypeLabel_Throw() {
    auto src = GetRegister(iterator().GetOperand0());
    // Attach the nearest catch label (the first TypeLabel_Catch after the
    // current offset) as the exceptional successor. This makes catch blocks
    // reachable in the IR CFG and preserves bytecode layout constraints.
    Block* catch_bb = nullptr;
    const int cur_offset = iterator().GetCurrentOffset();
    const int catch_offset = FindNextCatchLabelOffset(cur_offset);
    if (catch_offset >= 0) {
      catch_bb = GetBasicBlock(catch_offset);
      // Model the exceptional control-flow edge for SSA/env merging.
      MergeEnvironment(catch_offset);
    }

    builder()->Create<ThrowInst>(iterator().GetCurrentLineCol(), src, catch_bb);

    // Replace the pre-linked fallthrough terminator.
    if (auto* original_terminator = builder()->GetBlock()->GetTerminator()) {
      original_terminator->EraseFromParent();
    }

    // Note: do not clear current_env_ here. We still need an environment while
    // scanning later bytecode to build subsequent (possibly unreachable)
    // blocks.
  }

  void BuildTypeOp_CreateContext() {
    auto dst_reg = iterator().GetOperand0();
    auto array_size = iterator().GetOperand1() + 1;

    auto create_function_context_inst =
        builder()->Create<CreateFunctionContextInst>(
            iterator().GetCurrentLineCol(),
            builder()->GetLiteralUint8(array_size));

    BindRegister(dst_reg, create_function_context_inst);
  }

  void BuildTypeOp_SetContextSlotMove() {
    auto context_reg = GetRegister(iterator().GetOperand0());
    auto index = iterator().GetOperand1();
    auto val = GetRegister(iterator().GetOperand2());

    builder()->Create<SetContextSlotMovInst>(
        iterator().GetCurrentLineCol(), context_reg,
        builder()->GetLiteralUint8(index), val);
  }

  void BuildTypeOp_GetContextSlotMove() {
    auto dst_reg = iterator().GetOperand0();
    auto index = iterator().GetOperand1();
    auto context = GetRegister(iterator().GetOperand2());
    auto* get_context_slot_inst = builder()->Create<GetContextSlotMovInst>(
        iterator().GetCurrentLineCol(), context,
        builder()->GetLiteralUint8(index));

    BindRegister(dst_reg, get_context_slot_inst);
  }

  void BuildTypeOp_PushContext() {
    auto src_val = GetRegister(iterator().GetOperand0());
    builder()->Create<PushContextInst>(iterator().GetCurrentLineCol(), src_val);
  }

  void BuildTypeOp_PopContext() {
    builder()->Create<PopContextInst>(iterator().GetCurrentLineCol());
  }

  void BuildTypeOp_GetContextSlot() {
    auto dst_reg = iterator().GetOperand0();
    auto index = iterator().GetOperand1();
    auto depth = iterator().GetOperand2();

    auto* get_context_slot_inst = builder()->Create<GetContextSlotInst>(
        iterator().GetCurrentLineCol(), builder()->GetLiteralUint8(depth),
        builder()->GetLiteralUint8(index));

    BindRegister(dst_reg, get_context_slot_inst);
  }

  void BuildTypeOp_SetContextSlot() {
    auto src_val = GetRegister(iterator().GetOperand0());
    auto index = iterator().GetOperand1();
    auto depth = iterator().GetOperand2();

    builder()->Create<SetContextSlotInst>(
        iterator().GetCurrentLineCol(), builder()->GetLiteralUint8(depth),
        builder()->GetLiteralUint8(index), src_val);
  }

  void BuildTypeOp_CreateBlockContext() {
    auto dst_reg = iterator().GetOperand0();
    auto array_size = iterator().GetOperand1() + 1;

    auto create_block_context_inst = builder()->Create<CreateBlockContextInst>(
        iterator().GetCurrentLineCol(),
        builder()->GetLiteralUint32(array_size));

    BindRegister(dst_reg, create_block_context_inst);
  }

  void BuildTypeLabel_EnterBlock() {
    builder()->Create<PushBlockContextInst>(iterator().GetCurrentLineCol());
  }

  void BuildTypeLabel_LeaveBlock() {
    builder()->Create<PopBlockContextInst>(iterator().GetCurrentLineCol());
  }
};

SSABuilder::SSABuilder(IRContext* ir_ctx, FuncOp* func)
    : ir_ctx_(ir_ctx), func_(func) {
  auto lepus_function = func->GetLepusFunction();
  param_count_ = lepus_function->GetParamsSize();
  auto max_reg = lepus_function->GetRegisterCount();
  if (LEPUS_UNLIKELY(max_reg >= 256)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: SSABuilder register count must be < 256");
  }
  register_count_ = max_reg + 1;
}

void SSABuilder::Build() {
  auto lepus_function = func_->GetLepusFunction();
  auto builder = ir_ctx_->GetOpBuilder();
  // create entry block
  builder->CreateBlock(func_->GetSingleRegion(), BlockType::BT_INST,
                       lepus_function->CurrentLineCol());

  BytecodeBuilderImpl impl(ir_ctx_, func_, register_count_, param_count_);
  auto opcodes = lepus_function->GetOpCodes();
  const auto& line_cols = lepus_function->GetLineCol();
  impl.Build(const_cast<lynx::lepus::Instruction*>(&opcodes[0]),
             lepus_function->OpCodeSize(), line_cols);
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
