// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_BASE_EVENT_CONVERTER_H_
#define CORE_RENDERER_UTILS_BASE_EVENT_CONVERTER_H_

#include "core/public/devtool/vm_event.h"
#include "core/runtime/common/bindings/event/message_event.h"

namespace lynx {
namespace tasm {

// Converts DevTool VM events. The data field is an opaque string.
class EventConverter {
 public:
  static devtool::VMEvent ConvertMessageEventToVMEvent(
      const runtime::MessageEvent& event);
  static fml::RefPtr<runtime::MessageEvent> ConvertVMEventToMessageEvent(
      const devtool::VMEvent& event);
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_BASE_EVENT_CONVERTER_H_
