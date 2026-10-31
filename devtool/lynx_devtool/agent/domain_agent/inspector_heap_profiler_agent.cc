// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/domain_agent/inspector_heap_profiler_agent.h"

namespace lynx {
namespace devtool {

InspectorHeapProfilerAgent::InspectorHeapProfilerAgent(
    const std::shared_ptr<LynxDevToolMediator>& devtool_mediator)
    : devtool_mediator_(devtool_mediator) {}

void InspectorHeapProfilerAgent::CallMethod(
    const std::shared_ptr<CDPResponder>& responder,
    const Json::Value& message) {
  (void)responder->RetrieveSender();
  devtool_mediator_->DispatchJSMessage(message);
}

}  // namespace devtool
}  // namespace lynx
