// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_EMITTER_H_
#define CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_EMITTER_H_

#include <base/include/fml/memory/ref_ptr.h>

namespace lynx {

namespace event {
class Event;
}

namespace tasm {

class NativePaintingCtxPlatformRef;

class PlatformEventEmitter {
 public:
  explicit PlatformEventEmitter(NativePaintingCtxPlatformRef* platform_ref)
      : platform_ref_(platform_ref) {}

  void SendEvent(int32_t target_id, fml::RefPtr<event::Event> event);

 private:
  // owned by NativePaintingCtxPlatformRef
  NativePaintingCtxPlatformRef* platform_ref_{nullptr};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FRAGMENT_EVENT_PLATFORM_EVENT_EMITTER_H_
