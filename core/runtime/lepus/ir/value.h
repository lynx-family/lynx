// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_VALUE_H_
#define CORE_RUNTIME_LEPUS_IR_VALUE_H_

#include <memory>
#include <string>
#include <utility>

#include "core/runtime/lepus/ir/attribute_kind.h"
#include "core/runtime/lepus/ir/attributes_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/SmallVector.h"

namespace lynx {
namespace lepus {
namespace ir {

class Attribute;
class Instruction;
class TypeOp;
class OpBuilder;
class Attribute;

enum class ValueKind : uint16_t {
  First_ValueKind,
#define DEF_VALUE(CLASS, PARENT) CLASS##Kind,
#define BEGIN_VALUE(CLASS, PARENT) First_##CLASS##Kind,
#define DEF_TAG(NAME, PARENT) NAME##Kind,
#define END_VALUE(CLASS) Last_##CLASS##Kind,
#define MARK_VALUE(CLASS) CLASS##Kind,
#define MARK_FIRST(CLASS, PARENT) First_##CLASS##Kind,
#define MARK_LAST(CLASS) Last_##CLASS##Kind,

#define ENABLE_MIR_INSTR 1
#include "core/runtime/lepus/ir/value_kinds.def"
#undef ENABLE_MIR_INSTR

  Last_ValueKind,
};

class Value {
 public:
  using UseListTy = llvh::SmallVector<Instruction*, 2>;
  using Use = std::pair<Value*, unsigned>;
  using attr_iterator = Attributes::iterator;
  using attr_range = Attributes::iterator_range;

 private:
  friend class ModuleOp;
  friend class IRBuilder;

  ValueKind kind_;

  OpBuilder* builder_;

  std::unique_ptr<Attributes> attrs_;

  /// The type of the value.
  TypeOp* value_type_ = nullptr;

  /// A list of users of this instruction.
  UseListTy users_;

 public:
  // Instances of Value are not supposed to be deleted directly because we
  // want to avoid defining a virtual destructor. The private operator
  // delete can only be invoked by the \c destroy() method.

  // Use sized deallocation to speed things up, if it is available
  void operator delete(void* p) { ::operator delete(p); }

 protected:
  explicit Value(ValueKind k, OpBuilder* builder);
  explicit Value(ValueKind k, OpBuilder* builder, Attributes* attrs);
  ~Value();
  void SetKind(ValueKind kind) { this->kind_ = kind; }

 public:
  Value(const Value&) = delete;
  void operator=(const Value&) = delete;

  /// Run a Value's destructor and deallocate its memory.
  static void Destroy(Value* v);

  /// \return the users of the value.
  const UseListTy& GetUsers() const;

  /// \returns the number of users the value has.
  unsigned GetNumUsers() const;

  /// \returns true if the value has some users.
  bool HasUsers() const;

  /// \returns true if the value has only one user.
  bool HasOneUser() const;

  /// Remove the first occurrence of \p Inst from the Users list.
  void RemoveUse(Use u);

  /// Add the instruction \p Inst to the Users list.
  /// \returns the use handle.
  Value::Use AddUser(Instruction* inst);

  /// Replaces all uses of the current value with \p Other.
  void ReplaceAllUsesWith(Value* other);

  /// Replace all uses of the current value with \p Other except Other
  void ReplaceOtherUsesWith(Value* other);

  /// \returns the kind of the value.
  ValueKind GetKind() const { return kind_; }

  /// \returns the string representation of the Value kind.
  ///
  /// NOTE: This returns an owning `std::string` so it can be directly used in
  /// exceptions and test assertions (which typically stream to `std::ostream`).
  std::string GetKindStr() const;

  /// Sets a new type \p type to the value.
  void SetType(TypeOp* type) { value_type_ = type; }

  TypeOp* GetType() const { return value_type_; }

  OpBuilder* GetBuilder() { return builder_; }

  // --------------------  attributes helper function --------------------
  void CopyAttrs(Value* other) { GetAttr()->CopyFrom(other->GetAttr()); }

  Attributes* GetAttr() const { return attrs_.get(); }

  attr_iterator AttrBegin() { return GetAttr()->begin(); }
  attr_iterator AttrEnd() { return GetAttr()->end(); }
  attr_range AttributesRange() { return GetAttr()->range(); }

  uint32_t GetAttrSize() { return GetAttr()->GetAttrSize(); }

#define ATTRIBUTE(name, desc, type, type_name) void Set##name(type val);
#include "core/runtime/lepus/ir/attributes.def"
#undef ATTRIBUTE

#define NON_BOOL_ATTRIBUTE(name, desc, type, type_name) type Get##name() const;
#include "core/runtime/lepus/ir/attributes.def"
#undef NON_BOOL_ATTRIBUTE

#define BOOL_ATTRIBUTE(name, desc, type, type_name) type Is##name() const;
#include "core/runtime/lepus/ir/attributes.def"
#undef BOOL_ATTRIBUTE

  void RegisterAttr(SpecificAttr attr, Attribute* init);
  bool HasAttr(SpecificAttr attr);

  static bool classof(const Value*) { return true; }

  using iterator = UseListTy::iterator;
  using const_iterator = UseListTy::const_iterator;

  inline const_iterator UsersBegin() const { return users_.begin(); }
  inline const_iterator UsersEnd() const { return users_.end(); }
  inline iterator UsersBegin() { return users_.begin(); }
  inline iterator UsersEnd() { return users_.end(); }
};

static inline bool KindInRange(ValueKind kind, ValueKind from, ValueKind to) {
  return kind > from && kind < to;
}

/// Return true if the specified kind falls in the range of the specified class.
#define LEPUS_IR_KIND_IN_CLASS(kind, CLASS)         \
  KindInRange(kind, ValueKind::First_##CLASS##Kind, \
              ValueKind::Last_##CLASS##Kind)

}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_VALUE_H_
