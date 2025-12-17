/*
 * Copyright (C) 2006, 2008, 2009 Apple Inc. All rights reserved.
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

#include "core/event/event_listener.h"

#include "core/event/event.h"

namespace lynx {
namespace event {

EventListener::EventListener(Type type, const EventListener::Options& options)
    : type_(type), options_(options) {}

bool EventListener::IsMatchEvent(fml::RefPtr<Event> event) const {
  bool is_capture_bubble_event = event->IsCaptureBubbleEvent();
  if (is_capture_bubble_event) {
    bool is_capture_listener =
        event->event_phase() == Event::PhaseType::kCapturingPhase &&
        GetOptions().IsCapture() && !GetOptions().IsGlobal();
    // Align the logic of capturePhase:false of miniapp.
    bool is_good_capture = GetOptions().IsCapture() && event->capture();
    bool is_target_listener =
        event->event_phase() == Event::PhaseType::kAtTarget &&
        (is_good_capture || !GetOptions().IsCapture()) &&
        !GetOptions().IsGlobal();
    bool is_bubble_listener =
        event->event_phase() == Event::PhaseType::kBubblingPhase &&
        !GetOptions().IsCapture() && !GetOptions().IsGlobal();
    bool is_global_listener =
        event->event_phase() == Event::PhaseType::kGlobal &&
        GetOptions().IsGlobal();
    return is_capture_listener || is_target_listener || is_bubble_listener ||
           is_global_listener;
  }
  return true;
}

}  // namespace event
}  // namespace lynx
