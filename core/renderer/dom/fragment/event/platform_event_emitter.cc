// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fragment/event/platform_event_emitter.h"

#include "core/event/event.h"
#include "core/renderer/ui_wrapper/painting/native_painting_context_platform_ref.h"

namespace lynx {
namespace tasm {

void PlatformEventEmitter::SendEvent(int32_t target_id,
                                     fml::RefPtr<event::Event> event) {
  platform_ref_->SendEvent(target_id, event);
}

}  // namespace tasm
}  // namespace lynx
