// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/utils/base/event_converter.h"

#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

devtool::VMEvent EventConverter::ConvertMessageEventToVMEvent(
    const runtime::MessageEvent& event) {
  const auto* message = event.message();
  return {event.type(), event.GetOriginString(), event.GetTargetString(),
          message ? message->str() : "", event.time_stamp()};
}

fml::RefPtr<runtime::MessageEvent> EventConverter::ConvertVMEventToMessageEvent(
    const devtool::VMEvent& event) {
  auto factory = pub::PubValueFactoryDefault();
  // Events entering the VM get a fresh timestamp, as on the other platforms.
  return fml::MakeRefCounted<runtime::MessageEvent>(
      event.type,
      runtime::ContextProxy::ConvertStringToContextType(event.origin),
      runtime::ContextProxy::ConvertStringToContextType(event.target),
      factory.CreateString(event.data));
}

}  // namespace tasm
}  // namespace lynx
