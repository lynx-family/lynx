// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/value.h"

#include "core/runtime/lepus/ir/dialects/mir/instrs.h"
#include "core/runtime/lepus/ir/instrs.h"
#include "core/runtime/lepus/ir/instruction.h"
#include "core/runtime/lepus/ir/op_builder.h"
#include "core/runtime/lepus/ir/primitive_typeop.h"

namespace lynx {
namespace lepus {
namespace ir {

Value::Value(ValueKind k, OpBuilder* builder) {
  kind_ = k;
  builder_ = builder;
  if (builder_ && builder_->GetMod())
    value_type_ = TypeOp::CreateInvalidType(builder_);
  attrs_ = std::make_unique<Attributes>();
}

Value::Value(ValueKind k, OpBuilder* builder, Attributes* attrs) {
  kind_ = k;
  this->builder_ = builder;
  this->attrs_ = std::make_unique<Attributes>(attrs);
}

Value::~Value() { attrs_.reset(); }

#define ATTRIBUTE(name, desc, TYPE, TYPENAME) \
  void Value::Set##name(TYPE val) { attrs_->Set##name(val); }
#include "core/runtime/lepus/ir/attributes.def"
#undef ATTRIBUTE

#define NON_BOOL_ATTRIBUTE(name, desc, TYPE, TYPENAME) \
  TYPE Value::Get##name() const { return attrs_->Get##name(); }
#include "core/runtime/lepus/ir/attributes.def"
#undef NON_BOOL_ATTRIBUTE

#define BOOL_ATTRIBUTE(name, desc, TYPE, TYPENAME) \
  TYPE Value::Is##name() const { return attrs_->Is##name(); }
#include "core/runtime/lepus/ir/attributes.def"
#undef BOOL_ATTRIBUTE

void Value::RegisterAttr(SpecificAttr attr, Attribute* init) {
  return attrs_->RegisterAttr(attr, init);
}

bool Value::HasAttr(SpecificAttr attr) { return attrs_->HasAttr(attr); }

// Make sure the ValueKinds.def tree is consistent with the class hierarchy.
#define QUOTE(X) #X
#define DEF_VALUE(CLASS, PARENT)                                              \
  static_assert(std::is_base_of<PARENT, CLASS>::value,                        \
                QUOTE(CLASS) " should directly inherit from " QUOTE(PARENT)); \
  static_assert(std::is_convertible<CLASS*, PARENT*>::value,                  \
                QUOTE(CLASS) " should publicly inherit from " QUOTE(PARENT)); \
  static_assert(                                                              \
      ValueKind::CLASS##Kind > ValueKind::First_##PARENT##Kind,               \
      QUOTE(CLASS) "Kind should be after First_" QUOTE(PARENT) "Kind");       \
  static_assert(                                                              \
      ValueKind::CLASS##Kind < ValueKind::Last_##PARENT##Kind,                \
      QUOTE(CLASS) "Kind should be before Last_" QUOTE(PARENT) "Kind");

#define ENABLE_MIR_INSTR 1

#include "core/runtime/lepus/ir/value_kinds.def"

#undef ENABLE_MIR_INSTR
#undef QUOTE

void Value::Destroy(Value* v) {
  if (!v) return;

  switch (v->kind_) {
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Value::Destroy encountered invalid ValueKind");
#define DEF_VALUE(XX, PARENT) \
  case ValueKind::XX##Kind:   \
    delete llvh::cast<XX>(v); \
    break;
#define DEF_TAG(XX, PARENT)       \
  case ValueKind::XX##Kind:       \
    delete llvh::cast<PARENT>(v); \
    break;

#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/value_kinds.def"
#undef ENABLE_MIR_INSTR
  }
}

std::string Value::GetKindStr() const {
  switch (kind_) {
    default:
      throw ::lynx::lepus::CompileException(
          "Lepus IR error: Value::GetKindStr encountered invalid ValueKind");
#define DEF_VALUE(XX, PARENT) \
  case ValueKind::XX##Kind:   \
    return std::string(#XX);
#define DEF_TAG(XX, PARENT) DEF_VALUE(XX, PARENT)

#define ENABLE_MIR_INSTR 1

#include "core/runtime/lepus/ir/value_kinds.def"

#undef ENABLE_MIR_INSTR
  }

  return "";
}

const Value::UseListTy& Value::GetUsers() const { return users_; }

unsigned Value::GetNumUsers() const { return users_.size(); }

bool Value::HasUsers() const { return users_.size(); }

bool Value::HasOneUser() const { return 1 == users_.size(); }

void Value::RemoveUse(Use u) {
  if (LEPUS_UNLIKELY(users_.empty())) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Value::RemoveUse called on empty user list");
  }
  if (LEPUS_UNLIKELY(u.first != this)) {
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Value::RemoveUse called with mismatched use owner");
  }

  // We don't care about the order of the operands in the use vector. One
  // cheap way to delete an element is to pop the last element and save it
  // on top of the element that we want to remove. This is faster than
  // moving the whole array.
  users_[u.second] = users_.back();
  users_.pop_back();

  // If we've changed the location of a use in the use list then we need to
  // update the operand in the user.
  if (u.second != users_.size()) {
    Use old_use = {this, static_cast<unsigned>(users_.size())};
    auto& operands = users_[u.second]->operands_;
    for (int i = 0, e = operands.size(); i < e; i++) {
      if (operands[i] == old_use) {
        operands[i] = {this, u.second};
        return;
      }
    }
    throw ::lynx::lepus::CompileException(
        "Lepus IR error: Value::RemoveUse can't find user in operand list");
  }
}

Value::Use Value::AddUser(Instruction* inst) {
  users_.push_back(inst);
  return {this, static_cast<unsigned>(users_.size() - 1)};
}

void Value::ReplaceAllUsesWith(Value* other) {
  if (this == other) {
    return;
  }

  // Ask the users of this value to unregister themselves. Notice that the
  // users modify and invalidate the iterators of Users.
  while (users_.size()) {
    users_[users_.size() - 1]->ReplaceFirstOperandWith(this, other);
  }
}

void Value::ReplaceOtherUsesWith(Value* other) {
  if (this == other) return;

  for (int i = users_.size() - 1; i >= 0; i--) {
    if (users_[i] != other) users_[i]->ReplaceFirstOperandWith(this, other);
  }
}
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
