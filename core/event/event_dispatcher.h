/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights
 * reserved.
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

#include "core/event/event_dispatch_result.h"

#ifndef CORE_EVENT_EVENT_DISPATCHER_H_
#define CORE_EVENT_EVENT_DISPATCHER_H_

namespace lynx {
namespace event {

class Event;
class EventDispatchHandlingState;
class EventTarget;

enum EventDispatchContinuation { kContinueDispatching, kDoneDispatching };

class EventDispatcher {
 public:
  static DispatchEventResult DispatchEvent(EventTarget&, Event&);

  DispatchEventResult Dispatch();
  EventTarget& GetTarget() const { return *target_; }
  Event& GetEvent() const { return *event_; }

 private:
  EventDispatcher(EventTarget&, Event&);

  EventTarget* target_;
  Event* event_;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_EVENT_DISPATCHER_H_
