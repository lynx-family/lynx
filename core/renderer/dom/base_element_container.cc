// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/base_element_container.h"

#include <algorithm>
#include <cstddef>
#include <deque>

#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/prop_bundle_style_writer.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

BaseElementContainer::BaseElementContainer(Element* element)
    : element_(element), manager_(element->element_manager()) {}

BaseElementContainer::~BaseElementContainer() {}

Element* BaseElementContainer::element() const { return element_; }
ElementManager* BaseElementContainer::element_manager() const {
  return manager_;
}
PaintingContext* BaseElementContainer::painting_context() const {
  return element_manager()->painting_context();
}

int BaseElementContainer::id() const { return element()->impl_id(); }

bool BaseElementContainer::CheckFlatten(
    base::MoveOnlyClosure<bool, bool> func) {
  return painting_context()->IsFlatten(std::move(func));
}

void BaseElementContainer::UpdatePlatformExtraBundle(
    PlatformExtraBundle* bundle) {
  painting_context()->UpdatePlatformExtraBundle(element()->impl_id(), bundle);
}

void BaseElementContainer::SetKeyframes(fml::RefPtr<PropBundle> bundle) {
  painting_context()->SetKeyframes(std::move(bundle));
}

void BaseElementContainer::SetFrameAppBundle(
    const std::shared_ptr<LynxTemplateBundle>& bundle) {
  painting_context()->SetFrameAppBundle(element()->impl_id(), bundle);
}

void BaseElementContainer::ListCellWillAppear(const std::string& item_key) {
  painting_context()->ListCellWillAppear(element()->impl_id(), item_key);
}

void BaseElementContainer::ListCellDisappear(bool is_exist,
                                             const base::String& item_key) {
  painting_context()->ListCellDisappear(element()->impl_id(), is_exist,
                                        item_key);
}

void BaseElementContainer::ListReusePaintingNode(int32_t child_id,
                                                 const std::string& item_key) {
  painting_context()->ListReusePaintingNode(child_id, item_key);
}

void BaseElementContainer::InsertListItemPaintingNode(int32_t child_id) {
  painting_context()->InsertListItemPaintingNode(element()->impl_id(),
                                                 child_id);
}

void BaseElementContainer::RemoveListItemPaintingNode(int32_t child_id) {
  painting_context()->RemoveListItemPaintingNode(element()->impl_id(),
                                                 child_id);
}

std::vector<float> BaseElementContainer::ScrollBy(float width, float height) {
  return painting_context()->ScrollBy(element()->impl_id(), width, height);
}

std::vector<float> BaseElementContainer::GetRectToLynxView() {
  return painting_context()->GetRectToLynxView(element()->impl_id());
}

void BaseElementContainer::UpdateScrollInfo(float estimated_offset, bool smooth,
                                            bool scrolling) {
  painting_context()->UpdateScrollInfo(element()->impl_id(), smooth,
                                       estimated_offset, scrolling);
}

void BaseElementContainer::Invoke(
    const std::string& method, const pub::Value& params,
    const std::function<void(int32_t code, const pub::Value& data)>& callback) {
  return painting_context()->Invoke(element()->impl_id(), method, params,
                                    callback);
}

void BaseElementContainer::UpdateContentOffsetForListContainer(
    float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  painting_context()->UpdateContentOffsetForListContainer(
      element()->impl_id(), content_size, delta_x, delta_y,
      is_init_scroll_offset, from_layout);
}

void BaseElementContainer::SetGestureDetectorState(int32_t gesture_id,
                                                   int32_t state) {
  painting_context()->SetGestureDetectorState(element()->impl_id(), gesture_id,
                                              state);
}
void BaseElementContainer::ConsumeGesture(int32_t gesture_id,
                                          const lepus::Value& params) {
  painting_context()->ConsumeGesture(element()->impl_id(), gesture_id,
                                     pub::ValueImplLepus(params));
}

void BaseElementContainer::OnNodeReady() {
  painting_context()->OnNodeReady(element()->impl_id());
}

void BaseElementContainer::OnNodeReload() {
  painting_context()->OnNodeReload(element()->impl_id());
}

void BaseElementContainer::UpdateLayoutPatching() {
  painting_context()->UpdateLayoutPatching();
}

void BaseElementContainer::UpdateNodeReadyPatching() {
  painting_context()->UpdateNodeReadyPatching();
}

void BaseElementContainer::Flush() { painting_context()->Flush(); }

void BaseElementContainer::FlushImmediately() {
  painting_context()->FlushImmediately();
}

void BaseElementContainer::OnFirstScreen() {
  painting_context()->OnFirstScreen();
}

void BaseElementContainer::AppendOptionsForTiming(
    const std::shared_ptr<PipelineOptions>& options) {
  painting_context()->AppendOptionsForTiming(options);
}

void BaseElementContainer::FinishLayoutOperation(
    const std::shared_ptr<PipelineOptions>& options) {
  painting_context()->FinishLayoutOperation(options);
}

void BaseElementContainer::MarkLayoutUIOperationQueueFlushStartIfNeed() {
  painting_context()->MarkLayoutUIOperationQueueFlushStartIfNeed();
}

}  // namespace tasm
}  // namespace lynx
