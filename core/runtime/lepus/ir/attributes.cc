// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/ir/attributes.h"

#include "core/runtime/lepus/ir/attributes_base.h"
#include "core/runtime/lepus/ir/ir_context.h"
#include "core/runtime/lepus/ir/module_op.h"

namespace lynx {
namespace lepus {
namespace ir {
Attribute::Attribute(ValueKind kind, IRContext* ir_ctx,
                     AttrEntryKind entry_kind)
    : Value(kind, nullptr), ir_ctx_(ir_ctx), kind_(entry_kind) {}

#define ATTRIBUTE(name, desc, TYPE, TYPENAME)                      \
  void Attributes::Set##name(TYPE val) {                           \
    values_[SpecificAttr::SA_##name] =                             \
        Attributes::Get##TYPENAME##Attr(TYPENAME##AttrEntry, val); \
  }
#include "core/runtime/lepus/ir/attributes.def"
#undef ATTRIBUTE

#define GET_ATTRIBUTE(name)                                                    \
  auto iter = values_.find(SpecificAttr::SA_##name);                           \
  if (iter == values_.end()) {                                                 \
    std::string msg =                                                          \
        std::string("Lepus IR error: missing attribute: ") + #name;            \
    throw ::lynx::lepus::CompileException(msg.c_str());                        \
  }                                                                            \
  if (auto* unsigned_attr = llvh::dyn_cast<UnsignedAttr>(iter->second))        \
    return unsigned_attr->GetVal();                                            \
  else if (auto* bool_attr = llvh::dyn_cast<BoolAttr>(iter->second))           \
    return bool_attr->GetVal();                                                \
  {                                                                            \
    std::string msg =                                                          \
        std::string("Lepus IR error: unexpected attribute entry kind for: ") + \
        #name;                                                                 \
    throw ::lynx::lepus::CompileException(msg.c_str());                        \
  }

#define UNSIGNED_ATTRIBUTE(name, desc, TYPE, TYPENAME) \
  TYPE Attributes::Get##name() const { GET_ATTRIBUTE(name); }
#include "core/runtime/lepus/ir/attributes.def"
#undef UNSIGNED_ATTRIBUTE

#define STRING_ATTRIBUTE(name, desc, TYPE, TYPENAME)                     \
  TYPE Attributes::Get##name() const {                                   \
    auto iter = values_.find(SpecificAttr::SA_##name);                   \
    if (iter == values_.end()) {                                         \
      std::string msg =                                                  \
          std::string("Lepus IR error: missing attribute: ") + #name;    \
      throw ::lynx::lepus::CompileException(msg.c_str());                \
    }                                                                    \
    if (auto* value_attr = llvh::dyn_cast<StringAttr>(iter->second))     \
      return value_attr->GetVal();                                       \
    {                                                                    \
      std::string msg =                                                  \
          std::string(                                                   \
              "Lepus IR error: unexpected attribute entry kind for: ") + \
          #name;                                                         \
      throw ::lynx::lepus::CompileException(msg.c_str());                \
    }                                                                    \
  }
#include "core/runtime/lepus/ir/attributes.def"
#undef STRING_ATTRIBUTE

#define VALUE_ATTRIBUTE(name, desc, TYPE, TYPENAME)                      \
  TYPE Attributes::Get##name() const {                                   \
    auto iter = values_.find(SpecificAttr::SA_##name);                   \
    if (iter == values_.end()) {                                         \
      std::string msg =                                                  \
          std::string("Lepus IR error: missing attribute: ") + #name;    \
      throw ::lynx::lepus::CompileException(msg.c_str());                \
    }                                                                    \
    if (auto* value_attr = llvh::dyn_cast<ValueAttr>(iter->second))      \
      return value_attr->GetVal();                                       \
    {                                                                    \
      std::string msg =                                                  \
          std::string(                                                   \
              "Lepus IR error: unexpected attribute entry kind for: ") + \
          #name;                                                         \
      throw ::lynx::lepus::CompileException(msg.c_str());                \
    }                                                                    \
  }
#include "core/runtime/lepus/ir/attributes.def"
#undef VALUE_ATTRIBUTE

#define BOOL_ATTRIBUTE(name, desc, TYPE, TYPENAME) \
  TYPE Attributes::Is##name() const { GET_ATTRIBUTE(name); }
#include "core/runtime/lepus/ir/attributes.def"
#undef BOOL_ATTRIBUTE

#define ATTRIBUTE_GETTER(type, NAME)                                       \
  Attribute* Attributes::Get##NAME##Attr(AttrEntryKind kind, type value) { \
    if (kind != NAME##AttrEntry) {                                         \
      std::string msg =                                                    \
          std::string("Lepus IR error: Attributes::Get" #NAME              \
                      "Attr called with mismatched AttrEntryKind");        \
      throw ::lynx::lepus::CompileException(msg.c_str());                  \
    }                                                                      \
    static OwningFoldingSet<NAME##Attr, AttrDeleter> attrs;                \
    return attrs.GetOrEmplaceWithNew(value).first;                         \
  }

ATTRIBUTE_GETTER(unsigned, Unsigned)
ATTRIBUTE_GETTER(bool, Bool)
ATTRIBUTE_GETTER(std::string, String)
ATTRIBUTE_GETTER(Value*, Value)

void Attributes::RegisterAttr(SpecificAttr attr, const Attribute* init) {
  values_[attr] = init;
}

bool Attributes::HasAttr(SpecificAttr attr) {
  return values_.find(attr) != values_.end();
}

void AttrDeleter::operator()(Value* v) { Value::Destroy(v); }
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
