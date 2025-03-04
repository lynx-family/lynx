// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/agent/inspector_common_observer_impl.h"

#include "devtool/lynx_devtool/agent/inspector_util.h"
#include "devtool/lynx_devtool/agent/lynx_devtool_mediator.h"
#include "devtool/lynx_devtool/agent/lynx_global_devtool_mediator.h"

namespace lynx {
namespace devtool {

void InspectorCommonObserverImpl::EndReplayTest(const std::string& file_path) {
  auto sender = sender_.lock();
  LynxGlobalDevToolMediator::GetInstance().EndReplayTest(sender, file_path);
}

void InspectorCommonObserverImpl::SendLayoutTree() {
  auto devtool_mediator = mediator_wp_.lock();
  CHECK_NULL_AND_LOG_RETURN(devtool_mediator, "devtool_mediator is null");
  devtool_mediator->SendLayoutTree();
}

}  // namespace devtool
}  // namespace lynx
