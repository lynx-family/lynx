// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_ATTRIBUTES_H_
#define CORE_RUNTIME_LEPUS_IR_ATTRIBUTES_H_

#include <map>
#include <string>

#include "core/runtime/lepus/ir/attributes_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/StringRef.h"
#include "core/runtime/lepus/ir/value.h"

namespace lynx {
namespace lepus {
namespace ir {

class IRContext;

class Attribute : public Value {
 public:
  explicit Attribute(ValueKind kind, IRContext* ir_ctx,
                     AttrEntryKind entry_kind);
  AttrEntryKind GetEntryKind() { return kind_; }

 private:
  [[maybe_unused]] IRContext* ir_ctx_ = nullptr;
  AttrEntryKind kind_;
};

class ValueAttr : public Attribute, public llvh::FoldingSetNode {
 public:
  explicit ValueAttr(Value* val)
      : Attribute(ValueKind::ValueAttrKind, nullptr, ValueAttrEntry),
        val_(val) {}

  Value* GetVal() const { return val_; }

  static void Profile(llvh::FoldingSetNodeID& id, Value* value) {
    id.AddInteger(llvh::Int64ToBits((int64_t)value));
  }

  void Profile(llvh::FoldingSetNodeID& id) const {
    ValueAttr::Profile(id, val_);
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::ValueAttrKind;
  }

 private:
  Value* val_;
};

class UnsignedAttr : public Attribute, public llvh::FoldingSetNode {
 public:
  explicit UnsignedAttr(unsigned val)
      : Attribute(ValueKind::UnsignedAttrKind, nullptr, UnsignedAttrEntry),
        val_(val) {}

  unsigned GetVal() const { return val_; }

  static void Profile(llvh::FoldingSetNodeID& id, int64_t value) {
    id.AddInteger(llvh::Int64ToBits(value));
  }

  void Profile(llvh::FoldingSetNodeID& id) const {
    UnsignedAttr::Profile(id, val_);
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::UnsignedAttrKind;
  }

 private:
  unsigned val_;
};

class BoolAttr : public Attribute, public llvh::FoldingSetNode {
 public:
  explicit BoolAttr(bool val)
      : Attribute(ValueKind::BoolAttrKind, nullptr, BoolAttrEntry), val_(val) {}

  bool GetVal() const { return val_; }

  static void Profile(llvh::FoldingSetNodeID& id, int64_t value) {
    id.AddInteger(llvh::Int64ToBits(value));
  }

  void Profile(llvh::FoldingSetNodeID& id) const {
    BoolAttr::Profile(id, val_);
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::BoolAttrKind;
  }

 private:
  bool val_;
};

class StringAttr : public Attribute, public llvh::FoldingSetNode {
 public:
  explicit StringAttr(std::string val)
      : Attribute(ValueKind::StringAttrKind, nullptr, StringAttrEntry),
        val_(val) {}

  const std::string GetVal() const { return val_; }

  static void Profile(llvh::FoldingSetNodeID& id, std::string value) {
    llvh::StringRef key(value);
    id.AddString(key);
  }

  void Profile(llvh::FoldingSetNodeID& id) const {
    StringAttr::Profile(id, val_);
  }

  static bool classof(const Value* v) {
    return v->GetKind() == ValueKind::StringAttrKind;
  }

 private:
  std::string val_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_ATTRIBUTES_H_
