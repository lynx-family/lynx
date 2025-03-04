// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_DATE_API_H_
#define CORE_RUNTIME_VM_LEPUS_DATE_API_H_

#include <time.h>

#include <chrono>
#include <utility>

namespace lynx {
namespace lepus {
Value Now(VMContext* context) {
  using std::chrono::milliseconds;
  using std::chrono::system_clock;
  using std::chrono::time_point;
  using std::chrono::time_point_cast;
  using time_stamp = time_point<system_clock, milliseconds>;
  time_stamp tp = time_point_cast<milliseconds>(system_clock::now());
  auto current_time = tp.time_since_epoch().count();

  return Value((uint64_t)current_time);
}

void RegisterDateAPI(Context* ctx) {
  fml::RefPtr<Dictionary> table = Dictionary::Create();
  RegisterTableFunction(ctx, table, "now", &Now);
  RegisterBuiltinFunctionTable(ctx, "Date", std::move(table));
}
}  // namespace lepus
}  // namespace lynx
#endif  // CORE_RUNTIME_VM_LEPUS_DATE_API_H_
