/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights
 * reserved.
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
 *
 */

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_EVENT_EVENT_H_
#define CORE_EVENT_EVENT_H_

#include <string>
#include <vector>

#include "core/event/event_dispatch_result.h"

namespace lynx {
namespace event {

class EventTarget;
class EventDispatcher;

class Event {
 public:
  enum class Bubbles {
    kYes,
    kNo,
  };

  enum class Cancelable {
    kYes,
    kNo,
  };

  enum class PhaseType {
    kNone = 0,
    kCapturingPhase = 1,
    kAtTarget = 2,
    kBubblingPhase = 3
  };

  enum class ComposedMode {
    kComposed,
    kScoped,
  };

  // If need extend a new Event subclass, a new enumeration should be added in
  // EventType.
  enum class EventType {
    kTouchEvent,
    kKeyboardEvent,
    kWheelEvent,
    kPointerEvent,
    kUIEvent,
    kMouseEvent,
    kMessageEvent,
    kCustomEvent,
  };

  Event(const std::string&, int64_t, EventType, Bubbles, Cancelable,
        ComposedMode, PhaseType);

  Event(const std::string&, EventType, Bubbles, Cancelable, ComposedMode,
        PhaseType);

  Event(const std::string&, EventType, Bubbles, Cancelable, ComposedMode);

  virtual ~Event() = default;

  const std::string& type() const { return type_; }
  EventType event_type() { return event_type_; }

  PhaseType event_phase() const { return event_phase_; }
  void set_event_phase(PhaseType event_phase) { event_phase_ = event_phase; }

  bool bubbles() const { return bubbles_; }
  bool cancelable() const { return cancelable_; }
  bool composed() const { return composed_; }

  int64_t time_stamp() const { return time_stamp_; };

  void set_target(EventTarget* target) { target_ = target; }
  void set_current_target(EventTarget* current_target) {
    current_target_ = current_target;
  }

  EventTarget* target() const { return target_; }
  EventTarget* current_target() const { return current_target_; }
  const std::vector<EventTarget*>& event_path() const;

  void InitEventPath(EventTarget& target);
  virtual DispatchEventResult DispatchEvent(EventDispatcher&);

 private:
  int64_t time_stamp_;
  std::string type_;

  EventType event_type_;

  bool bubbles_ : 1;
  bool cancelable_ : 1;
  bool composed_ : 1;

  PhaseType event_phase_;

  EventTarget* current_target_;
  EventTarget* target_;

  std::vector<EventTarget*> event_path_;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_EVENT_H_
