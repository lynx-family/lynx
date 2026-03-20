// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_IR_DUMPER_H_
#define CORE_RUNTIME_LEPUS_IR_IR_DUMPER_H_

#include <map>
#include <memory>
#include <ostream>
#include <string>

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/ir_visitor.h"
#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {

class Value;
class Instruction;
class Block;
class FuncOp;
class ModuleOp;

class CondBranchInst;
// class ReturnInst;
class BranchInst;
class IRContext;

/// A utility class for naming IR values. This should only be used for
/// pretty-printing instructions and basic blocks.
class ValueNamer {
  /// The kind of the value associated with this number, and the actual number.
  struct ValueT {
    /// The kind of the value for which this number was allocated. If the kind
    /// changes, a new number will be allocated. A kind can change when an
    /// instruction is freed and a new one is allocated at the same address.
    ValueKind kind_;

    /// Generation of the namer when this number was visited. Unvisited values
    /// are "garbage collected".
    uint8_t visited_gen_;

    /// The unique (within a function) number label associated with this value.
    unsigned number_;

    ValueT(ValueKind kind, uint8_t visited_gen, unsigned int number)
        : kind_(kind), visited_gen_(visited_gen), number_(number) {}
  };

  /// Map from an IR Value pointer, to a number associated with it, plus some
  /// extra data.
  llvh::DenseMap<Value*, ValueT> map_{};

  /// The current generation. This is stamped in every value when we visit it.
  /// When the generation changes, all values that were not visited in the last
  /// generation are removed from the map.
  uint8_t current_gen_ = 0;

  /// Next number to allocate.
  unsigned counter_ = 0;

 public:
  ValueNamer() = default;

  /// Clear the map and reset the counter.
  void Clear();

  /// Advance the generation. This invalidates all numbers that were not visited
  /// in the previous generation.
  void NextGeneration();

  /// Return the number associated with \p v. If \p v is not in the map, or is
  /// in the map bit associated with a different kind, a new number is
  /// allocated.
  unsigned GetNumber(Value* v);
};

/// This class holds all state necessary for naming things in IR dumps.
class Namer {
  /// State needed per IR Function.
  struct PerFunction {
    ValueNamer inst_namer;
    ValueNamer bb_namer;

    void NextGeneration() {
      inst_namer.NextGeneration();
      bb_namer.NextGeneration();
    }

    void Clear() {
      inst_namer.Clear();
      bb_namer.Clear();
    }
  };

  /// Whether the state should persist across functions.
  const bool persistent_;

  /// Associates per-function state with an IR Function. Only in "persistent"
  /// mode.
  llvh::DenseMap<const FuncOp*, std::unique_ptr<PerFunction>> function_map_{};

  /// State that is used in non-persistent mode.
  std::unique_ptr<PerFunction> non_persistent_state_;

  /// The current state.
  PerFunction* cur_function_state_ = nullptr;

 public:
  explicit Namer(bool persistent) : persistent_(persistent) {
    if (!persistent_) {
      non_persistent_state_ = std::make_unique<PerFunction>();
      cur_function_state_ = non_persistent_state_.get();
    }
  }

  void NewFunction(const FuncOp* m) {
    if (persistent_) {
      auto [it, inserted] =
          function_map_.try_emplace(m, std::unique_ptr<PerFunction>());
      if (inserted) {
        it->second = std::make_unique<PerFunction>();
      }
      cur_function_state_ = it->second.get();
      cur_function_state_->NextGeneration();
    } else {
      cur_function_state_->Clear();
    }
  }

  /// Return the number associated with \p inst.
  unsigned GetInstNumber(Instruction* inst) {
    return cur_function_state_->inst_namer.GetNumber(inst);
  }
  /// Return the number associated with \p bb.
  unsigned GetBBNumber(Block* bb) {
    return cur_function_state_->bb_namer.GetNumber(bb);
  }
};

struct IRIndentRAII {
 public:
  explicit IRIndentRAII(unsigned& indent, uint32_t step = 2)
      : indent_(indent), step_(step) {
    indent_ += step_;
  }
  ~IRIndentRAII() { indent_ -= step_; }

 private:
  unsigned& indent_;
  uint32_t step_;
};

class IRPrinter {
 protected:
  IRContext* ir_ctx_;
  /// Indentation level.
  unsigned indent_;

  /// Output stream.
  std::ostream& os_;
  /// If set to true then we need to escape the quote mark because the output of
  /// this printer may be printed as a quoted label.
  bool need_escape_;

  /// A non-peristent namer used when one isn't provided by Context.
  std::unique_ptr<Namer> temp_namer_;

 public:
  /// State for naming values and variables.
  Namer& namer_;

  /// \param ctx  the Context
  /// \param use_persistent whether to use the persistent namer from Context
  /// \param ost  output stream
  /// \param escape whether to escape the quote mark.
  explicit IRPrinter(IRContext* ir_ctx, bool use_persistent, std::ostream& os,
                     bool escape);
  explicit IRPrinter(IRContext* ir_ctx, std::ostream& ost, bool escape = false)
      : IRPrinter(ir_ctx, true, ost, escape) {}

  virtual ~IRPrinter() = default;

  virtual void PrintBlock(const Block& block);
  virtual void PrintSourceLocation(int64_t loc);
  virtual void PrintMethodHeader(FuncOp* fun, bool skip_class = true);
  virtual void PrintValueLabel(Instruction* instr, Value* v, unsigned op_index);
  virtual void PrintTypeLabel(Value* v);
  virtual void PrintInstruction(Instruction* inst);
  virtual bool PrintInstructionDestination(Instruction* inst);
  void PrintType(TypeOp* type, std::ostream& output_stream);

  std::string GetQuoteSign() { return need_escape_ ? R"(\")" : R"(")"; }

  /// Quote the string if it has spaces.
  std::string QuoteStr(const std::string& name);
};

class IRDumper : public IRVisitor<IRDumper>, public IRPrinter {
 public:
  IRDumper(IRContext* ir_ctx, std::ostream& ost, bool escape = false)
      : IRPrinter(ir_ctx, true, ost, escape) {}
  void VisitInstruction(const Instruction& inst);
  void VisitBlock(const Block& block);
  void VisitMethod(const FuncOp& func);
  void VisitMethod(const FuncOp& M, llvh::ArrayRef<Block*> order);
  void VisitModule(const ModuleOp& mod);
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_IR_DUMPER_H_
