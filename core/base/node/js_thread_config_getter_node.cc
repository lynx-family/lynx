// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/fml/platform/node/message_loop_node.h"
#include "base/include/log/logging.h"
#include "core/base/threading/js_thread_config_getter.h"

namespace lynx {
namespace base {

namespace {
void SetupNodeRuntime() {
  //   TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, SETUP_ARK_TS_RUNTIME);
  LOGI("Start setup node runtime.");

  auto message_loop = fml::MakeRefCounted<fml::MessageLoopNode>();

  fml::MessageLoop::EnsureInitializedForCurrentThread(message_loop);

  LOGI("Start setup node runtime done.");
}
}  // namespace

fml::Thread::ThreadConfig GetJSThreadConfig(const std::string& worker_name) {
  return fml::Thread::ThreadConfig{
      worker_name, fml::Thread::ThreadPriority::HIGH,
      std::make_shared<base::closure>(
          static_cast<void (*)()>(SetupNodeRuntime))};
}

}  // namespace base
}  // namespace lynx
