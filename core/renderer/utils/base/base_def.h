// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RENDERER_UTILS_BASE_BASE_DEF_H_
#define CORE_RENDERER_UTILS_BASE_BASE_DEF_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/vector.h"
#include "core/renderer/events/events.h"
#include "core/renderer/events/gesture.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

// base_def.h is imported multiple times by tasm_base_unittest.cc
// Use inner macro to to avoid redefinitions.
#ifndef CORE_RENDERER_UTILS_BASE_BASE_INNER_DEF_H_
#define CORE_RENDERER_UTILS_BASE_BASE_INNER_DEF_H_

// 0 to 6 children account for more than 99%.
constexpr const static size_t kChildrenInlineVectorSize = 6;

// 8-CSS-classes covers most cases.
using ClassList = base::InlineVector<base::String, 8>;
using AttrMap = std::unordered_map<base::String, lepus::Value>;
using DataMap = std::unordered_map<base::String, lepus::Value>;
using EventMap =
    std::unordered_map<base::String, std::unique_ptr<EventHandler>>;
using GestureMap =
    std::unordered_map<uint32_t, std::shared_ptr<GestureDetector>>;

using AttrUMap = std::unordered_map<base::String, lepus::Value>;

using BuiltinAttrMap = std::unordered_map<uint32_t, lepus::Value>;

static constexpr const char kListNodeTag[] = "list";
static constexpr const char kGlobalBind[] = "global-bindEvent";
static constexpr const char kSystemInfo[] = "SystemInfo";
static constexpr const char kGlobalPropsKey[] = "__globalProps";
static constexpr const char kInitData[] = "initData";

// invalid element impl id
static constexpr int32_t kInvalidImplId = 0;

// initial element impl id
static constexpr int32_t kInitialImplId = 10;

// Enlarge `PseudoState` if has more states...

#endif  // CORE_RENDERER_UTILS_BASE_BASE_INNER_DEF_H_

/*
To make the code more concise, add the following macro for inspector. Only exec
expression when ENABLE_INSPECTOR == 1. For example, the following code
```
#if ENABLE_INSPECTOR
if (GetDevToolFlag() && GetRadonPlug()) {
    create_plug_element_ = true;
}
#endif
```
can be refactored as

```
EXEC_EXPR_FOR_INSPECTOR(
    if (GetDevToolFlag() && GetRadonPlug()) {
        create_plug_element_ = true;
    }
);
```
*/
#if ENABLE_INSPECTOR
#define EXEC_EXPR_FOR_INSPECTOR(...) \
  do {                               \
    __VA_ARGS__;                     \
  } while (0)
#else
#define EXEC_EXPR_FOR_INSPECTOR(...)
#endif  // ENABLE_INSPECTOR

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_BASE_BASE_DEF_H_
