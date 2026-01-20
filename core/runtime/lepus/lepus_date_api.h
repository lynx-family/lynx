// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_LEPUS_DATE_API_H_
#define CORE_RUNTIME_LEPUS_LEPUS_DATE_API_H_

#include <string>
#include <vector>

#include "core/runtime/lepus/builtin.h"

namespace lynx {
namespace lepus {

void RegisterLepusDateAPI(Context* ctx);

const RestrictedValue& GetDatePrototypeAPI(const base::String& key);

const std::vector<std::string>& DateContent();

}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_LEPUS_DATE_API_H_
