// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_EVENT_EVENT_DISPATCH_RESULT_H_
#define CORE_EVENT_EVENT_DISPATCH_RESULT_H_

#include <stdint.h>

namespace lynx {
namespace event {

enum class DispatchEventResult : uint8_t {
  // Event was not canceled by event handler or default event handler.
  kNotCanceled = 0,
  // Event was canceled by event handler; i.e. a script handler calling
  // preventDefault.
  kCanceledByEventHandler,
  // Event was canceled by the default event handler; i.e. executing the default
  // action.  This result should be used sparingly as it deviates from the DOM
  // Event Dispatch model. Default event handlers really shouldn't be invoked
  // inside of dispatch.
  kCanceledByDefaultEventHandler,
  // Event was canceled but suppressed before dispatched to event handler.  This
  // result should be used sparingly; and its usage likely indicates there is
  // potential for a bug. Trusted events may return this code; but untrusted
  // events likely should always execute the event handler the developer intends
  // to execute.
  kCanceledBeforeDispatch,
};

}  // namespace event
}  // namespace lynx

#endif  // CORE_EVENT_EVENT_DISPATCH_RESULT_H_
