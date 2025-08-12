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

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/event/event_dispatch_result.h"

#define EVENT_TYPE_CAPTURE "captureEvent"
#define EVENT_TYPE_CAPTURE_CATCH "capture-catch"
#define EVENT_TYPE_CATCH "catchEvent"
#define EVENT_TYPE_GLOBAL "global-bindEvent"

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
    kBubblingPhase = 3,
    kGlobal = 4,
  };

  // If need extend a new bind type for Event, a new enumeration should be added
  // in BindType
  enum class BindType {
    kNone,
    kBubble,
    kCapture,
    kCaptureCatch,
    kBubbleCatch,
    kGlobalBind,
  };

  enum class ComposedMode {
    kComposed,
    kScoped,
  };

  // If need extend a new Event subclass, a new enumeration should be added in
  // EventType.
  enum class EventType {
    kNone,
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

  EventType event_type() { return event_type_; }
  void set_event_type(EventType event_type) { event_type_ = event_type; }
  int64_t time_stamp() const { return time_stamp_; };
  const std::string& type() const { return type_; }
  bool bubbles() const { return bubbles_; }
  bool cancelable() const { return cancelable_; }
  bool composed() const { return composed_; }

  PhaseType event_phase() const { return event_phase_; }
  void set_event_phase(PhaseType event_phase) { event_phase_ = event_phase; }

  bool is_stop_propagation() const { return is_stop_propagation_; }
  void set_is_stop_propagation(bool is_stop_propagation) {
    is_stop_propagation_ = is_stop_propagation;
  }

  bool is_stop_immediate_propagation() const {
    return is_stop_immediate_propagation_;
  }
  void set_is_stop_immediate_propagation(bool is_stop_immediate_propagation) {
    is_stop_immediate_propagation_ = is_stop_immediate_propagation;
  }

  fml::WeakPtr<EventTarget> target() const { return target_; }
  void set_target(fml::WeakPtr<EventTarget> target) { target_ = target; }

  fml::WeakPtr<EventTarget> current_target() const { return current_target_; }
  void set_current_target(fml::WeakPtr<EventTarget> current_target) {
    current_target_ = current_target;
  }

  lepus::Value& detail() { return detail_; }
  void set_detail(const lepus::Value detail) { detail_ = detail; }

  const std::vector<fml::WeakPtr<EventTarget>>& event_path() const;

  void SetTraceFlowId(uint64_t trace_flow_id) {
    trace_flow_id_ = trace_flow_id;
  }
  uint64_t TraceFlowId() const { return trace_flow_id_; }

  void InitEventPath(EventTarget& target);
  virtual DispatchEventResult DispatchEvent(EventDispatcher&);

  // Called before triggering (invoke the listener) an event to get the base
  // part of detail_.
  virtual void HandleEventBaseDetail(bool is_core_event = false);
  // Called before dispatching an event to get the custom part of detail_.
  virtual void HandleEventCustomDetail();
  // Called before dispatching an event to handle the conflic and param.
  virtual bool HandleEventConflictAndParam() { return false; }

 protected:
  EventType event_type_{EventType::kNone};
  int64_t time_stamp_;
  std::string type_;

  bool bubbles_ : 1;
  bool cancelable_ : 1;
  bool composed_ : 1;

  PhaseType event_phase_{PhaseType::kNone};

  bool is_stop_propagation_{false};
  bool is_stop_immediate_propagation_{false};

  fml::WeakPtr<EventTarget> current_target_;
  fml::WeakPtr<EventTarget> target_;

  // Save the event parameters passed to the listener's closure.
  lepus::Value detail_{lepus::Dictionary::Create()};

  std::vector<fml::WeakPtr<EventTarget>> event_path_;

  uint64_t trace_flow_id_{TRACE_FLOW_ID()};
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_EVENT_H_
