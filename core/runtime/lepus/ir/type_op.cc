// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/type_op.h"

#include "base/include/value/base_value.h"
#include "core/runtime/lepus/ir/module_op.h"
#include "core/runtime/lepus/ir/primitive_typeop.h"

namespace lynx {
namespace lepus {
namespace ir {
TypeOp::TypeOp(ValueKind value_kind, Block* parent, OpBuilder* builder,
               int64_t location, TypeKind kind)
    : Operation(value_kind, parent, builder, location), kind_(kind) {}

TypeOp* TypeOp::CreateNull(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Null);
}
TypeOp* TypeOp::CreateUndefined(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Undefined);
}
TypeOp* TypeOp::CreateNullOrUndefined(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder,
                                      TypeOp::TypeKind::NullOrUndefined);
}
TypeOp* TypeOp::CreateBoolean(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Boolean);
}
TypeOp* TypeOp::CreateClosure(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Closure);
}
TypeOp* TypeOp::CreateRegExp(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::RegExp);
}
TypeOp* TypeOp::CreateStringProtoAPI(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder,
                                      TypeOp::TypeKind::StringProtoAPI);
}
TypeOp* TypeOp::CreateBuiltinFuncTable(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder,
                                      TypeOp::TypeKind::BuiltinFuncTable);
}
TypeOp* TypeOp::CreateString(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::String);
}
TypeOp* TypeOp::CreateTable(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Object);
}
TypeOp* TypeOp::CreateArray(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Array);
}
TypeOp* TypeOp::CreateInt8(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Int8);
}
TypeOp* TypeOp::CreateInt32(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Int32);
}
TypeOp* TypeOp::CreateInt64(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Int64);
}
TypeOp* TypeOp::CreateNumber(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Number);
}
TypeOp* TypeOp::CreateUint8(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Uint8);
}
TypeOp* TypeOp::CreateUint32(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Uint32);
}
TypeOp* TypeOp::CreateUint64(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Uint64);
}
TypeOp* TypeOp::CreateFloat64(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Float64);
}
TypeOp* TypeOp::CreateInvalidType(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::INVALID_TYPE);
}

TypeOp* TypeOp::CreateAnyType(OpBuilder* builder) {
  return PrimitiveTypeOp::GetInstance(builder, TypeOp::TypeKind::Any);
}

void TypeOp::Print(std::ostream& os) const {
  if (!IsValidType()) {
    os << "notype";
    return;
  }
  os << GetKindStr(kind_).str();
}

TypeOp* TypeOp::GetValueTypeOp(OpBuilder* builder, lynx::lepus::Value* value) {
  switch (value->Type()) {
    case lepus::ValueType::Value_Nil:
      return TypeOp::CreateNull(builder);
    case lepus::ValueType::Value_Undefined:
      return TypeOp::CreateUndefined(builder);
    case lepus::ValueType::Value_Double:
      return TypeOp::CreateFloat64(builder);
    case lepus::ValueType::Value_Int32:
      return TypeOp::CreateInt32(builder);
    case lepus::ValueType::Value_Int64:
      return TypeOp::CreateInt64(builder);
    case lepus::ValueType::Value_UInt32:
      return TypeOp::CreateUint32(builder);
    case lepus::ValueType::Value_UInt64:
      return TypeOp::CreateUint64(builder);
    case lepus::ValueType::Value_Bool:
      return TypeOp::CreateBoolean(builder);
    case lepus::ValueType::Value_String:
      return TypeOp::CreateString(builder);
    case lepus::ValueType::Value_Table:
      return TypeOp::CreateTable(builder);
    case lepus::ValueType::Value_Array:
      return TypeOp::CreateArray(builder);
    case lepus::ValueType::Value_Closure:
      return TypeOp::CreateClosure(builder);
    case lepus::ValueType::Value_RegExp:
      return TypeOp::CreateRegExp(builder);
    case lepus::ValueType::Value_FunctionTable:
      return TypeOp::CreateBuiltinFuncTable(builder);
    default:
      return TypeOp::CreateAnyType(builder);
  }
}

bool TypeOp::operator==(const TypeOp& rhs) const {
  if (kind_ != rhs.kind_) return false;
  if (kind_ == TypeOp::TypeKind::NullOrUndefined &&
      rhs.kind_ == TypeOp::TypeKind::NullOrUndefined)
    return false;
  if (kind_ == TypeOp::TypeKind::Number ||
      rhs.kind_ == TypeOp::TypeKind::Number)
    return false;

  return true;
}

std::ostream& operator<<(std::ostream& os, const TypeOp& t) {
  t.print(os);
  return os;
}

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

void llvh::ilist_alloc_traits<::lynx::lepus::ir::TypeOp>::deleteNode(
    ::lynx::lepus::ir::TypeOp* v) {
  ::lynx::lepus::ir::Value::Destroy(v);
}
