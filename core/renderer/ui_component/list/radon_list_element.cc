// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/radon_list_element.h"

#include "core/renderer/dom/element_manager.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {

RadonListElement::RadonListElement(const base::String& tag,
                                   const std::shared_ptr<AttributeHolder>& node,
                                   ElementManager* element_manager,
                                   uint32_t node_index)
    : RadonElement(tag, node, element_manager, node_index),
      ListContainer(this) {
  UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kListContainer,
                            lepus::Value(true));
  tasm::report::FeatureCounter::Instance()->Count(
      tasm::report::LynxFeature::CPP_ENABLE_NATIVE_LIST);
}

void RadonListElement::TickElement(fml::TimePoint& time) {
  if (list_container_delegate()) {
    list_container_delegate()->OnNextFrame();
  }
}

/**
 * @description: This method should be overridden by the subclass to establish
 *whether the attribute needs updating at the platform. If update is
 *unnecessary, will return false.
 * @param key: attribute's name
 * @param value: attribute's value
 **/
bool RadonListElement::OnAttributeSet(const base::String& key,
                                      const lepus::Value& value) {
  return list_container_delegate()
             ? list_container_delegate()->ResolveAttribute(key, value)
             : true;
}

/**
 * @description: When the list element changes, this method will be invoked. For
 *example, if the list's width or height changes, or if the List itself has new
 *diff information.
 **/
void RadonListElement::OnListElementUpdated(const PipelineOptions& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonListElement::OnListElementUpdated");
  if (list_container_delegate()) {
    if (options.need_timestamps) {
      tasm::TimingCollector::Instance()->Mark(
          tasm::timing::kListRenderChildrenStart);
    }
    list_container_delegate()->OnLayoutChildren();
    if (options.need_timestamps) {
      tasm::TimingCollector::Instance()->Mark(
          tasm::timing::kListRenderChildrenEnd);
    }
  }
}

/**
 * @description: When the rendering of the list's child node is complete, this
 *method will be invoked. In this method, we can accurately obtain the layout
 *information of the child node.
 * @param operation_id: the unique identifier for the current rendering
 *operation.
 * @param component: child
 **/
void RadonListElement::OnComponentFinished(Element* component,
                                           const PipelineOptions& option) {
  if (list_container_delegate() && component && option.operation_id != 0) {
    list_container_delegate()->FinishBindItemHolder(component, option);
  }
}

void RadonListElement::OnListItemLayoutUpdated(Element* component) {
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    list_container_delegate()->OnListItemLayoutUpdated(component);
  }
}

/**
 * @description: Send scroll distance to list element.
 * @param delta_x: scroll distance in horizontal direction.
 * @param delta_y: scroll distance in vertical direction.
 **/
void RadonListElement::ScrollByListContainer(float content_offset_x,
                                             float content_offset_y,
                                             float original_x,
                                             float original_y) {
  if (list_container_delegate()) {
    list_container_delegate()->ScrollByPlatformContainer(
        content_offset_x, content_offset_y, original_x, original_y);
  }
}

/**
 * @description: Implement list's ScrollToPosition ui method.
 * @param index: target position
 * @param offset: scroll offset
 * @param align: alignment type: top / bottom / middle
 * @param smooth: smooth scroll
 **/
void RadonListElement::ScrollToPosition(int index, float offset, int align,
                                        bool smooth) {
  if (list_container_delegate()) {
    list_container_delegate()->ScrollToPosition(index, offset, align, smooth);
  }
}

/**
 * @description: Finish ScrollToPosition
 **/
void RadonListElement::ScrollStopped() {
  if (list_container_delegate()) {
    list_container_delegate()->ScrollStopped();
  }
}

void RadonListElement::SetEventHandler(const base::String& name,
                                       EventHandler* handler) {
  Element::SetEventHandler(name, handler);
  if (list_container_delegate()) {
    list_container_delegate()->AddEvent(name);
  }
}

void RadonListElement::ResetEventHandlers() {
  Element::ResetEventHandlers();
  if (list_container_delegate()) {
    list_container_delegate()->ClearEvents();
  }
}

void RadonListElement::ResolveStyleValue(CSSPropertyID id,
                                         const tasm::CSSValue& value,
                                         bool force_update) {
  RadonElement::ResolveStyleValue(id, value, force_update);
  if (list_container_delegate() &&
      (CSSPropertyID::kPropertyIDListMainAxisGap == id ||
       CSSPropertyID::kPropertyIDListCrossAxisGap == id)) {
    lepus::Value axis_gap_value = computed_css_style()->GetValue(id);
    list_container_delegate()->ResolveListAxisGap(id, axis_gap_value);
  }
}

void RadonListElement::PropsUpdateFinish() {
  if (list_container_delegate()) {
    list_container_delegate()->PropsUpdateFinish();
  }
}

}  // namespace tasm
}  // namespace lynx
