// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_IR_ATTRIBUTES_BASE_H_
#define CORE_RUNTIME_LEPUS_IR_ATTRIBUTES_BASE_H_

#include <map>
#include <string>

#include "core/runtime/lepus/ir//attribute_kind.h"
#include "core/runtime/lepus/ir/ir_base.h"
#include "core/runtime/lepus/ir/llvh/include/llvh/ADT/iterator_range.h"
#include "core/runtime/lepus/ir/owning_folding_set.h"

namespace lynx {
namespace lepus {
namespace ir {

class IRContext;
class Attribute;
class Value;
class BoolAttr;

struct AttrDeleter {
  void operator()(Value* v);
};

enum AttrEntryKind : uint8_t {
  UnsignedAttrEntry = 0,
  BoolAttrEntry,
  StringAttrEntry,
  ValueAttrEntry,
};

class Attributes {
  NON_COPYABLE(Attributes);

 public:
  using iterator = std::map<SpecificAttr, const Attribute*>::iterator;
  using iterator_range = llvh::iterator_range<iterator>;

  explicit Attributes() { values_.clear(); }

  explicit Attributes(Attributes* others) {
    values_.clear();
    values_ = others->values_;
  }

  std::map<SpecificAttr, const Attribute*>& GetValues() { return values_; }

  void CopyFrom(Attributes* other) {
    auto& other_values = other->GetValues();
    for (auto iter : other_values) values_[iter.first] = iter.second;
  }

  iterator begin() { return values_.begin(); }

  iterator end() { return values_.end(); }

  iterator_range range() { return iterator_range(begin(), end()); }

#define ATTRIBUTE(name, desc, TYPE, TYPENAME) void Set##name(TYPE val);
#include "core/runtime/lepus/ir/attributes.def"
#undef ATTRIBUTE

#define NON_BOOL_ATTRIBUTE(name, desc, TYPE, TYPENAME) TYPE Get##name() const;
#include "core/runtime/lepus/ir/attributes.def"
#undef NON_BOOL_ATTRIBUTE

#define BOOL_ATTRIBUTE(name, desc, TYPE, TYPENAME) TYPE Is##name() const;
#include "core/runtime/lepus/ir/attributes.def"
#undef BOOL_ATTRIBUTE

  void RegisterAttr(SpecificAttr attr, const Attribute* init);
  bool HasAttr(SpecificAttr attr);

  uint32_t GetAttrSize() const { return values_.size(); }

  static Attribute* GetUnsignedAttr(AttrEntryKind kind, unsigned value);
  static Attribute* GetStringAttr(AttrEntryKind kind, std::string value);
  static Attribute* GetBoolAttr(AttrEntryKind kind, bool value);
  static Attribute* GetValueAttr(AttrEntryKind kind, Value* value);

 private:
  std::map<SpecificAttr, const Attribute*> values_;
};

}  // namespace ir
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_IR_ATTRIBUTES_BASE_H_
