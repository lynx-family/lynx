// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_INSTRUCTION_H_
#define CORE_RUNTIME_LEPUS_IR_INSTRUCTION_H_

#include <string>

#include "core/runtime/lepus/ir/attributes_base.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ilist.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ilist_node.h"
#include "core/runtime/lepus/ir/operation.h"
#include "core/runtime/lepus/ir/side_effect.h"
#include "core/runtime/lepus/ir/value.h"
#include "core/runtime/lepus/ir/value_forward_declare.h"

namespace lynx {
namespace lepus {
namespace ir {

class Instruction : public llvh::ilist_node<Instruction>, public Operation {
  friend class Value;
  NON_COPYABLE(Instruction);

  llvh::SmallVector<Value::Use, 2> operands_;

 protected:
  explicit Instruction(ValueKind kind, Block* parent, OpBuilder* builder,
                       int64_t location)
      : Operation(kind, parent, builder, location) {
    RegisterRequiredAttr();
  }

  explicit Instruction(ValueKind kind, Attributes* attrs, Block* parent,
                       OpBuilder* builder, int64_t location)
      : Operation(kind, attrs, parent, builder, location) {}

  explicit Instruction(Block* parent, OpBuilder* builder,
                       const Instruction* src, llvh::ArrayRef<Value*> operands);

 public:
  void RegisterRequiredAttr();
  void SetOperand(Value* val, unsigned index);
  Value* GetOperand(unsigned index) const;
  unsigned GetNumOperands() const;
  void RemoveOperand(unsigned index);
  void PushOperand(Value* val);

  /// \return whether this instruction has an output value.
  bool HasOutput() const;

  /// Replace the first operand from \p From to \p To. The value \p From must
  /// be an operand of the instruction. The method only replaces the first
  /// occurrence of \p From.
  void ReplaceFirstOperandWith(Value* old_value, Value* new_value);

  void MoveBefore(Instruction* later);

  /// Return the name of the instruction.
  std::string GetName() const;

  /// \returns the side effect of the instruction.
  SideEffect GetSideEffect() const;

  /// Return the hash code the this instruction.
  llvh::hash_code GetHashCode() const;

  /// Return true if \p RHS is equal to this instruction.
  bool IsIdenticalTo(const Instruction* rhs) const;

  static bool classof(const Value* v) {
    return LEPUS_IR_KIND_IN_CLASS(v->GetKind(), Instruction);
  }
};
}  // namespace ir

}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_INSTRUCTION_H_
