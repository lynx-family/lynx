// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/services/replay/lynx_callback_testbench.h"

#include "core/services/replay/lynx_replay_helper.h"

namespace lynx {
namespace runtime {
namespace js {
ModuleCallbackTestBench::ModuleCallbackTestBench(int64_t callback_id)
    : ModuleCallback(callback_id) {}

void ModuleCallbackTestBench::Invoke(Runtime *runtime,
                                     ModuleCallbackFunctionHolder *holder) {
  if (guarded && lifetime.expired()) {
    return;
  }
  if (runtime == nullptr) {
    LOGE("lynx ModuleCallback has null runtime or null function");
    return;
  }
  if (guarded) {
    auto value = Value::createFromJsonUtf8(
        *runtime, reinterpret_cast<const uint8_t *>(fixture_json.data()),
        fixture_json.size());
    if (value) holder->function_.call(*runtime, *value);
    return;
  }
  Runtime *rt = runtime;
  Value args = ReplayHelper::convertRapidJsonObjectToJSIValue(*rt, argument);
  holder->function_.call(*rt, args);
}
}  // namespace js
}  // namespace runtime
}  // namespace lynx
