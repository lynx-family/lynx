// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/base_element_container.h"

#include <algorithm>
#include <cstddef>
#include <deque>

#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_container.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/prop_bundle_style_writer.h"
#include "core/value_wrapper/value_impl_lepus.h"

namespace lynx {
namespace tasm {

BaseElementContainer::BaseElementContainer(Element* element)
    : element_(element), manager_(element->element_manager()) {
  id_ = element_ ? element_->impl_id() : -1;
}

BaseElementContainer::~BaseElementContainer() {}

Element* BaseElementContainer::element() const { return element_; }
ElementManager* BaseElementContainer::element_manager() const {
  return manager_;
}
PaintingContext* BaseElementContainer::painting_context() const {
  return element_manager()->painting_context();
}

int BaseElementContainer::id() const { return id_; }

bool BaseElementContainer::CheckFlatten(
    base::MoveOnlyClosure<bool, bool> func) {
  return painting_context()->IsFlatten(std::move(func));
}

ElementContainer* BaseElementContainer::CastToElementContainer() {
  return static_cast<ElementContainer*>(this);
}

Fragment* BaseElementContainer::CastToFragment() {
  return static_cast<Fragment*>(this);
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
                                                 const base::String& item_key) {
  painting_context()->ListReusePaintingNode(child_id, item_key);
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

BaseElementContainer* BaseElementContainer::EnclosingStackingContextNode() {
  Element* current = element();
  for (; current != nullptr; current = current->parent()) {
    if (current->IsStackingContextNode()) return current->element_container();
  }
  // Unreachable code
  LOGE("EnclosingStackingContextNode: No stacking context found");
  return this;
}

void BaseElementContainer::MarkDirtyState(DirtyState state) {
  if (dirty_state_ & state) {
    return;
  }

  if (state == kNeedSortZChild) {
    dirty_state_ = static_cast<DirtyState>(dirty_state_ | state);
    set_has_z_child(true);
    element_manager()->InsertDirtyContext(this);
  }

  if (IsRootContainer() && state == kNeedSortFixedChild) {
    dirty_state_ = static_cast<DirtyState>(dirty_state_ | state);
    set_has_fixed_child(true);
    element_manager()->InsertDirtyContext(this);
  }

  if (state == kNeedRedraw || state == kNeedUpdateSubtreeProperty) {
    dirty_state_ = static_cast<DirtyState>(dirty_state_ | state);
  }
}

bool BaseElementContainer::IsRootContainer() const {
  return element()->is_page();
}

void BaseElementContainer::ResetDirtyState(DirtyState state) {
  dirty_state_ = static_cast<DirtyState>(dirty_state_ & ~state);
}

Element const* BaseElementContainer::FindCommonAncestor(
    Element const** left_mark, Element const** right_mark) {
  std::deque<const Element*> left_ancestors;
  std::deque<const Element*> right_ancestors;
  Element const* left = *left_mark;
  Element const* right = *right_mark;
  while (left != nullptr) {
    left_ancestors.emplace_front(left);
    left = left->parent();
  }
  while (right != nullptr) {
    right_ancestors.emplace_front(right);
    right = right->parent();
  }
  auto it_l = left_ancestors.begin();
  auto it_r = right_ancestors.begin();
  while (it_l != left_ancestors.end() && it_r != right_ancestors.end() &&
         *it_l == *it_r) {
    it_l++;
    it_r++;
  }
  if (it_l == left_ancestors.end() || it_r == right_ancestors.end()) {
    return nullptr;
  }
  *left_mark = *it_l;
  *right_mark = *it_r;
  return (*left_mark)->parent();
}

int BaseElementContainer::CompareElementOrder(Element* left, Element* right) {
  if (left == right) {
    return 0;
  }
  // left is right's ancestor
  const Element* temp = right;
  while (temp != nullptr) {
    if (temp->parent() == left) {
      // left is smaller
      return -1;
    }
    temp = temp->parent();
  }
  // right is left's ancestor
  temp = left;
  while (temp != nullptr) {
    if (temp->parent() == right) {
      return 1;
    }
    temp = temp->parent();
  }
  // find the common ancestor
  Element const* left_mark = left;
  Element const* right_mark = right;
  Element const* common_ancestor = FindCommonAncestor(&left_mark, &right_mark);
  // compare the order in the common ancestor
  if (common_ancestor) {
    int i = 0;
    size_t count = const_cast<Element*>(common_ancestor)->GetChildCount();
    Element* child = nullptr;
    while (i < static_cast<int>(count)) {
      child = const_cast<Element*>(common_ancestor)->GetChildAt(i);
      if (child == right_mark) {
        return 1;
      }
      if (child == left_mark) {
        return -1;
      }
      i++;
    }
  }
  return 0;
}

void BaseElementContainer::UpdateGlobalInsertionOrder() {
  global_insertion_order_ = element_manager()->GenerateGlobalInsertionOrder();
}

}  // namespace tasm
}  // namespace lynx
