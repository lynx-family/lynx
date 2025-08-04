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

  struct Options {
    static const int kCaptureBit = 1 << 0;
    static const int kOnceBit = 1 << 1;
    static const int kPassiveBit = 1 << 2;
    static const int kSignalBit = 1 << 3;
    // Indicates whether to intercept the event. Compatible with capture-catch
    // and catch.
    static const int kCatchBit = 1 << 4;
    // Indicates whether to global bind the event. Compatible with global-bind.
    static const int kGlobalBit = 1 << 5;

    int64_t flags;

    Options(bool capture = false, bool once = false, bool passive = false,
            bool signal = false, bool is_catch = false, bool is_global = false)
        : flags((capture ? kCaptureBit : 0) | (once ? kOnceBit : 0) |
                (passive ? kPassiveBit : 0) | (signal ? kSignalBit : 0) |
                (is_catch ? kCatchBit : 0) | (is_global ? kGlobalBit : 0)) {}

    bool IsCapture() const { return (flags & kCaptureBit) == kCaptureBit; }

    bool IsCatch() const { return (flags & kCatchBit) == kCatchBit; }

    bool IsGlobal() const { return (flags & kGlobalBit) == kGlobalBit; }
  };

  explicit EventListener(Type type, const Options& options = Options());
  virtual ~EventListener() = default;

  // make EventListener move only
  EventListener(const EventListener&) = delete;
  EventListener& operator=(const EventListener&) = delete;
  EventListener(EventListener&&) = default;
  EventListener& operator=(EventListener&&) = default;

  void set_removed(bool value) { removed_ = value; }
  bool removed() { return removed_; }

  Type type() const { return type_; }

  const Options& GetOptions() const { return options_; }

  virtual void Invoke(Event* event) = 0;

  virtual bool Matches(EventListener* listener) = 0;

 protected:
  bool removed_{false};
  Type type_;
  Options options_;
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_EVENT_LISTENER_H_
