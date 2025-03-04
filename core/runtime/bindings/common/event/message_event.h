/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights
 * reserved.
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

#ifndef CORE_RUNTIME_BINDINGS_COMMON_EVENT_MESSAGE_EVENT_H_
#define CORE_RUNTIME_BINDINGS_COMMON_EVENT_MESSAGE_EVENT_H_

#include <memory>
#include <string>

#include "core/event/event.h"
#include "core/runtime/bindings/common/event/context_proxy.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace runtime {

class MessageEvent : public event::Event {
 public:
  MessageEvent(ContextProxy::Type origin, ContextProxy::Type target,
               const lepus::Value& message);
  MessageEvent(const std::string& type, ContextProxy::Type origin,
               ContextProxy::Type target, const lepus::Value& message);
  MessageEvent(const std::string& type, int64_t time_stamp,
               ContextProxy::Type origin, ContextProxy::Type target,
               const lepus::Value& message);
  virtual ~MessageEvent() override = default;

  static MessageEvent ShallowCopy(const MessageEvent&);
  static MessageEvent ShallowCopy(MessageEvent&);

  // make MessageEvent move only
  MessageEvent(const MessageEvent&) = delete;
  MessageEvent& operator=(const MessageEvent&) = delete;
  MessageEvent(MessageEvent&&) = default;
  MessageEvent& operator=(MessageEvent&&) = default;

  ContextProxy::Type GetTargetType() const { return target_; }
  ContextProxy::Type GetOriginType() const { return origin_; }
  std::string GetTargetString() const;
  std::string GetOriginString() const;

  lepus::Value message() const { return message_; }

  bool IsSendingToUIThread() {
    return target_ == ContextProxy::Type::kUIContext ||
           target_ == ContextProxy::Type::kDevTool;
  }

  bool IsSendingToDevTool() { return target_ == ContextProxy::Type::kDevTool; }

  bool IsSendingToJSThread() {
    return target_ == ContextProxy::Type::kJSContext;
  }

  bool IsSendingToCoreThread() {
    return target_ == ContextProxy::Type::kCoreContext;
  }

 private:
  ContextProxy::Type origin_;
  ContextProxy::Type target_;
  lepus::Value message_;
};

}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_COMMON_EVENT_MESSAGE_EVENT_H_
