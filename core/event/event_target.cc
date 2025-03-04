/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/event_target.h"

#include <utility>

#include "core/event/event.h"
#include "core/event/event_listener.h"

namespace lynx {
namespace event {

EventTarget::EventTarget()
    : event_listener_map_(std::make_unique<EventListenerMap>()) {}

DispatchEventResult EventTarget::DispatchEvent(Event& event) {
  auto vector = event_listener_map_->Find(event.type());
  if (vector == nullptr) {
    return DispatchEventResult::kNotCanceled;
  }

  // Fire all listeners registered for this event. Don't fire listeners removed
  // during event dispatch. Also, don't fire event listeners added during event
  // dispatch. Conveniently, all new event listeners will be added after or at
  // index |size|, so iterating up to (but not including) |size| naturally
  // excludes new event listeners.
  EventListenerVector copy = *vector;
  for (auto& listener : copy) {
    if (listener->removed()) {
      continue;
    }
    listener->Invoke(&event);
  }
  return DispatchEventResult::kNotCanceled;
}

bool EventTarget::AddEventListener(
    const std::string& type, std::shared_ptr<EventListener> listener,
    const EventListenerMap::AddOptions& options) {
  return event_listener_map_->Add(type, std::move(listener), options);
}

bool EventTarget::RemoveEventListener(const std::string& type,
                                      std::shared_ptr<EventListener> listener) {
  return event_listener_map_->Remove(type, std::move(listener));
}

}  // namespace event
}  // namespace lynx
