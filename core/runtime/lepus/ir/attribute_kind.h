// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_LEPUS_IR_ATTRIBUTE_KIND_H_
#define CORE_RUNTIME_LEPUS_IR_ATTRIBUTE_KIND_H_

#include <cstdint>

namespace lynx {
namespace lepus {
namespace ir {

enum class SpecificAttr : uint32_t {
  SA_ATTR_START = 0,
#define ATTRIBUTE(name, desc, TYPE, TYPENAME) SA_##name,
#include "core/runtime/lepus/ir/attributes.def"
#undef ATTRIBUTE
  SA_CNT
};
static_assert(static_cast<uint32_t>(SpecificAttr::SA_CNT) <= UINT64_MAX,
              "too many attributes");
}  // namespace ir
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_IR_ATTRIBUTE_KIND_H_
