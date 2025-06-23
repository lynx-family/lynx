// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/text/text_event_target.h"

namespace lynx {
namespace tasm {
namespace harmony {
void TextEventTarget::AddChild(const std::shared_ptr<TextEventTarget>& child) {
  child->parent_ = this;
  children_.push_back(child);
}

EventTarget* TextEventTarget::ParentTarget() { return parent_; }

void TextEventTarget::SetParent(EventTarget* parent) { parent_ = parent; }

bool TextEventTarget::BlockNativeEvent() { return false; }

bool TextEventTarget::EventThrough() {
  if (event_through_ == LynxEventPropStatus::kEnable) {
    return true;
  } else if (event_through_ == LynxEventPropStatus::kDisable) {
    return false;
  }

  EventTarget* parent = this->ParentTarget();
  while (parent) {
    if (!parent->ParentTarget()) {
      return false;
    }
    return parent->EventThrough();
  }
  return false;
}

bool TextEventTarget::IgnoreFocus() {
  if (ignore_focus_ == LynxEventPropStatus::kEnable) {
    return true;
  } else if (ignore_focus_ == LynxEventPropStatus::kDisable) {
    return false;
  }

  EventTarget* parent = this->ParentTarget();
  while (parent) {
    if (!parent->ParentTarget()) {
      return false;
    }
    return parent->IgnoreFocus();
  }
  return false;
}

ConsumeSlideDirection TextEventTarget::ConsumeSlideEvent() {
  return ConsumeSlideDirection::kNone;
}

EventTarget* TextEventTarget::HitTest(float point[2]) {
  if (ContainsPoint(point)) {
    EventTarget* ret;
    for (auto event_target : children_) {
      ret = event_target->HitTest(point);
      if (ret) {
        return ret;
      }
    }
    return this;
  }
  return nullptr;
}

bool TextEventTarget::ShouldHitTest() { return true; }

bool TextEventTarget::ContainsPoint(float point[2]) {
  for (auto rect : text_rects_) {
    if (point[0] >= rect.left && point[0] <= rect.right &&
        point[1] >= rect.top && point[1] <= rect.bottom) {
      return true;
    }
  }

  return false;
}

bool TextEventTarget::IsOnResponseChain() { return false; }

void TextEventTarget::AddRect(float left, float top, float right,
                              float bottom) {
  text_rects_.emplace_back(left, top, right, bottom);
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
