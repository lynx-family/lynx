/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/common/event/message_event.h"

#include <chrono>

#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/bindings/common/event/runtime_constants.h"

namespace lynx {
namespace runtime {

MessageEvent::MessageEvent(ContextProxy::Type origin, ContextProxy::Type target,
                           const lepus::Value& message)
    : MessageEvent(kMessage, origin, target, message) {}

MessageEvent::MessageEvent(const std::string& type, ContextProxy::Type origin,
                           ContextProxy::Type target,
                           const lepus::Value& message)
    : MessageEvent(type,
                   std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count(),
                   origin, target, message) {}

MessageEvent::MessageEvent(const std::string& type, int64_t time_stamp,
                           ContextProxy::Type origin, ContextProxy::Type target,
                           const lepus::Value& message)
    : event::Event(type, time_stamp, event::Event::EventType::kMessageEvent,
                   Bubbles::kNo, Cancelable::kNo, ComposedMode::kScoped,
                   PhaseType::kAtTarget),
      origin_(origin),
      target_(target),
      message_(message) {}

MessageEvent MessageEvent::ShallowCopy(const MessageEvent& event) {
  return MessageEvent(event.type(), event.time_stamp(), event.origin_,
                      event.target_, lepus::Value::ShallowCopy(event.message_));
}

MessageEvent MessageEvent::ShallowCopy(MessageEvent& event) {
  return MessageEvent(event.type(), event.time_stamp(), event.origin_,
                      event.target_, lepus::Value::ShallowCopy(event.message_));
}

std::string MessageEvent::GetTargetString() const {
  return ContextProxy::ConvertContextTypeToString(GetTargetType());
}

std::string MessageEvent::GetOriginString() const {
  return ContextProxy::ConvertContextTypeToString(GetOriginType());
}

}  // namespace runtime
}  // namespace lynx
