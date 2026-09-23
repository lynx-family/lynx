// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/recorder/record_payload.h"

#include <atomic>

#include "core/renderer/utils/devtool_lifecycle.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx::tasm::recorder {
namespace {
bool payload_enabled = false;
std::atomic<RecordPayloadCallback> send_payload{nullptr};
}  // namespace

void SetObservationPayloadEnabled(bool enabled) { payload_enabled = enabled; }

void SetRecordPayloadCallback(RecordPayloadCallback send) {
  send_payload.store(send, std::memory_order_relaxed);
}

RecordPayloadCallback GetRecordPayloadCallback() {
  if (!payload_enabled || !LynxEnv::GetInstance().IsDevToolEnabled() ||
      !DevToolLifecycle::GetInstance().IsConnected()) {
    return nullptr;
  }
  return send_payload.load(std::memory_order_relaxed);
}
}  // namespace lynx::tasm::recorder
