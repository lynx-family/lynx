// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_TYPE_OP_H_
#define CORE_RUNTIME_LEPUS_IR_TYPE_OP_H_

#include <ostream>
#include <string>

#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/ilist_node.h"
#include "core/runtime/lepus/ir/operation.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {

class Value;

namespace ir {

class TypeOp;
class IRContext;
class PrimitiveTypeOp;

#define TYPE_LIST_V(V)                  \
  V(Null, null)                         \
  V(Undefined, undefined)               \
  V(NullOrUndefined, nullOrUndefined)   \
  V(Boolean, boolean)                   \
  V(Int8, int8)                         \
  V(Uint8, uint8)                       \
  V(Int32, int32)                       \
  V(Uint32, uint32)                     \
  V(Uint64, uint64)                     \
  V(Int64, int64)                       \
  V(Float64, float64)                   \
  V(Number, number)                     \
  V(String, string)                     \
  V(Array, array)                       \
  V(Object, object)                     \
  V(Closure, func)                      \
  V(RegExp, regexp)                     \
  V(StringProtoAPI, stringProtoApi)     \
  V(BuiltinFuncTable, builtinFuncTable) \
  V(Any, any)

class TypeOp : public llvh::ilist_node<TypeOp>, public Operation {
  friend class Value;
  NON_COPYABLE(TypeOp);

 public:
  enum TypeKind : uint8_t {
#define DEF_TYPE_KIND(NAME, STR) NAME,
    TYPE_LIST_V(DEF_TYPE_KIND)
#undef DEF_TYPE_KIND
        INVALID_TYPE
  };

  static llvh::StringRef RetKindStr(TypeKind idx) {
    // The strings below match the values in TypeKind.
    static const char* const names[] = {
#define DEF_TYPE_STR(NAME, STR) #STR,
        TYPE_LIST_V(DEF_TYPE_STR)
#undef DEF_TYPE_STR
    };
    return names[(uint32_t)idx];
  }

 protected:
  explicit TypeOp(ValueKind value_kind, Block* parent, OpBuilder* builder,
                  int64_t location, TypeKind kind);

 public:
  constexpr bool IsInvalidType() const {
    return kind_ == TypeKind::INVALID_TYPE;
  }
  constexpr bool IsBooleanType() const { return kind_ == TypeKind::Boolean; }
  constexpr bool IsClosureType() const { return kind_ == TypeKind::Closure; }
  constexpr bool IsAnyType() const { return kind_ == TypeKind::Any; }
  constexpr bool IsTableType() const { return kind_ == TypeKind::Object; }
  constexpr bool IsStringProtoAPIType() const {
    return kind_ == TypeKind::StringProtoAPI;
  }
  constexpr bool IsBuiltinFuncTableType() const {
    return kind_ == TypeKind::BuiltinFuncTable;
  }
  constexpr bool IsArrayType() const { return kind_ == TypeKind::Array; }
  constexpr bool IsStringType() const { return kind_ == TypeKind::String; }
  constexpr bool IsNumberType() const {
    return (kind_ == TypeKind::Int8) || (kind_ == TypeKind::Int32) ||
           (kind_ == TypeKind::Int64) || (kind_ == TypeKind::Uint8) ||
           (kind_ == TypeKind::Uint32) || (kind_ == TypeKind::Uint64) ||
           (kind_ == TypeKind::Float64) || (kind_ == TypeKind::Number);
  }
  constexpr bool IsInt8Type() const { return kind_ == TypeKind::Int8; }
  constexpr bool IsInt32Type() const { return kind_ == TypeKind::Int32; }
  constexpr bool IsInt64Type() const { return kind_ == TypeKind::Int64; }
  constexpr bool IsIntType() const {
    return (kind_ == TypeKind::Int8) || (kind_ == TypeKind::Int64) ||
           (kind_ == TypeKind::Int32);
  }
  constexpr bool IsUint8Type() const { return kind_ == TypeKind::Uint8; }
  constexpr bool IsUint32Type() const { return kind_ == TypeKind::Uint32; }
  constexpr bool IsUintType() const {
    return (kind_ == TypeKind::Uint8) || (kind_ == TypeKind::Uint32);
  }
  constexpr bool IsNullOrUndefinedType() const {
    return kind_ == TypeKind::Null || kind_ == TypeKind::Undefined ||
           kind_ == TypeKind::NullOrUndefined;
  }
  constexpr bool IsFloat64Type() const { return kind_ == TypeKind::Float64; }
  constexpr bool IsNullType() const { return kind_ == TypeKind::Null; }
  constexpr bool IsUndefinedType() const {
    return kind_ == TypeKind::Undefined;
  }
  constexpr bool IsValidType() const { return kind_ != TypeKind::INVALID_TYPE; }
  TypeKind GetTypeKind() const { return kind_; }

  static TypeOp* CreateNull(OpBuilder* builder);
  static TypeOp* CreateUndefined(OpBuilder* builder);
  static TypeOp* CreateNullOrUndefined(OpBuilder* builder);
  static TypeOp* CreateBoolean(OpBuilder* builder);
  static TypeOp* CreateClosure(OpBuilder* builder);
  static TypeOp* CreateStringProtoAPI(OpBuilder* builder);
  static TypeOp* CreateBuiltinFuncTable(OpBuilder* builder);
  static TypeOp* CreateRegExp(OpBuilder* builder);
  static TypeOp* CreateString(OpBuilder* builder);
  static TypeOp* CreateTable(OpBuilder* builder);
  static TypeOp* CreateArray(OpBuilder* builder);
  static TypeOp* CreateInt8(OpBuilder* builder);
  static TypeOp* CreateInt32(OpBuilder* builder);
  static TypeOp* CreateInt64(OpBuilder* builder);
  static TypeOp* CreateUint8(OpBuilder* builder);
  static TypeOp* CreateUint32(OpBuilder* builder);
  static TypeOp* CreateUint64(OpBuilder* builder);
  static TypeOp* CreateFloat64(OpBuilder* builder);
  static TypeOp* CreateNumber(OpBuilder* builder);
  static TypeOp* CreateAnyType(OpBuilder* builder);
  static TypeOp* CreateInvalidType(OpBuilder* builder);

  static TypeOp* GetValueTypeOp(OpBuilder* builder, lynx::lepus::Value* value);

  bool operator==(const TypeOp& rhs) const;
  bool operator!=(const TypeOp& rhs) const { return !(*this == rhs); }
  void Print(std::ostream& os) const;
  void print(std::ostream& os) const { Print(os); }

  /// The hash of a Type is the hash of its opaque value.
  llvh::hash_code Hash() const {
    return llvh::hash_combine(llvh::hash_value(kind_), llvh::hash_value(this));
  }

  llvh::hash_code hash() const { return Hash(); }

 private:
  /// Return the string representation of the type at index \p idx.
  llvh::StringRef GetKindStr(TypeKind idx) const {
    return TypeOp::RetKindStr(idx);
  }

  /// Each bit represent the possibility of the type being the type that's
  /// represented in the enum entry.
  TypeKind kind_;
  bool was_generic_type_ = false;  // mark the type as generic
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

namespace lynx {
namespace lepus {
namespace ir {
std::ostream& operator<<(std::ostream& os, const TypeOp& t);
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_TYPE_OP_H_
