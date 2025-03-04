// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_DARWIN_EVENT_CONVERTER_DARWIN_H_
#define CORE_RENDERER_UTILS_DARWIN_EVENT_CONVERTER_DARWIN_H_

#include <Foundation/Foundation.h>

#include "core/runtime/bindings/common/event/message_event.h"

namespace lynx {
namespace tasm {
namespace darwin {
class EventConverterDarwin {
 public:
  // Convert MessageEvent to NSDictionary
  static NSDictionary* ConverMessageEventToNSDictionary(const runtime::MessageEvent& event);
  // Convert NSDictionary to MessageEvent
  static runtime::MessageEvent ConvertNSDictionaryToMessageEvent(NSDictionary* dict);
};

}  // namespace darwin
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_DARWIN_EVENT_CONVERTER_DARWIN_H_
