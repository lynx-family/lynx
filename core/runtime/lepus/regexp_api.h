// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_REGEXP_API_H_
#define CORE_RUNTIME_LEPUS_REGEXP_API_H_

#include "core/runtime/lepus/builtin.h"

namespace lynx {
namespace lepus {
const RestrictedValue& GetRegexPrototypeAPI(const base::String& key);
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_REGEXP_API_H_
