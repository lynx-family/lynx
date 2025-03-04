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

#ifndef CORE_EVENT_EVENT_LISTENER_H_
#define CORE_EVENT_EVENT_LISTENER_H_

#include <string>

namespace lynx {
namespace event {

class Event;

class EventListener {
 public:
  // If need extend a new EventListener subclass, a new enumeration should be
  // added in Type.
  enum class Type {
    kLepusClosureEventListener,
    kJSClosureEventListener,
    kClosureEventListener
  };

  explicit EventListener(Type type);
  virtual ~EventListener() = default;

  // make EventListener move only
  EventListener(const EventListener&) = delete;
  EventListener& operator=(const EventListener&) = delete;
  EventListener(EventListener&&) = default;
  EventListener& operator=(EventListener&&) = default;

  void set_removed(bool value) { removed_ = value; }
  bool removed() { return removed_; }

  Type type() const { return type_; }

  virtual void Invoke(Event* event) = 0;

  virtual bool Matches(EventListener* listener) = 0;

 private:
  bool removed_{false};

  Type type_;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_EVENT_LISTENER_H_
