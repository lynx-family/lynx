/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2005, 2006, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/event/event.h"

#include <chrono>
#include <utility>

#include "base/include/fml/memory/weak_ptr.h"
#include "core/event/event_dispatcher.h"
#include "core/event/event_target.h"

namespace lynx {
namespace event {

Event::Event(const std::string& type, int64_t time_stamp, EventType event_type,
             Bubbles bubbles, Cancelable cancelable, ComposedMode composed_mode,
             PhaseType phase_type)
    : event_type_(event_type),
      time_stamp_(time_stamp),
      type_(type),
      bubbles_(bubbles == Bubbles::kYes),
      cancelable_(cancelable == Cancelable::kYes),
      composed_(composed_mode == ComposedMode::kScoped),
      event_phase_(phase_type) {
  BASE_STATIC_STRING_DECL(kType, "type");
  BASE_STATIC_STRING_DECL(kTimestamp, "timestamp");
  detail_.Table()->SetValue(kType, type_);
  detail_.Table()->SetValue(kTimestamp, time_stamp_);
}

Event::Event(const std::string& type, EventType event_type, Bubbles bubbles,
             Cancelable cancelable, ComposedMode composed_mode,
             PhaseType phase_type)
    : Event(type,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count(),
            event_type, bubbles, cancelable, composed_mode, phase_type) {}

Event::Event(const std::string& type, EventType event_type, Bubbles bubbles,
             Cancelable cancelable, ComposedMode composed_mode)
    : Event(type,
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count(),
            event_type, bubbles, cancelable, composed_mode, PhaseType::kNone) {}

void Event::InitEventPath(EventTarget& target) {
  EventTarget* event_target = &target;
  if (!event_target) {
    LOGE("Event::InitEventPath error: the target is null.");
    return;
  }
  while (!event_target->IsEventPathCatch()) {
    event_path_.push_back(event_target->GetWeakTarget());
    event_target = event_target->GetParentTarget();
    if (!event_target) {
      LOGE("Event::InitEventPath error: the target is null.");
      return;
    }
  };
}

const std::vector<fml::WeakPtr<EventTarget>>& Event::event_path() const {
  return event_path_;
}

DispatchEventResult Event::DispatchEvent(EventDispatcher& dispatcher) {
  return dispatcher.Dispatch();
}

void Event::HandleEventBaseDetail(bool is_core_event) {
  if (!target_ || !current_target_) {
    LOGE(
        "Event::HandleEventBaseDetail error: the target or current_target is "
        "null.");
    return;
  }
  BASE_STATIC_STRING_DECL(kTarget, "target");
  BASE_STATIC_STRING_DECL(kCurrentTarget, "currentTarget");
  detail_.SetProperty(kTarget, target_->GetEventTargetInfo(is_core_event));
  detail_.SetProperty(kCurrentTarget,
                      current_target_->GetEventTargetInfo(is_core_event));
}

void Event::HandleEventCustomDetail() {}

}  // namespace event
}  // namespace lynx
