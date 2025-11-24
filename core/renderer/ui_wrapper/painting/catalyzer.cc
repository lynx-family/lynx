// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/catalyzer.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/starlight/layout/layout_object.h"
#include "core/renderer/ui_wrapper/painting/painting_context.h"

namespace lynx {
namespace tasm {

class NodeIndexPair {
 public:
  Element* node;
  int index;
  NodeIndexPair(Element* node, int index) {
    this->node = node;
    this->index = index;
  }
};

Catalyzer::Catalyzer(std::unique_ptr<PaintingContext> painting_context,
                     int32_t instance_id)
    : painting_context_(std::move(painting_context)),
      instance_id_(instance_id) {}

bool Catalyzer::NeedUpdateLayout() { return root_ && root_->need_update(); }

void Catalyzer::UpdateLayoutRecursively() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LAYOUT_CONTEXT_UPDATE_LAYOUT_RECURSIVE);
  if (root_) {
    root_->element_container()->UpdateLayout(root_->left(), root_->top());
  }
}

void Catalyzer::UpdateLayoutRecursivelyWithoutChange() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CATALYZER_TRIGGER_NODE_READY);
  if (root_ && root_->element_container()) {
    root_->element_container()->UpdateLayoutWithoutChange();
  }
}

std::vector<float> Catalyzer::getBoundingClientOrigin(Element* node) {
  return painting_context_->getBoundingClientOrigin(node->impl_id());
}

std::vector<float> Catalyzer::getWindowSize(Element* node) {
  return painting_context_->getWindowSize(node->impl_id());
}

std::vector<float> Catalyzer::GetRectToWindow(Element* node) {
  return painting_context_->GetRectToWindow(node->impl_id());
}

std::vector<float> Catalyzer::GetRectToLynxView(Element* node) {
  return painting_context_->GetRectToLynxView(node->impl_id());
}

std::vector<float> Catalyzer::ScrollBy(int64_t id, float width, float height) {
  return painting_context_->ScrollBy(id, width, height);
}

// 1 - active, 2 - fail, 3 - end
void Catalyzer::SetGestureDetectorState(int64_t id, int32_t gesture_id,
                                        int32_t state) {
  painting_context_->SetGestureDetectorState(id, gesture_id, state);
}

void Catalyzer::ConsumeGesture(int64_t id, int32_t gesture_id,
                               const pub::Value& params) {
  painting_context_->ConsumeGesture(id, gesture_id, params);
}

void Catalyzer::Invoke(
    int64_t id, const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  return painting_context_->Invoke(id, method, params, callback);
}

}  // namespace tasm
}  // namespace lynx
