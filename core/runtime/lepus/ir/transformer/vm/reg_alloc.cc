// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/transformer/vm/reg_alloc.h"

#include <algorithm>
#include <iostream>
#include <limits>
#include <queue>
#include <utility>

#include "core/runtime/lepus/ir/analysis/analysis.h"
#include "core/runtime/lepus/ir/analysis/cfg.h"
#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/instruction.h"
#include "core/runtime/lepus/ir/ir_context.h"
#ifdef LEPUS_TEST
#include "core/runtime/lepus/ir/ir_dumper.h"
#endif
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/DenseSet.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/op_builder.h"

namespace lynx {
namespace lepus {
namespace ir {

#ifdef LEPUS_TEST
struct LivenessRegAllocIRPrinter : IRDumper {
  RegisterAllocator& allocator;

  explicit LivenessRegAllocIRPrinter(IRContext* ir_ctx, RegisterAllocator& ra,
                                     std::ostream& ost, bool escape = false)
      : IRDumper(ir_ctx, ost, escape), allocator(ra) {}

  bool PrintInstructionDestination(Instruction* i) override {
    // auto codeGenOpts = I->getContext().getCodeGenerationSettings();

    if (!allocator.IsAllocated(i))
      os_ << "$???";
    else
      os_ << "$" << allocator.GetRegister(i);

    return true;
  }

  void PrintValueLabel(Instruction* i, Value* v, unsigned op_index) override {
    if (allocator.IsAllocated(v))
      os_ << "$" << allocator.GetRegister(v);
    else
      IRPrinter::PrintValueLabel(i, v, op_index);
  }
};
#endif

RegisterAllocator::RegisterAllocator(FuncOp* func) : f_(func) {
  root_function_ = f_->GetIRCtx()->GetMainMod()->GetRootFunction();
}

// get the max register for the call_inst function
unsigned RegisterAllocator::GetTargetRegForCallFunction(Instruction* call_inst,
                                                        Value* exclude) {
  if (!llvh::isa<CallInst>(call_inst)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterAllocator::GetTargetRegForCallFunction "
        "expects CallInst");
  }
  unsigned max_live_reg_idx = 0;
  unsigned call_inst_idx = GetInstructionNumber(call_inst);
  bool find = false;

  // Liveness in this allocator is defined in the (idx + 1) space. A value used
  // by `call_inst` must be live at point (call_inst_idx + 1).
  Segment call_point(call_inst_idx + 1, call_inst_idx + 2);

  // Performance: iterate `instructions_by_numbers_` instead of scanning the
  // `allocated` DenseMap for each CallInst.
  const unsigned max_inst_idx = GetMaxInstrIndex();
  for (unsigned inst_idx = 0; inst_idx < max_inst_idx; ++inst_idx) {
    Instruction* i = instructions_by_numbers_[inst_idx];
    if (!i || i == call_inst) continue;
    if (exclude && i == exclude) continue;
    if (!IsAllocated(i)) continue;

    Register r = GetRegister(i);
    if (!r.IsValid()) continue;

    Interval& inst_interval = instruction_interval_[inst_idx];
    if (inst_interval.Intersects(call_point)) {
      max_live_reg_idx = std::max(max_live_reg_idx, r.GetIndex());
      find = true;
    }
  }

  // Conservatively include any call operands (function + args). These are the
  // values that must remain intact while the VM materializes arguments.
  for (int i = 0, e = call_inst->GetNumOperands(); i < e; ++i) {
    Value* op = call_inst->GetOperand(i);
    if (exclude && op == exclude) continue;
    if (!IsAllocated(op)) continue;
    Register r = GetRegister(op);
    if (!r.IsValid()) continue;
    max_live_reg_idx = std::max(max_live_reg_idx, r.GetIndex());
    find = true;
  }

  // IMPORTANT: for functions with a preallocated prefix (parameters / toplevel
  // variables), the VM call fast-paths (Call1/CallRandom*) materialize
  // arguments into `a+1..a+argc`. If `a` stays below the prefix boundary, the
  // argument materialization can clobber values in the prefix (e.g. parameters
  // in non-toplevel functions, toplevel variables in the root function).
  //
  // Therefore, for any call with arguments, conservatively treat the whole
  // prefix as live at the call-site and ensure the target register is at least
  // `prefix_reg_`.
  auto* call = llvh::cast<CallInst>(call_inst);
  const unsigned argc = call->GetNumArguments();
  const unsigned func_reg = GetRegister(call->GetFunction()).GetIndex();
  if (argc > 0 && prefix_reg_ > 1 && func_reg < prefix_reg_ - 1) {
    // If callee stays inside the prefix, the argument materialization
    // (a+1..a+argc) can overwrite values in the prefix (e.g. parameters).
    //
    // Note: when callee is already at (prefix_reg_ - 1), (a+1) starts at
    // prefix_reg_, so it won't clobber prefix values.
    max_live_reg_idx = std::max(max_live_reg_idx, prefix_reg_ - 1);
    find = true;
  }
  return find ? max_live_reg_idx + 1 : max_live_reg_idx;
}

bool RegisterFile::IsUsed(Register r) { return !registers_.test(r.GetIndex()); }

bool RegisterFile::IsFree(Register r) { return !IsUsed(r); }

void RegisterFile::KillRegister(Register reg) {
  if (!IsUsed(reg)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterFile::KillRegister called on an unused "
        "register");
  }
  registers_.set(reg.GetIndex());
  if (!IsFree(reg)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterFile::KillRegister failed to free register");
  }
}

void RegisterFile::Rebuild(unsigned new_size,
                           llvh::ArrayRef<unsigned> used_regs) {
  registers_.clear();
  registers_.resize(new_size, true);
  for (unsigned idx : used_regs) {
    if (idx >= new_size) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: RegisterFile::Rebuild used register index out of "
          "range");
    }
    registers_.reset(idx);
  }
}

Register RegisterFile::AllocateRegister(uint32_t idx, bool check) {
  Register R(idx);
  if (registers_.size() < idx + 1) registers_.resize(idx + 1, true);

  if (LEPUS_UNLIKELY(!IsFree(R) && check)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterFile::AllocateRegister tried to allocate an "
        "already-assigned register");
  }
  registers_.reset(idx);
  return R;
}

Register RegisterFile::AllocateRegisterCluster(uint32_t idx, unsigned count) {
  if (registers_.size() < idx + count) registers_.resize(idx + count, true);
  unsigned i = 0;
  while (i < count) {
    if (LEPUS_UNLIKELY(!IsFree(Register(idx + (i++))))) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: RegisterFile::AllocateRegisterCluster tried to "
          "allocate an already-assigned register");
    }
  }
  registers_.reset(idx, idx + count);
  return Register(idx);
}

Register RegisterFile::AllocateRegister() {
  // We first check for the 'all' case because we usually have a small number of
  // active bits (<64), so this operation is actually faster than the linear
  // scan.
  if (registers_.none()) {
    // If all bits are set, create a new register and return it.
    unsigned num_regs = registers_.size();
    registers_.resize(num_regs + 1, false);
    Register R = Register(num_regs);
    if (!IsUsed(R)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: RegisterFile::AllocateRegister failed to mark new "
          "register as used");
    }
    return R;
  }

  // Search for a free register to use.
  int i = registers_.find_first();
  if (i < 0) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterFile::AllocateRegister failed to find free "
        "register");
  }
  Register R(i);
  if (!IsFree(R)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterFile::AllocateRegister selected non-free "
        "register");
  }
  registers_.reset(i);
  return R;
}

Register RegisterFile::TailAllocateConsecutive(unsigned n) {
  if (n == 0) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterFile::TailAllocateConsecutive can't request "
        "zero registers");
  }

  int last_used = registers_.size() - 1;
  while (last_used >= 0) {
    if (!registers_.test(last_used)) break;

    last_used--;
  }

  int first_clear = last_used + 1;

  registers_.resize(std::max(registers_.size(), first_clear + n), true);
  registers_.reset(first_clear, first_clear + n);

  return Register(first_clear);
}

/// \returns true if the PHI node has an external user (that requires a
/// register read) and a local writer.
static bool PhiReadWrite(PhiInst* p) {
  bool local_phi_use = false;
  bool external_use = false;
  bool terminator_use = false;

  Block* parent = p->GetParent();

  for (auto* u : p->GetUsers()) {
    terminator_use |= llvh::isa<TerminatorInst>(u);
    local_phi_use |=
        (llvh::isa<PhiInst>(u) && u->GetParent() == parent && p != u);
    external_use |= u->GetParent() != parent;
  }

  return terminator_use || local_phi_use || external_use;
}

void RegisterAllocator::LowerPhis(ArrayRef<Block*> order) {
  llvh::SmallVector<PhiInst*, 8> phis;
  OpBuilder builder;
  builder.SetModuleOp(f_->GetIRCtx()->GetMainMod());

  // Collect all phis.
  for (auto& bb : order) {
    for (auto& inst : *bb) {
      if (auto* p = llvh::dyn_cast<PhiInst>(&inst)) {
        phis.push_back(p);
      }
    }
  }

  // The MOV sequence at the end of the block writes the values that need to go
  // into the PHI node registers in the jump-destination basic blocks. In the
  // case of cycles we may need to read a value from a current PHI node and also
  // prepare the value of the same PHI node for the next iteration. To make sure
  // that we read an up-to-date value we copy the value using a MOV before we
  // emit the MOV sequence and replace all external uses.
  for (PhiInst* p : phis) {
    if (!PhiReadWrite(p)) continue;

    // The MOV sequence may clobber the PHI. Insert a copy.
    builder.SetInsertionPoint(p->GetParent()->GetTerminator());
    int64_t mov_loc = p->GetLocation();
    if (!mov_loc) {
      mov_loc = p->GetParent()->GetTerminator()->GetLocation();
    }
    auto* mov = builder.Create<MovInst>(mov_loc, p);

    Value::UseListTy users = p->GetUsers();
    // Update all external users:
    for (auto* u : users) {
      // Local uses of the PHI are allowed.
      if (!llvh::isa<PhiInst>(u) && !llvh::isa<TerminatorInst>(u) &&
          u->GetParent() == p->GetParent())
        continue;

      u->ReplaceFirstOperandWith(p, mov);
    }
  }

  /// A list of registers that were copied to prevent clobbering. Maps the
  /// original PHI node to the copied value.
  DenseMap<Value*, MovInst*> copied;

  // Lower all PHI nodes into a sequence of MOVs in the predecessor blocks.
  for (PhiInst* p : phis) {
    for (unsigned i = 0, e = p->GetNumEntries(); i < e; ++i) {
      auto E = p->GetEntry(i);
      auto* term = E.second->GetTerminator();
      builder.SetInsertionPoint(term);
      int64_t mov_loc = term->GetLocation();
      if (!mov_loc) {
        mov_loc = p->GetLocation();
      }
      if (!mov_loc) {
        if (auto* in_inst = llvh::dyn_cast<Instruction>(E.first)) {
          mov_loc = in_inst->GetLocation();
        }
      }
      auto* mov = builder.Create<MovInst>(mov_loc, E.first);

      f_->GetIRCtx()->UpdateSpecialAttribute(p, mov);

      p->UpdateEntry(i, mov, E.second);

      // If the terminator uses the value that we are inserting then we can fix
      // the lifetime by making it use the MOV. We can do this because we know
      // that terminators don't modify values in destination PHI nodes and this
      // allows us to merge the lifetime of the value and save a register.
      copied[E.first] = mov;
    }
  }

  // The terminator comes after the MOV sequence, so make sure it uses the
  // updated registers.
  for (auto& bb : order) {
    auto* term = bb->GetTerminator();

    for (int i = 0, e = term->GetNumOperands(); i < e; i++) {
      auto* op = term->GetOperand(i);
      if (llvh::isa<Literal>(op)) continue;
      auto it = copied.find(op);
      if (it != copied.end()) {
        if (it->second->GetParent() == bb) {
          term->SetOperand(it->second, i);
          it->second->MoveBefore(term);
        }
      }
    }
  }
}

void RegisterAllocator::CalculateLocalLiveness(BlockLifetimeInfo& liveness_info,
                                               Block* bb) {
  // For each instruction in the block:
  for (auto it = bb->InstBegin(); it != bb->InstEnd(); ++it) {
    Instruction* i = *it;

    unsigned idx = GetInstructionNumber(i);
    liveness_info.kill_.set(idx);

    // PHI nodes require special handling because they are flow sensitive. Mask
    // out flow that does not go in the direction of the phi edge.
    if (auto* p = llvh::dyn_cast<PhiInst>(i)) {
      llvh::SmallVector<unsigned, 4> incoming_value_num;

      // Collect all incoming value numbers.
      for (int i = 0, e = p->GetNumEntries(); i < e; i++) {
        auto entry = p->GetEntry(i);
        // Skip unreachable predecessors.
        if (!block_liveness_.count(entry.second)) continue;
        if (auto* ii = llvh::dyn_cast<Instruction>(entry.first)) {
          incoming_value_num.push_back(GetInstructionNumber(ii));
        }
      }

      // Block the incoming values from flowing into the predecessors.
      for (int i = 0, e = p->GetNumEntries(); i < e; i++) {
        auto entry = p->GetEntry(i);
        // Skip unreachable predecessors.
        if (!block_liveness_.count(entry.second)) continue;
        for (auto num : incoming_value_num) {
          block_liveness_[entry.second].mask_in_.set(num);
        }
      }

      // Allow the flow of incoming values in specific directions:
      for (int i = 0, e = p->GetNumEntries(); i < e; i++) {
        auto entry = p->GetEntry(i);
        // Skip unreachable predecessors.
        if (!block_liveness_.count(entry.second)) continue;
        if (auto* ii = llvh::dyn_cast<Instruction>(entry.first)) {
          unsigned idx_ii = GetInstructionNumber(ii);
          block_liveness_[entry.second].mask_in_.reset(idx_ii);
        }
      }
    }

    // For each one of the operands that are also instructions:
    for (unsigned op_idx = 0, e = i->GetNumOperands(); op_idx != e; ++op_idx) {
      auto* op_inst = llvh::dyn_cast<Instruction>(i->GetOperand(op_idx));
      if (!op_inst) continue;
      // Skip instructions from unreachable blocks.
      if (!block_liveness_.count(op_inst->GetParent())) continue;

      // Get the index of the operand.
      auto op_inst_idx = GetInstructionNumber(op_inst);
      liveness_info.gen_.set(op_inst_idx);
    }
  }
}

void RegisterAllocator::CalculateGlobalLiveness(ArrayRef<Block*> order) {
  llvh::SmallVector<Block*, 64> work_list;
  llvh::DenseSet<Block*> in_work_list;

  // Init the live-in vector to be GEN-KILL.
  for (auto it = order.rbegin(), e = order.rend(); it != e; it++) {
    Block* bb = *it;
    BlockLifetimeInfo& liveness_info = block_liveness_[bb];
    liveness_info.live_in_ |= liveness_info.gen_;
    liveness_info.live_in_.reset(liveness_info.kill_);
    liveness_info.live_in_.reset(liveness_info.mask_in_);

    work_list.push_back(bb);
    in_work_list.insert(bb);
  }

  while (!work_list.empty()) {
    Block* bb = work_list.pop_back_val();
    in_work_list.erase(bb);

    BlockLifetimeInfo& liveness_info = block_liveness_[bb];

    // Rule:  OUT = SUCC0_in  + SUCC1_in ...
    bool out_changed = false;
    for (auto* succ : Successors(bb)) {
      BlockLifetimeInfo& succ_info = block_liveness_[succ];
      if (succ_info.live_in_.test(liveness_info.live_out_)) {
        liveness_info.live_out_ |= succ_info.live_in_;
        out_changed = true;
      }
    }

    if (out_changed) {
      // Rule: In = gen + (OUT - KILL - MASK)
      BitVector& new_in = liveness_info.scratch_;
      new_in = liveness_info.live_out_;
      new_in |= liveness_info.gen_;
      new_in.reset(liveness_info.kill_);
      new_in.reset(liveness_info.mask_in_);

      if (new_in != liveness_info.live_in_) {
        // Swap to reuse allocations in scratch_.
        liveness_info.live_in_.swap(new_in);
        for (auto* pred : Predecessors(bb)) {
          if (in_work_list.insert(pred).second) {
            work_list.push_back(pred);
          }
        }
      }
    }
  }
}

Interval& RegisterAllocator::GetInstructionInterval(Instruction* i) {
  auto idx = GetInstructionNumber(i);
  return instruction_interval_[idx];
}

bool RegisterAllocator::IsManuallyAllocatedInterval(Instruction* i) {
  if (HasTargetSpecificLowering(i)) return true;

  for (auto* u : i->GetUsers()) {
    if (HasTargetSpecificLowering(u)) return true;
  }

  return false;
}

void RegisterAllocator::Coalesce(DenseMap<Instruction*, Instruction*>& map,
                                 ArrayRef<Block*> order) {
  auto get_root = [&](Instruction* i) {
    Instruction* curr = i;
    while (map.count(curr)) {
      curr = map[curr];
    }
    // Path compression
    Instruction* temp = i;
    while (map.count(temp)) {
      Instruction* next = map[temp];
      if (next == curr) break;
      map[temp] = curr;
      temp = next;
    }
    return curr;
  };

  // Phase 1: Merge all PHI nodes into a single interval. This part is required
  // for correctness because it bounds the MOV and the phis into a single
  // interval.
  for (Block* bb : order) {
    auto it = bb->InstBegin();
    for (; it != bb->InstEnd(); ++it) {
      Instruction& i = **it;
      auto* p = llvh::dyn_cast<PhiInst>(&i);
      if (!p) continue;

      unsigned phi_num = GetInstructionNumber(p);
      for (unsigned i = 0, e = p->GetNumEntries(); i < e; ++i) {
        auto* mov = llvh::cast<MovInst>(p->GetEntry(i).first);

        // Bail out if the interval is already mapped, like in the case of self
        // edges.
        if (map.count(mov)) continue;

        if (!HasInstructionNumber(mov)) continue;

        unsigned idx = GetInstructionNumber(mov);
        instruction_interval_[phi_num].Add(instruction_interval_[idx]);

        // Record the fact that the mov should use the same register as the phi.
        Instruction* phi_dest = get_root(p);
        map[mov] = phi_dest;
      }
    }
  }

  // Phase 2: copy reuse optimization
  for (Block* bb : order) {
    DenseMap<Value*, MovInst*> lastCopy;

    auto it = bb->InstBegin();
    for (; it != bb->InstEnd(); ++it) {
      Instruction& i = **it;
      auto* mov = llvh::dyn_cast<MovInst>(&i);
      if (!mov) continue;

      Value* op = mov->GetSingleOperand();
      if (llvh::isa<Literal>(op)) continue;

      // If we've made a copy inside this basic block then use the copy.
      auto it = lastCopy.find(op);
      if (it != lastCopy.end()) {
        mov->SetOperand(it->second, 0);

        // Update the interval of the reused MOV instruction
        unsigned prev_idx = GetInstructionNumber(it->second);
        unsigned curr_idx = GetInstructionNumber(mov);
        instruction_interval_[prev_idx].Add(
            Segment(prev_idx + 1, curr_idx + 1));
      }

      lastCopy[op] = mov;
    }
  }

  // Phase 3: Optimize the program by coalescing multiple live intervals into a
  // single long interval. This phase is optional.
  for (Block* bb : order) {
    for (auto it = bb->InstBegin(); it != bb->InstEnd(); ++it) {
      Instruction& i = **it;
      auto* mov = llvh::dyn_cast<MovInst>(&i);
      if (!mov) continue;

      auto* op = llvh::dyn_cast<Instruction>(mov->GetSingleOperand());
      if (!op) continue;

      // Don't Coalesce intervals that are already coalesced to other intervals
      // or that there are other intervals that are coalesced into it, or if
      // the interval is pre-allocated.
      if (map.count(op) || IsAllocated(op) || IsAllocated(mov)) continue;

      Instruction* dest_root = get_root(mov);
      Instruction* op_root = get_root(op);

      // Be more aggressive for move coalescing:
      // - Keep skipping when the operand itself has target specific lowering.
      // - But allow coalescing when only *users* of the operand have
      //   target-specific lowering (e.g. call arguments). This helps reduce
      //   MOV chains without changing call lowering semantics.
      if (HasTargetSpecificLowering(op_root)) continue;

      if (dest_root == op_root) continue;

      unsigned dest_idx = GetInstructionNumber(dest_root);
      unsigned op_idx = GetInstructionNumber(op_root);
      Interval& dest_ivl = instruction_interval_[dest_idx];
      Interval& op_ivl = instruction_interval_[op_idx];

      if (dest_ivl.Intersects(op_ivl)) continue;

      instruction_interval_[dest_idx].Add(op_ivl);
      map[op_root] = dest_root;
    }
  }
}

namespace {
/// Determines whether the Instruction is ever used outside its Block.
bool isBlockLocal(Instruction* inst) {
  Block* parent = inst->GetParent();
  for (auto user : inst->GetUsers()) {
    if (parent != user->GetParent()) {
      return false;
    }
  }
  return true;
}
}  // namespace

void RegisterAllocator::AllocateFastPass(ArrayRef<Block*> order) {
  // Make sure Phis and related Movs get the same register
  for (auto* bb : order) {
    for (auto it = bb->InstBegin(); it != bb->InstEnd(); ++it) {
      Instruction& inst = **it;
      HandleInstruction(&inst);
      if (auto* phi = llvh::dyn_cast<PhiInst>(&inst)) {
        auto reg = file_.AllocateRegister();
        UpdateRegister(phi, reg);
        for (int i = 0, e = phi->GetNumEntries(); i < e; i++) {
          UpdateRegister(phi->GetEntry(i).first, reg);
        }
      }
    }
  }

  llvh::SmallVector<Register, 16> block_locals;

  // Then just allocate the rest sequentially, while optimizing the case
  // where an inst is only ever used in its own block.
  for (auto* bb : order) {
    for (auto it = bb->InstBegin(); it != bb->InstEnd(); ++it) {
      Instruction& inst = **it;
      if (!IsAllocated(&inst)) {
        Register R = file_.AllocateRegister();
        UpdateRegister(&inst, R);
        if (inst.GetNumUsers() == 0) {
          file_.KillRegister(R);
        } else if (isBlockLocal(&inst)) {
          block_locals.push_back(R);
        }
      }
    }
    for (auto& reg : block_locals) {
      file_.KillRegister(reg);
    }
    block_locals.clear();
  }
}

void RegisterAllocator::ProcessCallInst(OpBuilder* builder,
                                        CallInst* call_inst) {
  Value* func = call_inst->GetFunction();
  if (!func) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterAllocator::ProcessCallInst found nullptr "
        "callee");
  }

  // Compute the max live register *excluding* the current callee value.
  // This makes the placement idempotent when this helper is re-run after
  // post-RA passes.
  unsigned target_reg = GetTargetRegForCallFunction(call_inst, func);
  const unsigned func_reg = GetRegister(func).GetIndex();
  // Only move the callee *upwards* when it is not already above all other live
  // values. This avoids unnecessary extra MOVs and is more resilient if some
  // post-RA transformations made the interval data slightly stale.
  if (func_reg < target_reg) {
    builder->SetInsertionPoint(call_inst);
    // Check if the current function value is already a MovInst that can be
    // reused to satisfy the call-clobber convention.
    // Conditions for reuse:
    // 1. It is a MovInst.
    // 2. It has exactly one user (the current CallInst).
    // 3. It is immediately before the CallInst. This ensures no other
    //    instructions' live ranges interfere with the register assignment
    //    between the Mov and the Call.
    if (auto* mov_inst = llvh::dyn_cast<MovInst>(func)) {
      if (mov_inst->HasOneUser()) {
        Operation* mov_op = mov_inst;
        auto iter = mov_op->getIterator();
        auto next_iter = ++iter;

        if (next_iter != mov_inst->GetParent()->end() &&
            &*next_iter == call_inst) {
          // Safe to reuse the existing MovInst by just updating its destination
          // register to the new target_reg.
          UpdateRegister(mov_inst, Register(target_reg));

          // Ensure the MovInst interval covers the call-site properly.
          InitMovInstInterval(mov_inst, call_inst);

          // Mark it as a call-func MOV so subsequent passes know it's special.
          mov_inst->SetCallFuncMov(true);
          return;
        }
      }
    }

    // add mov
    int64_t mov_loc = call_inst->GetLocation();
    if (!mov_loc) {
      if (auto* func_inst = llvh::dyn_cast<Instruction>(func)) {
        mov_loc = func_inst->GetLocation();
      }
    }
    auto* func_mov = builder->Create<MovInst>(mov_loc, func);
    InitMovInstInterval(func_mov, call_inst);
    call_inst->SetOperand(func_mov, CallInst::methodIdx);

    // IMPORTANT: the VM call fast-paths (e.g. TypeOp_Call1 / CallRandom*) may
    // write argument values into registers right after the function register
    // (e.g. `a + 1`, `a + 2`, ...). To avoid clobbering live values, we must
    // place the call's function operand at a fresh "top" register index.
    UpdateRegister(func_mov, Register(target_reg));
    func_mov->SetCallFuncMov(true);
  }
}

void RegisterAllocator::RemapRegByExecOrder(
    ArrayRef<Block*> order, DenseMap<Instruction*, Instruction*>& coalesced) {
  llvh::DenseMap<unsigned, llvh::SmallVector<Value*, 4>> reg_to_values;
  for (auto& item : allocated_) {
    unsigned reg = item.second.GetIndex();
    if (reg >= prefix_reg_) {
      reg_to_values[reg].push_back(item.first);
    }
  }

  if (reg_to_values.empty()) return;

  llvh::DenseMap<unsigned, unsigned> reg_to_min_exec_idx;
  for (auto& entry : reg_to_values) {
    unsigned reg = entry.first;
    auto& values = entry.second;
    unsigned min_idx = std::numeric_limits<unsigned>::max();
    for (Value* v : values) {
      if (auto* inst = llvh::dyn_cast<Instruction>(v)) {
        auto it = inst_to_execute_order_.find(inst);
        if (it != inst_to_execute_order_.end()) {
          min_idx = std::min(min_idx, it->second);
        }
      }
    }
    reg_to_min_exec_idx[reg] = min_idx;
  }

  llvh::SmallVector<unsigned, 32> sorted_old_regs;
  for (auto& entry : reg_to_values) {
    sorted_old_regs.push_back(entry.first);
  }
  std::sort(sorted_old_regs.begin(), sorted_old_regs.end(),
            [&](unsigned a, unsigned b) {
              unsigned idx_a = reg_to_min_exec_idx[a];
              unsigned idx_b = reg_to_min_exec_idx[b];
              if (idx_a != idx_b) return idx_a < idx_b;
              return a < b;
            });

  unsigned next_new_reg = prefix_reg_;
  for (unsigned old_reg : sorted_old_regs) {
    unsigned new_reg = next_new_reg++;
    for (Value* v : reg_to_values[old_reg]) {
      UpdateRegister(v, Register(new_reg));
    }
    file_.AllocateRegister(new_reg, false);
  }
}

void RegisterAllocator::ShuffleRegistersToMinimize() {
  llvh::SmallSet<unsigned, 32> used_regs;

  for (const auto& item : allocated_) {
    if (item.second.GetIndex() >= prefix_reg_) {
      used_regs.insert(item.second.GetIndex());
    }
  }
  if (used_regs.empty()) return;
  llvh::SmallVector<unsigned, 32> sortedRegs(used_regs.begin(),
                                             used_regs.end());
  std::sort(sortedRegs.begin(), sortedRegs.end());
  llvh::DenseMap<unsigned, unsigned> reg_remap;
  auto new_reg = prefix_reg_;
  for (unsigned old_reg : sortedRegs) {
    reg_remap[old_reg] = new_reg++;
  }

  for (const auto& [inst, old_reg] : allocated_) {
    if (!reg_remap.count(old_reg.GetIndex())) continue;
    Register n_reg = Register(reg_remap[old_reg.GetIndex()]);
    allocated_[inst] = n_reg;
    file_.AllocateRegister(n_reg.GetIndex(), false);
  }
}

void RegisterAllocator::Allocate(ArrayRef<Block*> order) {
  // PerfSection regAlloc("Register Allocation");

  unsigned exec_idx = 0;
  inst_to_execute_order_.clear();
  inst_to_old_reg_.clear();
  for (auto* bb : order) {
    for (auto it = bb->InstBegin(); it != bb->InstEnd(); ++it) {
      Instruction* i = *it;
      inst_to_execute_order_[i] = exec_idx++;
    }
  }

  // Lower PHI nodes into a sequence of MOVs.
  LowerPhis(order);

  // Number instructions:
  for (auto* bb : order) {
    for (auto it = bb->InstBegin(); it != bb->InstEnd(); ++it) {
      Instruction* i = *it;
      [[maybe_unused]] auto idx = GetInstructionNumber(i);
      if (instruction_numbers_[i] != idx) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: RegisterAllocator::Allocate encountered invalid "
            "instruction numbering");
      }
    }
  }

  // Init the basic block liveness data structure and calculate the local
  // liveness for each basic block.
  unsigned max_idx = GetMaxInstrIndex();
  for (auto* bb : order) {
    block_liveness_[bb].Init(max_idx);
  }
  for (auto* bb : order) {
    CalculateLocalLiveness(block_liveness_[bb], bb);
  }

  // Propagate the local liveness information across the whole function.
  CalculateGlobalLiveness(order);

  // Calculate the live intervals for each instruction.
  CalculateLiveIntervals(order);

  // Free the memory used for liveness.
  block_liveness_.clear();

  // Maps coalesced instructions. First uses the register allocated for Second.
  DenseMap<Instruction*, Instruction*> coalesced;

  Coalesce(coalesced, order);

  // Compare two intervals and return the one that starts first.
  auto starts_first = [&](unsigned a, unsigned b) {
    Interval& ia = instruction_interval_[a];
    Interval& ib = instruction_interval_[b];
    return ia.Start() < ib.Start() || (ia.Start() == ib.Start() && a < b);
  };

  // Compare two intervals and return the one that starts first. If two
  // intervals end at the same place, schedule the instruction before the
  // operands.

  auto ends_first = [&](unsigned a, unsigned b) {
    auto& a_interval = instruction_interval_[a];
    auto& b_interval = instruction_interval_[b];
    if (b_interval.End() == a_interval.End()) {
      return b_interval.Start() > a_interval.Start() ||
             (b_interval.Start() == a_interval.Start() && b > a);
    }
    return b_interval.End() > a_interval.End();
  };

  using InstList = llvh::SmallVector<unsigned, 32>;

  // Performance: build the priority queue via heapify (O(n)) instead of
  // repeated pushes (O(n log n)).
  InstList all_intervals;
  all_intervals.reserve(GetMaxInstrIndex());
  for (unsigned i = 0, e = GetMaxInstrIndex(); i < e; i++) {
    all_intervals.push_back(i);
  }
  std::priority_queue<unsigned, InstList, decltype(ends_first)> intervals(
      ends_first, std::move(all_intervals));

  std::priority_queue<unsigned, InstList, decltype(starts_first)>
      live_intervals_queue(starts_first);

  // Perform the register allocation:
  while (!intervals.empty()) {
    unsigned inst_idx = intervals.top();
    intervals.pop();
    Instruction* inst = instructions_by_numbers_[inst_idx];
    Interval& inst_interval = instruction_interval_[inst_idx];
    unsigned current_index = inst_interval.End();

    // Free all of the intervals that start after the current index.
    while (!live_intervals_queue.empty()) {
      unsigned top_idx = live_intervals_queue.top();
      Interval& range = instruction_interval_[top_idx];
      // Flush empty intervals and intervals that finished after our index.
      bool non_empty_interval = range.Size();
      if (range.Start() < current_index && non_empty_interval) {
        break;
      }

      live_intervals_queue.pop();

      Instruction* i = instructions_by_numbers_[top_idx];
      Register r = GetRegister(i);
      if (r.GetIndex() < prefix_reg_) {
        // do not free the preallocated registers
        HandleInstruction(i);
        continue;
      }

      file_.KillRegister(r);
      HandleInstruction(i);
    }

    // Don't try to allocate registers that were merged into other live
    // intervals.
    if (coalesced.count(inst)) {
      continue;
    }

    // Allocate a register for the live interval that we are currently handling.
    if (!IsAllocated(inst)) {
      Register R = file_.AllocateRegister();
      UpdateRegister(inst, R);
    }

    // Mark the current instruction as live and remember to perform target
    // specific calls when we are done with the bundle.
    live_intervals_queue.push(inst_idx);
  }  // For each instruction in the function.

  // Free the remaining intervals.
  while (!live_intervals_queue.empty()) {
    Instruction* i = instructions_by_numbers_[live_intervals_queue.top()];
    Register r = GetRegister(i);
    if (r.GetIndex() >= prefix_reg_) {
      file_.KillRegister(r);
    }
    HandleInstruction(i);
    live_intervals_queue.pop();
  }

  // Allocate registers for the coalesced registers.
  for (auto& RP : coalesced) {
    if (IsAllocated(RP.first)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: RegisterAllocator::Allocate found already-allocated "
          "coalesced instruction");
    }
    Instruction* dest = RP.second;
    while (coalesced.count(dest)) {
      dest = coalesced[dest];
    }
    UpdateRegister(RP.first, GetRegister(dest));
  }

  RemapRegByExecOrder(order, coalesced);

  // For CallInst, ensure the callee is placed in a fresh "top" register above
  // other live values, to avoid VM argument materialization clobbers.
  OpBuilder builder;
  builder.SetModuleOp(f_->GetIRCtx()->GetMainMod());
  for (auto* bb : order) {
    for (auto it = bb->InstBegin(); it != bb->InstEnd(); ++it) {
      Instruction* i = *it;
      if (auto* call_inst = llvh::dyn_cast<CallInst>(i)) {
        ProcessCallInst(&builder, call_inst);
      }
    }
  }
  ShuffleRegistersToMinimize();
}

void RegisterAllocator::InitMovInstInterval(Instruction* mov_inst,
                                            Instruction* target_inst) {
  unsigned mov_inst_id = GetInstructionNumber(mov_inst);
  unsigned target_inst_id = GetInstructionNumber(target_inst);

  // The call-func MOV is a transient value: it only needs to be live at the
  // call-site so the VM can read the callee.
  //
  // IMPORTANT:
  // - The VM materializes arguments *before* executing the call.
  // - The call-site liveness checks in this codebase use the point
  //   (call_idx + 1).
  //
  // Therefore we model the call-func MOV as live only on the minimal segment
  // that covers the call-site point, and NOT live at the preceding point.
  // Otherwise, it may be incorrectly considered live at a previous call-site
  // when calls are adjacent, leading to false conflicts.
  Interval mov_interval(target_inst_id + 1, target_inst_id + 2);
  instruction_interval_[mov_inst_id] = mov_interval;
}

void RegisterAllocator::Preallocate() {
  uint32_t idx = 0;
  auto assign_fix_reg = [&](Value* val) -> Register {
    if (IsAllocated(val)) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: RegisterAllocator::Preallocate encountered "
          "already-allocated value");
    }
    Register reg = file_.AllocateRegister(idx++);
    UpdateRegister(val, reg);
    // Mark preallocated values as fixed so later optimization passes won't
    // try to reassign their physical registers.
    val->SetFixReg(true);
    return reg;
  };

  // 1. assign the fix virtual registers for parameter at first
  llvh::for_each(f_->GetParams(), [&](Parameter* val) { assign_fix_reg(val); });

  if (f_->IsToplevelFunc()) {
    // for toplevel function, prealloc the toplevel global vars
    if (f_->GetParamSize() != 0) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: RegisterAllocator::Preallocate toplevel function "
          "must not have parameters");
    }
    if (idx != 0) {
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: RegisterAllocator::Preallocate expected idx to "
          "start from zero for toplevel function");
    }
    auto& toplevel_vars = f_->GetIRCtx()->GetToplevelVariables();
    for (const auto& item : toplevel_vars) {
      if (item.second->GetKind() == ValueKind::First_ValueKind) {
        throw ::lynx::lepus::CompileException(
            "Lepus IR error: RegisterAllocator::Preallocate found deleted "
            "toplevel variable instruction");
      }
      assign_fix_reg(item.second);
    }
  }

  prefix_reg_ = idx;
}

void RegisterAllocator::CalculateLiveIntervals(ArrayRef<Block*> order) {
  /// Calculate the live intervals for each instruction. Start with a list of
  /// intervals that only contain the instruction itself.
  for (int i = 0, e = instructions_by_numbers_.size(); i < e; ++i) {
    // The instructions are ordered consecutively. The start offset of the
    // instruction is the index in the array plus one because the value starts
    // to live on the next instruction.
    instruction_interval_[i] = Interval(i + 1, i + 1);
  }

  // For each basic block in the liveness map:
  for (Block* bb : order) {
    BlockLifetimeInfo& liveness = block_liveness_[bb];

    auto start_offset = GetInstructionNumber(*bb->InstBegin());
    auto end_offset = GetInstructionNumber(bb->GetTerminator());

    // Register fly-through basic blocks (basic blocks where the value enters)
    // and leavs without doing anything to any of the operands.
    for (int i : liveness.live_out_.set_bits()) {
      if (liveness.live_in_.test(i)) {
        instruction_interval_[i].Add(Segment(start_offset, end_offset + 1));
      }
    }

    // For each instruction in the block:
    for (auto it = bb->InstBegin(); it != bb->InstEnd(); ++it) {
      auto inst_offset = GetInstructionNumber(*it);
      // The instruction is defined in this basic block. Check if it is leaving
      // the basic block extend the interval until the end of the block.
      if (liveness.live_out_.test(inst_offset)) {
        instruction_interval_[inst_offset].Add(
            Segment(inst_offset + 1, end_offset + 1));
        if (liveness.live_in_.test(inst_offset)) {
          throw ::lynx::lepus::CompileException(
              "Lepus IR error: RegisterAllocator::CalculateLiveIntervals value "
              "is live-in but also killed in this block");
        }
      }

      // Extend the lifetime of the operands.
      auto inst = *it;
      for (int i = 0, e = inst->GetNumOperands(); i < e; i++) {
        auto inst_op = llvh::dyn_cast<Instruction>(inst->GetOperand(i));
        if (!inst_op) continue;

        if (!HasInstructionNumber(inst_op)) {
          if (!llvh::isa<PhiInst>(inst)) {
            throw ::lynx::lepus::CompileException(
                "Lepus IR error: RegisterAllocator::CalculateLiveIntervals "
                "non-PhiInst references value from dead code");
          }
          continue;
        }

        auto operand_idx = GetInstructionNumber(inst_op);
        // Extend the lifetime of the interval to reach this instruction.
        // Include this instruction in the interval in order to make sure that
        // the register is not freed before the use.

        auto start = operand_idx + 1;
        auto end = inst_offset + 1;
        if (start < end) {
          auto seg = Segment(operand_idx + 1, inst_offset + 1);
          instruction_interval_[operand_idx].Add(seg);
        }
      }

      // Extend the lifetime of the PHI to include the source basic blocks.
      if (auto* p = llvh::dyn_cast<PhiInst>(inst)) {
        for (int i = 0, e = p->GetNumEntries(); i < e; i++) {
          auto entry = p->GetEntry(i);
          // PhiInsts may reference instructions from dead code blocks
          // (which will be unnumbered and unallocated). Since the edge
          // is necessarily also dead, we can just skip it.
          if (!HasInstructionNumber(entry.second->GetTerminator())) continue;

          unsigned term_idx =
              GetInstructionNumber(entry.second->GetTerminator());
          Segment S(term_idx, term_idx + 1);
          instruction_interval_[inst_offset].Add(S);

          // Extend the lifetime of the predecessor to the end of the bb.
          if (auto* inst_op = llvh::dyn_cast<Instruction>(entry.first)) {
            auto pred_idx = GetInstructionNumber(inst_op);
            if (pred_idx + 1 <= term_idx) {
              auto s2 = Segment(pred_idx + 1, term_idx);
              instruction_interval_[pred_idx].Add(s2);
            }
          }
        }  // each pred.
      }

    }  // for each instruction in the block.
  }    // for each block.
}

void RegisterAllocator::Dump(std::ostream& os) {
#ifdef LEPUS_TEST
  LivenessRegAllocIRPrinter printer(f_->GetIRCtx(), *this, os);
  printer.VisitMethod(*f_);
#else
  (void)os;
#endif
}

Register RegisterAllocator::GetRegister(Value* i) {
  if (!IsAllocated(i)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterAllocator::GetRegister called for unallocated "
        "value");
  }
  return allocated_[i];
}

void RegisterAllocator::UpdateRegister(Value* i, Register r) {
  allocated_[i] = r;
}

void RegisterAllocator::InsertRegister(Value* i, Register r) {
  if (!r.IsValid()) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterAllocator::InsertRegister got invalid "
        "register");
  }
  allocated_.insert({i, r});
}

bool RegisterAllocator::IsAllocated(Value* i) { return allocated_.count(i); }

void RegisterAllocator::RemoveFromAllocated(Value* i) { allocated_.erase(i); }

void RegisterAllocator::RebuildRegisterFileFromAllocated() {
  if (allocated_.empty()) {
    file_.Rebuild(0, {});
    return;
  }

  llvh::SmallVector<unsigned, 32> used_regs;
  used_regs.reserve(allocated_.size());
  unsigned max_idx = 0;
  for (const auto& kv : allocated_) {
    Register r = kv.second;
    if (!r.IsValid()) continue;
    used_regs.push_back(r.GetIndex());
    max_idx = std::max(max_idx, r.GetIndex());
  }
  const unsigned new_size = used_regs.empty() ? 0 : (max_idx + 1);
  file_.Rebuild(new_size, used_regs);
}

Register RegisterAllocator::Reserve(unsigned count) {
  return file_.TailAllocateConsecutive(count);
}
Register RegisterAllocator::ExtendFromLast(unsigned count) {
  return file_.AllocateRegisterCluster(file_.GetMaxRegisterUsage(), count);
}

Register RegisterAllocator::Reserve(ArrayRef<Value*> values) {
  if (values.empty()) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: RegisterAllocator::Reserve can't reserve zero "
        "registers");
  }
  Register first = file_.TailAllocateConsecutive(values.size());

  Register t = first;
  for (auto* v : values) {
    if (v) allocated_[v] = t;
    t = t.GetConsecutive();
  }

  return first;
}

bool RegisterAllocator::HasInstructionNumber(Instruction* i) {
  return instruction_numbers_.count(i);
}

unsigned RegisterAllocator::GetInstructionNumber(Instruction* i) {
  auto it = instruction_numbers_.find(i);
  if (it != instruction_numbers_.end()) {
    return it->second;
  }

  instructions_by_numbers_.push_back(i);
  instruction_interval_.push_back(Interval());

  unsigned new_idx = instructions_by_numbers_.size() - 1;
  instruction_numbers_[i] = new_idx;
  return new_idx;
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

unsigned llvh::DenseMapInfo<lynx::lepus::ir::Register>::getHashValue(
    lynx::lepus::ir::Register val) {
  return val.GetIndex();
}

bool llvh::DenseMapInfo<lynx::lepus::ir::Register>::isEqual(
    lynx::lepus::ir::Register lhs, lynx::lepus::ir::Register rhs) {
  return lhs.GetIndex() == rhs.GetIndex();
}

namespace lynx {
namespace lepus {
namespace ir {

std::ostream& operator<<(std::ostream& os, const Register& reg) {
  if (!reg.IsValid()) {
    os << "Null";
  } else {
    os << "Reg" << reg.GetIndex();
  }

  return os;
}

std::ostream& operator<<(std::ostream& os, const Segment& segment) {
  if (segment.Empty()) {
    os << "[empty]";
    return os;
  }

  os << "[" << segment.start_ << "..." << segment.end_ << ") ";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Interval& interval) {
  Interval t = interval.Compress();
  for (auto& s : t.segments_) {
    os << s;
  }
  return os;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
