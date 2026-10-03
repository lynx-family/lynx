/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All
 * rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "core/event/event_dispatcher.h"

#include <string>
#include <utility>

#include "base/include/fml/memory/weak_ptr.h"
#include "base/trace/native/trace_event.h"
#include "core/event/event.h"
#include "core/event/event_target.h"
#include "core/event/touch_event.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace event {
namespace {

bool ShouldDispatchFrontendCustomEventBubbleCompatible(
    const fml::RefPtr<Event>& event) {
  return event && event->enable_frontend_custom_event_bubble_compatible() &&
         event->from_frontend() &&
         event->event_type() == Event::EventType::kCustomEvent &&
         !event->bubbles();
}

bool HasBubbleEventListener(EventTarget& target, Event& event) {
  auto* listeners = target.GetEventListenerMap()->Find(event.type());
  if (listeners == nullptr) {
    return false;
  }
  for (const auto& listener : *listeners) {
    if (!listener || listener->removed()) {
      continue;
    }
    const auto& options = listener->GetOptions();
    if (!options.IsCapture() && !options.IsGlobal()) {
      return true;
    }
  }
  return false;
}

DispatchEventResult TraceDispatchResult(const fml::RefPtr<Event>& event,
                                        DispatchEventResult result,
                                        const char* stage) {
#if ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE
  TRACE_EVENT_INSTANT(
      LYNX_TRACE_CATEGORY, EVENT_DISPATCHER_DISPATCH_RESULT,
      [event, result, stage](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("name", event->type());
        ctx.event()->add_debug_annotations("stage", stage);
        ctx.event()->add_debug_annotations(
            "cancel_type",
            std::to_string(static_cast<uint32_t>(result.cancel_type)));
        ctx.event()->add_debug_annotations("consumed",
                                           result.consumed ? "true" : "false");
        ctx.event()->add_debug_annotations(
            "stop_propagation",
            event->is_stop_propagation() ? "true" : "false");
        ctx.event()->add_debug_annotations(
            "stop_immediate_propagation",
            event->is_stop_immediate_propagation() ? "true" : "false");
        ctx.event()->add_debug_annotations(
            "phase",
            std::to_string(static_cast<uint32_t>(event->event_phase())));
        ctx.event()->add_terminating_flow_ids(event->TraceFlowId());
      });
#else
  (void)event;
  (void)stage;
#endif
  return result;
}

}  // namespace

DispatchEventResult EventDispatcher::DispatchEvent(EventTarget& target,
                                                   fml::RefPtr<Event> event) {
  EventDispatcher dispatcher(target, event);
  return event->DispatchEvent(dispatcher);
}

EventDispatcher::EventDispatcher(EventTarget& target, fml::RefPtr<Event> event)
    : target_(target.GetWeakTarget()), event_(std::move(event)) {
  event_->InitEventPath(*target_);
}

DispatchEventResult EventDispatcher::Dispatch() {
  TRACE_EVENT(
      LYNX_TRACE_CATEGORY, EVENT_DISPATCHER_DISPATCH,
      [this, target = target_](lynx::perfetto::EventContext ctx) {
        ctx.event()->add_debug_annotations("name", event_->type());
        ctx.event()->add_debug_annotations(
            "event_type",
            std::to_string(static_cast<uint32_t>(event_->event_type())));
        ctx.event()->add_debug_annotations(
            "timestamp", std::to_string(event_->time_stamp()));
        ctx.event()->add_debug_annotations(
            "capture", event_->capture() ? "true" : "false");
        ctx.event()->add_debug_annotations(
            "bubbles", event_->bubbles() ? "true" : "false");
        ctx.event()->add_debug_annotations(
            "cancelable", event_->cancelable() ? "true" : "false");
        ctx.event()->add_debug_annotations(
            "composed", event_->composed() ? "true" : "false");
        ctx.event()->add_debug_annotations(
            "from_frontend", event_->from_frontend() ? "true" : "false");
        ctx.event()->add_debug_annotations(
            "path_size", std::to_string(event_->event_path().size()));
        ctx.event()->add_debug_annotations(
            "target",
            target ? target->GetUniqueID() : std::string("unavailable"));
        ctx.event()->add_flow_ids(event_->TraceFlowId());
      });
  if (!target_) {
    LOGE("EventDispatcher::Dispatch error: the target is null.")
    return TraceDispatchResult(
        event_, {EventCancelType::kCanceledBeforeDispatch, false},
        "target_unavailable");
  }
  LOGI("EventDispatcher::Dispatch name: " << event_->type() << " target: "
                                          << target_->GetUniqueID())
  // handle conflic and param
  if (event_->HandleEventConflictAndParam()) {
    return TraceDispatchResult(
        event_, {EventCancelType::kCanceledByEventHandler, false}, "conflict");
  }
  event_->set_target(target_->GetWeakTarget());
  event_->HandleEventCustomDetail();
  bool consumed = false;
  auto path = event_->event_path();

  // trigger global event, eg: trigger-global-event attribute or global-bind
  // event
  target_->HandleGlobalEvent(event_);

  // capture, eg: capture-bindtap
  if (event_->capture()) {
    for (auto item = path.rbegin(); item != path.rend(); ++item) {
      fml::WeakPtr<EventTarget> target = *item;
      if (!target) {
        LOGE(
            "EventDispatcher::Dispatch capture error: the target of event path "
            "is null.")
        continue;
      }
      if (event_->target() == target) {
        // target is handled by target phase.
        continue;
      }
      event_->set_event_phase(Event::PhaseType::kCapturingPhase);
      event_->set_current_target(*item);
      auto result = (*item)->DispatchEvent(event_);
      consumed |= result.consumed;
      if (result.IsCanceled()) {
        return TraceDispatchResult(event_, result, "capture");
      }
    }
  }

  // at target
  bool should_dispatch_frontend_custom_event_bubble_compatible =
      ShouldDispatchFrontendCustomEventBubbleCompatible(event_) &&
      !HasBubbleEventListener(*target_, *event_);
  {
    event_->set_event_phase(Event::PhaseType::kAtTarget);
    event_->set_current_target(target_->GetWeakTarget());
    auto result = target_->DispatchEvent(event_);
    consumed |= result.consumed;
    if (result.IsCanceled()) {
      return TraceDispatchResult(event_, result, "target");
    }
  }

  if (should_dispatch_frontend_custom_event_bubble_compatible) {
    for (auto& item : path) {
      if (!item) {
        LOGE(
            "EventDispatcher::Dispatch frontend custom event bubble compat "
            "error: the target of event path is null.")
        continue;
      }
      if (event_->target() == item) {
        continue;
      }
      event_->set_event_phase(Event::PhaseType::kBubblingPhase);
      event_->set_current_target(item);
      auto result = item->DispatchEvent(event_);
      consumed |= result.consumed;
      if (result.IsCanceled()) {
        return TraceDispatchResult(event_, result,
                                   "frontend_bubble_compatible");
      }
      if (result.consumed) {
        break;
      }
    }
  }

  // bubble, eg: bindtap
  if (event_->bubbles()) {
    for (auto& item : path) {
      if (!item) {
        LOGE(
            "EventDispatcher::Dispatch bubble error: the target of event path "
            "is null.")
        continue;
      }
      if (event_->target() == item) {
        // target is handled by target phase.
        continue;
      }
      event_->set_event_phase(Event::PhaseType::kBubblingPhase);
      event_->set_current_target(item);
      auto result = item->DispatchEvent(event_);
      consumed |= result.consumed;
      if (result.IsCanceled()) {
        return TraceDispatchResult(event_, result, "bubble");
      }
    }
  }

  return TraceDispatchResult(event_, {EventCancelType::kNotCanceled, consumed},
                             "complete");
}

}  // namespace event
}  // namespace lynx
