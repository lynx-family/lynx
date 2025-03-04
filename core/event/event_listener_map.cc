/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 *           (C) 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2011 Andreas Kling (kling@webkit.org)
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

#include "core/event/event_listener_map.h"

#include <algorithm>
#include <tuple>

namespace lynx {
namespace event {

void EventListenerMap::Clear() { map_.clear(); }

bool EventListenerMap::IsEmpty() const {
  if (map_.empty()) {
    return true;
  }
  for (const auto& pair : map_) {
    if (!pair.second.empty()) {
      return false;
    }
  }
  return true;
}

bool EventListenerMap::Contains(const std::string& type) const {
  for (const auto& pair : map_) {
    if (pair.first == type) {
      return true;
    }
  }
  return false;
}

bool EventListenerMap::Add(const std::string& type,
                           std::shared_ptr<EventListener> listener,
                           const AddOptions& options) {
  EventListenerVector* vector = Find(type);
  if (vector == nullptr) {
    vector = &map_.emplace_back(std::piecewise_construct,
                                std::forward_as_tuple(type), std::tuple<>())
                  .second;
  }
  vector->emplace_back(std::move(listener));
  return true;
}

bool EventListenerMap::Remove(const std::string& type,
                              std::shared_ptr<EventListener> listener) {
  EventListenerVector* vector = Find(type);
  if (!vector || vector->empty()) {
    return false;
  }
  auto iter = std::remove_if(vector->begin(), vector->end(),
                             [ptr = listener.get()](const auto& item) {
                               if (ptr->Matches(item.get())) {
                                 item->set_removed(true);
                                 return true;
                               }
                               return false;
                             });
  if (iter == vector->end()) {
    return false;
  }
  vector->erase(iter, vector->end());
  return true;
}

EventListenerVector* EventListenerMap::Find(const std::string& type) {
  for (auto& pair : map_) {
    if (pair.first == type) {
      return &(pair.second);
    }
  }
  return nullptr;
}

}  // namespace event
}  // namespace lynx
