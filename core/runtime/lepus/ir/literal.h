// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_LITERAL_H_
#define CORE_RUNTIME_LEPUS_IR_LITERAL_H_

#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/FoldingSet.h"
#include "core/runtime/lepus/ir/opt_value.h"
#include "core/runtime/lepus/ir/type_op.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {
class Literal : public Value {
  NON_COPYABLE(Literal);

 public:
  explicit Literal(ValueKind k) : Value(k, nullptr) {}
  static bool classof(const Value* v) {
    return LEPUS_IR_KIND_IN_CLASS(v->GetKind(), Literal);
  }
};

class LiteralUndefined : public Literal {
  NON_COPYABLE(LiteralUndefined);

 public:
  explicit LiteralUndefined() : Literal(ValueKind::LiteralUndefinedKind) {}
  void SetLiteralUndefinedType(OpBuilder* builder) {
    SetType(TypeOp::CreateUndefined(builder));
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::LiteralUndefinedKind;
  }
};

class LiteralNull : public Literal {
  NON_COPYABLE(LiteralNull);

 public:
  explicit LiteralNull() : Literal(ValueKind::LiteralNullKind) {}
  void SetLiteralNullType(OpBuilder* builder) {
    SetType(TypeOp::CreateNull(builder));
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::LiteralNullKind;
  }
};

class LiteralInt8 : public Literal, public llvh::FoldingSetNode {
  int8_t value_;

 public:
  NON_COPYABLE(LiteralInt8);

  int8_t GetValue() const { return value_; }

  explicit LiteralInt8(OpBuilder* builder, int8_t val)
      : Literal(ValueKind::LiteralInt8Kind), value_(val) {
    SetType(TypeOp::CreateInt8(builder));
  }

  static void Profile(llvh::FoldingSetNodeID& id, OpBuilder* builder,
                      int64_t value) {
    id.AddInteger(llvh::Int64ToBits(value));
  }

  void Profile(llvh::FoldingSetNodeID& id) const {
    LiteralInt8::Profile(id, nullptr, value_);
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::LiteralInt8Kind;
  }
};

class LiteralInt32 : public Literal, public llvh::FoldingSetNode {
  int32_t value_;

 public:
  LiteralInt32(const LiteralInt32&) = delete;
  void operator=(const LiteralInt32&) = delete;

  int32_t GetValue() const { return value_; }

  explicit LiteralInt32(OpBuilder* builder, int64_t val)
      : Literal(ValueKind::LiteralInt32Kind), value_(val) {
    SetType(TypeOp::CreateInt32(builder));
  }

  static void Profile(llvh::FoldingSetNodeID& id, OpBuilder* builder,
                      int64_t value) {
    id.AddInteger(llvh::Int64ToBits(value));
  }

  void Profile(llvh::FoldingSetNodeID& id) const {
    LiteralInt32::Profile(id, nullptr, value_);
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::LiteralInt32Kind;
  }
};

class LiteralUint8 : public Literal, public llvh::FoldingSetNode {
  uint8_t value_;

 public:
  NON_COPYABLE(LiteralUint8);

  uint8_t GetValue() const { return value_; }

  explicit LiteralUint8(OpBuilder* builder, uint8_t val)
      : Literal(ValueKind::LiteralUint8Kind), value_(val) {
    SetType(TypeOp::CreateUint8(builder));
  }

  static void Profile(llvh::FoldingSetNodeID& id, OpBuilder* builder,
                      uint64_t value) {
    id.AddInteger(llvh::Int64ToBits(value));
  }

  void Profile(llvh::FoldingSetNodeID& id) const {
    LiteralUint8::Profile(id, nullptr, value_);
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::LiteralUint8Kind;
  }
};

class LiteralUint32 : public Literal, public llvh::FoldingSetNode {
  uint32_t value_;

 public:
  NON_COPYABLE(LiteralUint32);

  uint32_t GetValue() const { return value_; }

  explicit LiteralUint32(OpBuilder* builder, uint32_t val)
      : Literal(ValueKind::LiteralUint32Kind), value_(val) {
    SetType(TypeOp::CreateUint32(builder));
  }

  static void Profile(llvh::FoldingSetNodeID& id, OpBuilder* builder,
                      uint64_t value) {
    id.AddInteger(llvh::Int64ToBits(value));
  }

  void Profile(llvh::FoldingSetNodeID& id) const {
    LiteralUint8::Profile(id, nullptr, value_);
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::LiteralUint32Kind;
  }
};

class LiteralFloat64 : public Literal, public llvh::FoldingSetNode {
  double value_;

 public:
  LiteralFloat64(const LiteralFloat64&) = delete;
  void operator=(const LiteralFloat64&) = delete;

  double GetValue() const { return value_; }

  explicit LiteralFloat64(OpBuilder* builder, double val)
      : Literal(ValueKind::LiteralFloat64Kind), value_(val) {
    SetType(TypeOp::CreateFloat64(builder));
  }

  static void Profile(llvh::FoldingSetNodeID& id, OpBuilder* builder,
                      double value) {
    id.AddInteger(llvh::DoubleToBits(value));
  }

  void Profile(llvh::FoldingSetNodeID& id) const {
    LiteralFloat64::Profile(id, nullptr, value_);
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::LiteralFloat64Kind;
  }
};

class LiteralBool : public Literal {
  LiteralBool(const LiteralBool&) = delete;
  void operator=(const LiteralBool&) = delete;
  bool value_;

 public:
  bool GetValue() const { return value_; }

  explicit LiteralBool(bool val)
      : Literal(ValueKind::LiteralBoolKind), value_(val) {}

  void SetLiteralBoolType(OpBuilder* builder) {
    SetType(TypeOp::CreateBoolean(builder));
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::LiteralBoolKind;
  }
};
}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_LITERAL_H_
