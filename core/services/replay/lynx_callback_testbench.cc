// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/services/replay/lynx_callback_testbench.h"

#include "core/services/replay/lynx_replay_helper.h"

namespace lynx {
namespace piper {

ModuleCallbackTestBench::ModuleCallbackTestBench(int64_t callback_id)
    : ModuleCallback(callback_id) {}

void ModuleCallbackTestBench::Invoke(Runtime *runtime,
                                     ModuleCallbackFunctionHolder *holder) {
  if (runtime == nullptr) {
    LOGE("lynx ModuleCallback has null runtime or null function");
    return;
  }
  piper::Runtime *rt = runtime;
  piper::Value args =
      ReplayHelper::convertRapidJsonObjectToJSIValue(*rt, argument);
  holder->function_.call(*rt, args);
}
}  // namespace piper
}  // namespace lynx
