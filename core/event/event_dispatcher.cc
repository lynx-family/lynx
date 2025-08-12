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

#include "base/include/fml/memory/weak_ptr.h"
#include "base/trace/native/trace_event.h"
#include "core/event/event.h"
#include "core/event/event_target.h"
#include "core/event/touch_event.h"
#include "core/renderer/trace/renderer_trace_event_def.h"

namespace lynx {
namespace event {

DispatchEventResult EventDispatcher::DispatchEvent(EventTarget& target,
                                                   Event& event) {
  EventDispatcher dispatcher(target, event);
  return event.DispatchEvent(dispatcher);
}

EventDispatcher::EventDispatcher(EventTarget& target, Event& event)
    : target_(target.GetWeakTarget()), event_(&event) {
  event_->InitEventPath(*target_);
}

DispatchEventResult EventDispatcher::Dispatch() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, EVENT_DISPATCHER_DISPATCH,
              [this](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_flow_ids(event_->TraceFlowId());
                ctx.event()->add_debug_annotations("name", event_->type());
              });
  LOGI("EventDispatcher::Dispatch name: " << event_->type());
  if (!target_) {
    LOGE("EventDispatcher::Dispatch error: the target is null.");
    return {EventCancelType::kCanceledBeforeDispatch, false};
  }
  // handle conflic and param
  if (event_->HandleEventConflictAndParam()) {
    return {EventCancelType::kCanceledByEventHandler, false};
  }
  event_->set_target(target_->GetWeakTarget());
  event_->HandleEventCustomDetail();
  bool consumed = false;
  auto path = event_->event_path();

  // TODO(hexionghui): trigger global event, eg: trigger-global-event attribute
  // or GlobalEventEmitter
  target_->HandleGlobalEvent(*event_);

  // TODO(hexionghui): global-bind event, eg: global-bindtap

  // no capture and bubble, eg: bindscroll
  if (!event_->bubbles()) {
    event_->set_event_phase(Event::PhaseType::kAtTarget);
    event_->set_current_target(target_->GetWeakTarget());
    auto result = target_->DispatchEvent(*event_);
    return {EventCancelType::kCanceledByDefaultEventHandler, result.consumed};
  }

  // capture, eg: capture-bindtap
  for (auto item = path.rbegin(); item != path.rend(); ++item) {
    fml::WeakPtr<EventTarget> target = *item;
    if (!target) {
      LOGE(
          "EventDispatcher::Dispatch capture error: the target of event path "
          "is null.");
      continue;
    }
    event_->set_event_phase((event_->target() == target)
                                ? Event::PhaseType::kAtTarget
                                : Event::PhaseType::kCapturingPhase);
    event_->set_current_target(*item);
    auto result = (*item)->DispatchEvent(*event_);
    consumed |= result.consumed;
    if (result.IsCanceled()) {
      return result;
    }
  }

  // bubble, eg: bindtap
  for (auto& item : path) {
    if (!item) {
      LOGE(
          "EventDispatcher::Dispatch capture error: the target of event path "
          "is null.");
      continue;
    }
    if (event_->target() == item) {
      // target is handled by capture phase.
      continue;
    }
    event_->set_event_phase(Event::PhaseType::kBubblingPhase);
    event_->set_current_target(item);
    auto result = item->DispatchEvent(*event_);
    consumed |= result.consumed;
    if (result.IsCanceled()) {
      return result;
    }
  }

  return {EventCancelType::kNotCanceled, consumed};
}

}  // namespace event
}  // namespace lynx
