// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_DATE_API_H_
#define CORE_RUNTIME_LEPUS_DATE_API_H_

#include <time.h>

#include <chrono>
#include <utility>

#include "core/runtime/lepus/restricted_value.h"

namespace lynx {
namespace lepus {
RestrictedValue Now(VMContext* context) {
  using std::chrono::milliseconds;
  using std::chrono::system_clock;
  using std::chrono::time_point;
  using std::chrono::time_point_cast;
  using time_stamp = time_point<system_clock, milliseconds>;
  time_stamp tp = time_point_cast<milliseconds>(system_clock::now());
  auto current_time = tp.time_since_epoch().count();

  return RestrictedValue((uint64_t)current_time);
}

void RegisterDateAPI(Context* ctx) {
  static BuiltinFunctionTable apis(BuiltinFunctionTable::Date,
                                   {
                                       {"now", &Now},
                                   });
  RegisterBuiltinFunctionTable(ctx, "Date", &apis);
}
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_LEPUS_DATE_API_H_
