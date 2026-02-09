// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_node.h"

#include <cstddef>
#include <functional>
#include <list>
#include <sstream>
#include <utility>
#include <vector>

#include "base/include/debug/lynx_assert.h"
#include "base/include/value/array.h"
#include "base/include/value/base_string.h"
#include "base/include/value/base_value.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/css/css_decoder.h"
#include "core/renderer/css/css_property.h"
#include "core/renderer/css/css_selector_constants.h"
#include "core/renderer/dom/attribute_holder.h"
#include "core/renderer/dom/element_manager_delegate.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/events/closure_event_listener.h"
#include "core/renderer/events/events.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/pipeline/pipeline_context.h"
#include "core/renderer/pipeline/pipeline_scope.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/js/runtime_constant.h"
#include "core/services/feature_count/feature_counter.h"

#if ENABLE_LEPUSNG_WORKLET
#include "core/renderer/worklet/lepus_element.h"
#include "core/renderer/worklet/lepus_raf_handler.h"
#endif  // ENABLE_LEPUSNG_WORKLET

namespace lynx {
namespace tasm {

RadonNode::RadonNode(PageProxy* page_proxy, const base::String& tag_name,
                     uint32_t node_index)
    : RadonBase(kRadonNode, tag_name, node_index), page_proxy_(page_proxy) {
  attribute_holder_ = fml::MakeRefCounted<AttributeHolder>();
  attribute_holder_->set_radon_node_ptr(this);
  attribute_holder_->set_tag(tag());
  if (!page_proxy_) {
    return;
  }
  // force_calc_new_style_ should be true when using Radon mode.
  force_calc_new_style_ =
      page_proxy_->element_manager()->GetForceCalcNewStyle();
}

RadonNode::RadonNode(const RadonNode& node, PtrLookupMap& map)
    : RadonBase{node, map},
      page_proxy_{node.page_proxy_},
      raw_inline_styles_{node.raw_inline_styles_},
      has_dynamic_class_{node.has_dynamic_class_},
      has_dynamic_inline_style_{node.has_dynamic_inline_style_},
      has_dynamic_attr_{node.has_dynamic_attr_},
      force_calc_new_style_{node.force_calc_new_style_} {
  if (node.attribute_holder_) {
    attribute_holder_ =
        fml::MakeRefCounted<AttributeHolder>(*node.attribute_holder_);
    attribute_holder_->set_radon_node_ptr(this);
    attribute_holder_->set_tag(node.tag());
  }
}

bool RadonNode::CreateElementIfNeeded() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_CREATE_ELEMENT_IF_NEEDED,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (element_ == nullptr) {
    element_ = CreateFiberElement();
    element_->SetNodeIndex(GetOriginalNodeIndex());
    if (page_proxy_->GetPageElementEnabled() && tag().IsEquals("page")) {
      page_proxy_->element_manager()->SetRootOnLayout(element_->impl_id());
      page_proxy_->element_manager()->catalyzer()->set_root(element_.get());
      page_proxy_->element_manager()->SetRoot(element_.get());
    }

    EXEC_EXPR_FOR_INSPECTOR({
      TRACE_EVENT(LYNX_TRACE_CATEGORY, DEVTOOL_CREATE_ELEMENT_IF_NEEDED);
      page_proxy_->element_manager()->PrepareNodeForInspector(element());
      CheckAndProcessComponentRemoveViewForInspector(element());
      CheckAndProcessSlotForInspector(element());
    });
    return true;
  }
  return false;
}

fml::RefPtr<Element> RadonNode::CreateFiberElement() {
  if (tag().IsEqual(kElementPageTag) && radon_component_ &&
      radon_component_->IsRadonPage()) {
    auto fiber_element = page_proxy_->element_manager()->CreateFiberPage(
        radon_component_->ComponentStrId(), radon_component_->GetCSSId());
    fiber_element->SetAttributeHolder(attribute_holder_);
    fiber_element->set_style_sheet_manager(
        radon_component_->style_sheet_manager());
    return fiber_element;
  }
  auto fiber_element =
      page_proxy_->element_manager()->CreateFiberElement(tag());
  fiber_element->SetAttributeHolder(attribute_holder_);
  fiber_element->SetParentComponentUniqueIdForFiber(ParentComponentElementId());
  return fiber_element;
}

void RadonNode::ResetElementRecursively() {
  element_ = nullptr;
  cached_styles_.clear();
  RadonBase::ResetElementRecursively();
}

void RadonNode::RemoveElementFromParent() {
  if (!NeedsElement()) {
    // When the component is 'removeComponentElement', should directly call its
    // children's RemoveElementFromParent.
    RadonBase::RemoveElementFromParent();
    return;
  }

  if (element_.get() != nullptr) {
    EXEC_EXPR_FOR_INSPECTOR(NotifyElementNodeRemoved());
    auto parent_element = element_->parent();
    if (parent_element) {
      parent_element->RemoveNode(element_);
    }
  }
}

void RadonNode::UpdateIdSelector(const base::String& new_id_selector) {
  if (new_id_selector == id_selector()) {
    return;
  }
  SetDynamicAttribute(BASE_STATIC_STRING(AttributeHolder::kIdSelectorAttrName),
                      lepus::Value(new_id_selector));
  // TODO: update css id selector.
  attribute_holder_->SetIdSelector(new_id_selector);
  id_dirty_ = true;
}

void RadonNode::DispatchFirstTime() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_DISPATCH_FIRST_TIME,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto key = BASE_STATIC_STRING(kTransmitClassDirty);
  auto it = attributes().find(key);
  if (it != attributes().end()) {
    need_transmit_class_dirty_ = it->second.Bool();
  }
  auto* fiber_element = static_cast<FiberElement*>(element_.get());
  // Using JSValue across multiple threads
  // is not allowed. Therefore, JSValue needs to be converted to LepusValue
  // before the parallel flush to prevent conversion through LepusContext
  // during the parallel flush phase.
  const auto should_convert_to_lepus_value =
      page_proxy_->element_manager()->GetEnableParallelElement();

  // Id Selector
  if (!id_selector().empty()) {
    fiber_element->SetIdSelector(id_selector());
  }

  // Class
  if (!classes().empty()) {
    fiber_element->SetClasses(attribute_holder_->ReleaseClasses());
  }

  // Attribute
  if (!attributes().empty()) {
    fiber_element->MarkCanBeLayoutOnly(false);
    for (const auto& [key, value] : attributes()) {
      // In first dispatch, should not flush empty attribute in RadonArch.
      if (!value.IsEmpty()) {
        if (should_convert_to_lepus_value) {
          fiber_element->SetAttribute(key, value.ToLepusValue(), false);
        } else {
          fiber_element->SetAttribute(key, value, false);
        }
      }
    }
  }

  // Data set
  if (!data_set().empty()) {
    fiber_element->MarkDirty(FiberElement::kDirtyDataset);
  }

  // Gesture
  if (!gesture_detectors().empty()) {
    fiber_element->MarkDirty(FiberElement::kDirtyGesture);
  }

  // Event
  if (!static_events().empty()) {
    fiber_element->MarkDirty(FiberElement::kDirtyEvent);
  }
  if (!lepus_events().empty()) {
    fiber_element->MarkDirty(FiberElement::kDirtyEvent);
  }
  if (!global_bind_events().empty()) {
    fiber_element->MarkDirty(FiberElement::kDirtyEvent);
  }

  // Raw Inline Styles
  if (!raw_inline_styles().empty()) {
    for (const auto& [key, value] : raw_inline_styles()) {
      if (should_convert_to_lepus_value) {
        fiber_element->SetStyle(key, value.ToLepusValue());
      } else {
        fiber_element->SetStyle(key, value);
      }
    }
    // After setting the raw_inline_style in FiberElement,
    // inline styles will be set in the AttributeHolder for use by DevTool.
    // Therefore, it is necessary to call NotifyElementNodeSetted to notify
    // the Inspector.
    EXEC_EXPR_FOR_INSPECTOR(NotifyElementNodeSetted());
  }
  // tag selector is enabled by default in RadonDiff. Should Mark style dirty
  // by default.
  fiber_element->MarkStyleDirty();
  ProcessEvents();
  class_dirty_ = false;
}

Element* RadonNode::GetParentWithFixed(Element* parent_element) {
  if (!parent_element || !element() || element()->parent()) {
    return nullptr;
  }

  if (element()->is_fixed() && !element()->GetEnableFixedNew()) {
    return page_proxy_->element_manager()->root();
  }
  return parent_element;
}

void RadonNode::InsertElementIntoParent(Element* parent_element) {
  auto parent = GetParentWithFixed(parent_element);
  if (!parent) {
    return;
  }
  auto child_ref = GetElementRef();
  child_ref->UpdateGlobalInsertionOrder();
  if (element()->is_fixed() && !element()->GetEnableFixedNew()) {
    parent->InsertNode(child_ref);
  } else {
    // When FiberElementForRadonDiff is enabled, element is not marked as fixed,
    // thus it is always inserted into its DOM parent.
    auto previous_element = PreviousSiblingElement();
    const auto base_index = parent->IndexOf(previous_element) + 1;
    parent->InsertNode(child_ref, base_index);
  }
}

void RadonNode::OnStyleChange() {
  if (cached_styles_.empty()) {
    return;
  }
  cached_styles_.clear();
}

void RadonNode::DispatchSelf(const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_DISPATCH_SELF,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!NeedsElement() || !option.need_update_element_) {
    return;
  }
  if (CreateElementIfNeeded()) {
    // if element is nullptr, the element will be created.
    DispatchFirstTime();
    InsertElementIntoParent(ParentElement());
    option.has_patched_ = true;
  }
  if (class_transmit_option_.has_value() &&
      !class_transmit_option_->IsEmpty()) {
    const auto& removed = class_transmit_option_->removed_classes();
    option.class_transmit_.RemoveClass(removed.begin(), removed.end());
    for (const auto& iter : class_transmit_option_->added_classes()) {
      option.class_transmit_.AddClass(iter);
    }
    class_transmit_option_.reset();
  }
}

CSSFragment* RadonNode::ParentStyleSheet() const {
  CSSFragment* style_sheet = nullptr;
  if (radon_component_) {
    style_sheet = radon_component_->GetStyleSheet();
  }
  return style_sheet;
}

CSSFragment* RadonNode::GetPageStyleSheet() {
  RadonComponent* page = GetRootEntryNode();
  if (page == nullptr) {
    return nullptr;
  }
  CSSFragment* rootSheet = page->GetStyleSheet();
  return rootSheet;
}

bool RadonNode::GetRemoveCSSScopeEnabled() const {
  if (page_proxy_ == nullptr) {
    return false;
  }
  return page_proxy_->GetRemoveCSSScopeEnabled();
}

bool RadonNode::GetCascadePseudoEnabled() const {
  return page_proxy_->element_manager()->GetEnableCascadePseudo();
}

bool RadonNode::GetRemoveDescendantSelectorScope() const {
  return page_proxy_->element_manager()->GetRemoveDescendantSelectorScope();
}

AttributeHolder* RadonNode::HolderParent() const {
  RadonBase* parent = radon_parent_;
  while (parent != nullptr && !parent->NeedsElement()) {
    parent = parent->Parent();
  }
  // Find a parent needElement or nullptr.
  if (parent) {
    return static_cast<RadonNode*>(parent)->attribute_holder_.get();
  }
  return nullptr;
}

bool RadonNode::InComponent() const {
  return radon_component_->IsRadonComponent();
}

int RadonNode::ParentComponentId() const {
  if (radon_component_) {
    return radon_component_->ComponentId();
  }
  return 0;
}

int32_t RadonNode::ParentComponentElementId() {
  if (!radon_component_) {
    return kInvalidImplId;
  }
  if (radon_component_->element()) {
    return radon_component_->element()->impl_id();
  }
  if (radon_component_->IsRadonPage() && page_proxy_->GetPageElementEnabled()) {
    const auto* element = GetRootElement();
    if (element) {
      return element->impl_id();
    }
  }
  return kInvalidImplId;
}

int RadonNode::ImplId() const {
  return element_ ? element_->impl_id() : kInvalidImplId;
}

void RadonNode::SwapElement(const std::unique_ptr<RadonBase>& old_radon_base,
                            const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_SWAP_ELEMENT,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto* old_radon_node = static_cast<RadonNode*>(old_radon_base.get());
  // re-use old_radon_node's need_transmit_class_dirty_
  need_transmit_class_dirty_ = old_radon_node->need_transmit_class_dirty_;
  has_dynamic_class_ |= old_radon_node->has_dynamic_class_;
  has_dynamic_inline_style_ |= old_radon_node->has_dynamic_inline_style_;
  has_dynamic_attr_ |= old_radon_node->has_dynamic_attr_;
  attribute_holder_->raw_set_pseudo_state(old_radon_node->pseudo_state());
  element_ = old_radon_node->element_;
  if (element_) {
    // Apply dynamic css and inheritance.
    // use new node's AttributeHolder
    element_->SetAttributeHolder(attribute_holder_);
    EXEC_EXPR_FOR_INSPECTOR(UpdateInlineStylesFromOldModel(
        old_radon_node->attribute_holder().get()));
    if (option.refresh_lifecycle_ || option.ssr_hydrating_) {
      fiber_element()->SetParentComponentUniqueIdForFiber(
          ParentComponentElementId());
    }
    EXEC_EXPR_FOR_INSPECTOR({
      // when set RemoveComponentElement and open DevToolDebug and DomTree
      // switch, component node will still has an element for inspect which has
      // no parent and children.For this element, it just need reset
      // AttributeHolder and NotifyElementNodeSetted.
      if (GetDevToolFlag() &&
          element_->inspector_attribute()->needs_erase_id_) {
        NotifyElementNodeSetted();
        return;
      }
    });
    auto previous_fixed = element_->is_fixed();
    // handle node's diff logic in ShouldFlush
    if (ShouldFlush(old_radon_base, option)) {
      EXEC_EXPR_FOR_INSPECTOR(NotifyElementNodeSetted());
      if (!element()->IsFixedNewOrUnifiedEnabled()) {
        // should modify element tree structure if the node's fixed style has
        // been changed
        // In Fixed New Process, don't need to modify element tree structure
        TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_HANDLE_FIXED_ELEMENT);
        if (element_->is_fixed() != previous_fixed) {
          if (element_->is_fixed()) {
            auto* parent = element()->parent();
            EXEC_EXPR_FOR_INSPECTOR(NotifyElementNodeRemoved());
            parent->RemoveNode(GetElementRef(), false);
            GetRootElement()->InsertNode(GetElementRef());
            EXEC_EXPR_FOR_INSPECTOR(NotifyElementNodeAdded());
          } else {
            auto* parent = GetRootElement();
            EXEC_EXPR_FOR_INSPECTOR(NotifyElementNodeRemoved());
            parent->RemoveNode(GetElementRef(), false);
            InsertElementIntoParent(ParentElement());
            EXEC_EXPR_FOR_INSPECTOR(NotifyElementNodeAdded());
          }
        }
      }
      option.has_patched_ = true;
    }

    // if reloading, needs to trigger onNodeReload lifecycle.
    if (option.refresh_lifecycle_ && page_proxy_->GetEnableReloadLifecycle()) {
      element_->onNodeReload();
    }
  }
}

bool RadonNode::ShouldFlush(const std::unique_ptr<RadonBase>& old_radon_base,
                            const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_SHOULD_FLUSH,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  if (!option.need_diff_) {
    return HydrateNode(option);
  }

  auto* old_radon_node = static_cast<RadonNode*>(old_radon_base.get());
  bool updated = false;
  id_dirty_ = !(id_selector() == old_radon_node->id_selector());
  class_dirty_ = false;
  if (has_dynamic_class_) {
    class_dirty_ = classes() != old_radon_node->classes();
  }

  updated |= ShouldFlushAttr(old_radon_node);
  updated |= ShouldFlushDataSet(old_radon_node);
  updated |= ShouldFlushStyle(old_radon_node, option);
  // only enbale new gesture need to check gesture update
  if (page_proxy_ && page_proxy_->GetEnableNewGesture()) {
    updated |= ShouldFlushGestureDetectors(old_radon_node);
    report::FeatureCounter::Instance()->Count(
        report::LynxFeature::CPP_ENABLE_NEW_GESTURE);
  }
  updated |= HydrateNode(option);
  EXEC_EXPR_FOR_INSPECTOR({
    // When the RadonNode's style doesn't change, but its class or id has been
    // changed, we still need to notify devtool to update it.
    if (!updated && (class_dirty_ || id_dirty_)) {
      NotifyElementNodeSetted();
    }
  });
  ProcessEvents();
  id_dirty_ = false;
  class_dirty_ = false;
  style_invalidated_ = true;
  return updated;
}

bool RadonNode::ShouldFlushAttr(const RadonNode* old_radon_node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_SHOULD_FLUSH_ATTR,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  return DiffAttrMapForFiber(old_radon_node->attributes(), attributes());
}

bool RadonNode::ShouldFlushDataSet(const RadonNode* old_radon_node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_SHOULD_FLUSH_DATA_SET,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  // If element_ is null, do not flush dataset.
  if (!element_) {
    return false;
  }
  const static auto& check_flush = [](const DataMap& new_data,
                                      const DataMap& old_data) {
    // When both are empty, do not need flush data set.
    if (old_data.empty() && new_data.empty()) {
      return false;
    }
    if (old_data.size() != new_data.size()) {
      return true;
    }
    // When exec this loop, new_data size == old_data size.
    // If new_data == old_data, each key in new_data can be found in
    // old_data, and the values in new_data & old_data are equal too. In
    // other words, if there is a key not found in old_data or value in
    // new_data not equals with that in old_data, new_data != old_data.
    // Since the above two statements are contrapositive, exec the following
    // loop can check new_data == old_data when new_data size == old_data
    // size.
    for (const auto& new_iter : new_data) {
      auto old_iter = old_data.find(new_iter.first);
      if (old_iter == old_data.end()) {
        return true;
      }
      if (!old_iter->second.IsEqual(new_iter.second)) {
        return true;
      }
    }
    return false;
  };
  const auto& old_data = old_radon_node->data_set();
  const auto& new_data = data_set();
  bool should_flush = check_flush(new_data, old_data);
  if (should_flush) {
    element()->SetDataSet(new_data);
  }
  return should_flush;
}

bool RadonNode::ShouldFlushGestureDetectors(const RadonNode* old_radon_node) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_SHOULD_FLUSH_GESTURE_DETECTORS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  // If element_ is null, do not flush dataset.
  if (!element_) {
    return false;
  }

  // Define a lambda function to check if a flush is needed
  const static auto& check_flush = [](const GestureMap& new_gesture_map,
                                      const GestureMap& old_gesture_map) {
    // When both are empty, no need to flush dataset.
    if (old_gesture_map.empty() && new_gesture_map.empty()) {
      return false;
    }
    if (old_gesture_map.size() != new_gesture_map.size()) {
      return true;
    }

    // Compare individual gesture detectors
    for (const auto& new_iter : new_gesture_map) {
      auto old_iter = old_gesture_map.find(new_iter.first);
      if (old_iter == old_gesture_map.end()) {
        return true;  // New gesture detected, need to flush
      }
    }
    return false;  // No change in gesture detectors
  };

  // Retrieve gesture maps
  const auto& old_gesture_map = old_radon_node->gesture_detectors();
  const auto& new_gesture_map = gesture_detectors();

  // Check if a flush is required
  bool should_flush = check_flush(new_gesture_map, old_gesture_map);

  // If a flush is needed, update gesture detectors
  if (should_flush) {
    for (const auto& gesture : new_gesture_map) {
      element()->SetGestureDetector(gesture.first, gesture.second.get());
    }
  }

  return should_flush;
}

void RadonNode::CollectInvalidationSetsAndInvalidate(
    RadonNode* old_radon_node) {
  if (force_calc_new_style_) {
    // In force calc mode, we don't need invalidation
    return;
  }
  CSSFragment* style_sheet =
      GetRemoveCSSScopeEnabled() ? GetPageStyleSheet() : ParentStyleSheet();
  if (!style_sheet || !style_sheet->enable_css_invalidation()) {
    return;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_SHOULD_COLLECT_INVALID_SETS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  css::InvalidationLists invalidation_lists;
  // Works when CSS Selector is enabled
  if (id_dirty_) {
    CSSFragment::CollectIdChangedInvalidation(
        style_sheet, invalidation_lists, old_radon_node->id_selector().str(),
        id_selector().str());
  }
  if (class_dirty_) {
    CSSFragment::CollectClassChangedInvalidation(
        style_sheet, invalidation_lists, old_radon_node->classes(), classes());
  }

  for (auto* invalidation_set : invalidation_lists.descendants) {
    if (invalidation_set->WholeSubtreeInvalid() ||
        !invalidation_set->IsEmpty()) {
      Visit(false, [invalidation_set, this](RadonBase* child) {
        if (child->IsRadonNode()) {
          auto* node = static_cast<RadonNode*>(child);
          if (!node->style_invalidated_ && !node->tag().IsEqual("raw-text") &&
              invalidation_set->InvalidatesElement(*node->attribute_holder())) {
            node->style_invalidated_ = true;
          }
        }
        return !child->IsRadonComponent() ||
               (child->IsRadonComponent() && GetRemoveCSSScopeEnabled());
      });
    }
  }
}

bool RadonNode::OptimizedShouldFlushStyle(RadonNode* old_radon_node,
                                          const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_OPTIMIZE_SHOULD_FLUSH_STYLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  bool style_updated = false;
  if (option.ShouldForceUpdate() || id_dirty_ || class_dirty_ ||
      has_external_class_ || css_variables_changed_ || style_invalidated_) {
    CollectInvalidationSetsAndInvalidate(old_radon_node);
    const auto& old_style_list = old_radon_node->cached_styles_;
    element()->ResolveStyle(cached_styles_);
    style_updated |= DiffStyleImpl(old_style_list, cached_styles_, true);
  } else if (has_dynamic_inline_style_) {
    // !class_transmit
    // !option.css_variable_changed_
    // !css_variables_changed_
    // !has_external_class_
    // !id_dirty_
    // !class_dirty_
    // no need to use GetCachedStyleList to get new style, diff inline_styles
    // is enough
    cached_styles_ = old_radon_node->cached_styles_;
    // css_variable_map should be reused either.
    attribute_holder_->set_css_variables_map(
        old_radon_node->css_variables_map());
    style_updated |=
        DiffStyleImpl(old_radon_node->inline_styles(), inline_styles(), true);
  } else {
    // !class_transmit
    // !option.css_variable_changed_
    // !css_variables_changed_
    // !has_external_class_
    // !id_dirty_
    // !class_dirty_
    // !has_dynamic_inline_style_
    // static inline style couldn't be changed,
    // just set cached styles.
    cached_styles_ = old_radon_node->cached_styles_;
    // css_variable_map should be reused either.
    attribute_holder_->set_css_variables_map(
        old_radon_node->css_variables_map());
  }
  return style_updated;
}

void RadonNode::MarkChildStyleDirtyRecursively(bool mark_whole_tree) {
  // When mark_whole_tree is false, should stop marking when we meet component.
  if (IsRadonComponent() && !mark_whole_tree) {
    return;
  }
  auto* fiber_ele = fiber_element();
  if (fiber_ele) {
    if (fiber_ele->StyleDirty()) {
      return;
    }
    fiber_ele->MarkStyleDirty(mark_whole_tree);
  }
  for (auto& child : radon_children_) {
    child->MarkChildStyleDirtyRecursively(mark_whole_tree);
  }
}

void RadonNode::ProcessEvents() {
  if (element() && tasm_ && tasm_->EnableEventHandleRefactor()) {
    if (!static_events().empty() || !global_bind_events().empty()) {
      for (const auto& static_event : static_events()) {
        SetEventListeners(static_event);
      }
      for (const auto& global_bind_event : global_bind_events()) {
        SetEventListeners(global_bind_event);
      }
    }
  }
}

void RadonNode::SetEventListeners(
    const std::pair<const base::String, std::unique_ptr<EventHandler>>&
        event_entry) {
  bool is_js_event = event_entry.second->is_js_event();
  const auto& name = event_entry.first.str();
  const auto& type = event_entry.second->type();
  bool is_capture = type == EVENT_TYPE_CAPTURE;
  bool is_capture_catch = type == EVENT_TYPE_CAPTURE_CATCH;
  bool is_bubble_catch = type == EVENT_TYPE_CATCH;
  bool is_global_bind = type == EVENT_TYPE_GLOBAL;
  auto event_options = event::EventListener::Options(
      is_capture || is_capture_catch, false, false, false,
      is_capture_catch || is_bubble_catch, is_global_bind);

  if (is_js_event) {
    // remove the listener firstly to adapt rebind
    element()->RemoveEventListener(
        name, std::make_unique<event::ClosureEventListener>(
                  [](lepus::Value args) {}, event_options,
                  event::ClosureEventListener::ClosureType::kJS));
    bool is_piper_event = event_entry.second->is_piper_event();
    if (is_piper_event) {
      const auto& piper_event_vec = *event_entry.second->piper_event_vec();
      element()->AddEventListener(
          name,
          std::make_unique<event::ClosureEventListener>(
              [tasm = tasm_, piper_event_vec](lepus::Value args) {
                for (const auto& event : piper_event_vec) {
                  const auto& func_name = event.piper_func_name_.str();
                  auto func_args = event.piper_func_args_;
                  tasm->TriggerLepusBridgeAsync(func_name, func_args);
                }
              },
              event_options, event::ClosureEventListener::ClosureType::kJS));
    } else {
      const auto& callback = event_entry.second->function().str();
      element()->AddEventListener(
          name,
          std::make_unique<event::ClosureEventListener>(
              [tasm = tasm_, name, callback](lepus::Value args) {
                const auto& args_array = args.Array();
                if (args.IsArray() && args_array->size() == 3) {
                  const auto& event_info = args_array->get(0);
                  const auto& event_detail = args_array->get(1);
                  const auto& event_info_array = event_info.Array();
                  if (event_info.IsArray() && event_info_array->size() == 2) {
                    auto call_method_name = event_info_array->get(0).Bool();
                    auto page_name_or_component_id =
                        call_method_name
                            ? tasm->FindEntry(tasm::DEFAULT_ENTRY_NAME)
                                  ->GetName()
                            : event_info_array->get(1).StdString();
                    TRACE_EVENT(
                        LYNX_TRACE_CATEGORY, CLOSURE_EVENT_LISTENER_CLOSURE,
                        [&name, &callback, &page_name_or_component_id](
                            lynx::perfetto::EventContext ctx) {
                          ctx.event()->add_debug_annotations("name", name);
                          ctx.event()->add_debug_annotations("callback",
                                                             callback);
                          ctx.event()->add_debug_annotations(
                              "component", page_name_or_component_id);
                        });
                    LOGI(
                        "Invoke the Closure of ClosureEventListener for event: "
                        << name << " with callback: " << callback
                        << " in component: " << page_name_or_component_id)
                    auto message = lepus::CArray::Create();
                    message->emplace_back(page_name_or_component_id);
                    message->emplace_back(callback);
                    // info be ShallowCopy first to avoid to be marked const.
                    message->emplace_back(
                        lepus_value::ShallowCopy(event_detail));
                    auto event = fml::MakeRefCounted<runtime::MessageEvent>(
                        call_method_name
                            ? runtime::kMessageEventTypeSendPageEvent
                            : runtime::kMessageEventTypePublishComponentEvent,
                        runtime::ContextProxy::Type::kCoreContext,
                        runtime::ContextProxy::Type::kJSContext,
                        std::make_unique<pub::ValueImplLepus>(
                            lepus::Value(std::move(message))));
                    tasm->DispatchMessageEvent(std::move(event));
                  }
                }
              },
              event_options, event::ClosureEventListener::ClosureType::kJS));
    }
  } else {
#if ENABLE_LEPUSNG_WORKLET
    const auto& lepus_func = event_entry.second->lepus_function();
    const auto& lepus_script = event_entry.second->lepus_script();
    // remove the listener firstly to adapt rebind
    element()->RemoveEventListener(
        name, std::make_unique<event::ClosureEventListener>(
                  [](lepus::Value args) {}, event_options,
                  event::ClosureEventListener::ClosureType::kCore));
    element()->AddEventListener(
        name,
        std::make_unique<event::ClosureEventListener>(
            [tasm = tasm_, lepus_func, lepus_script](lepus::Value args) {
              const auto& args_array = args.Array();
              if (args.IsArray() && args_array->size() == 3) {
                const auto& event_info = args_array->get(0);
                const auto& event_detail = args_array->get(1);
                auto event = fml::static_ref_ptr_cast<event::Event>(
                    args_array->get(2).RefCounted());
                const auto& event_info_array = event_info.Array();
                if (event_info.IsArray() && event_info_array->size() == 3) {
                  const auto& component_id =
                      event_info_array->get(0).StdString();
                  const auto& entry_name = event_info_array->get(1).StdString();
                  int32_t element_id = event_info_array->get(2).Int32();

                  auto task_handler =
                      std::make_shared<worklet::LepusApiHandler>();
                  std::shared_ptr<PipelineOptions> current_option =
                      std::make_shared<PipelineOptions>();
                  PipelineScope pipeline_scope(tasm, current_option);
                  EventResult result = EventResult::kDefault;

                  result = lynx::worklet::LepusElement::FireElementWorklet(
                      component_id, entry_name, tasm, lepus_func, lepus_script,
                      event_detail, task_handler, element_id,
                      static_cast<EventType>(1));
                  // trigger patch finish when a worklet operation is
                  // completed
                  tasm->page_proxy()->element_manager()->SetNeedsLayout();
                  tasm->page_proxy()->element_manager()->RequestResolve(
                      current_option);
                  if (event == nullptr) {
                    return;
                  }
                  if (static_cast<int>(result) &
                      static_cast<int>(
                          EventResult::kStopImmediatePropagationBit)) {
                    event->set_is_stop_immediate_propagation(true);
                  } else if (static_cast<int>(result) &
                             static_cast<int>(
                                 EventResult::kStopPropagationBit)) {
                    event->set_is_stop_propagation(true);
                  }
                }
              }
            },
            event_options, event::ClosureEventListener::ClosureType::kCore));
#endif  // ENABLE_LEPUSNG_WORKLET
  }
}

bool RadonNode::ShouldFlushStyle(RadonNode* old_radon_node,
                                 const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_SHOULD_FLUSH_STYLE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  bool style_updated = false;
  attribute_holder_->SetCSSVariableBundle(*old_radon_node->attribute_holder());
  auto* fiber_element = static_cast<FiberElement*>(element_.get());
  if (id_dirty_) {
    style_updated = true;
    fiber_element->SetIdSelector(id_selector());
  }
  if (class_dirty_) {
    style_updated = true;
    fiber_element->SetClasses(attribute_holder_->ReleaseClasses());
    for (auto& child : old_radon_node->radon_children_) {
      child->MarkChildStyleDirtyRecursively(option.ShouldForceUpdate());
    }
  }
  if (has_dynamic_inline_style_) {
    style_updated |= DiffRawStyleForFiber(old_radon_node->raw_inline_styles(),
                                          raw_inline_styles());
  }
  return style_updated;
}

void RadonNode::CollectInvalidationSetsForPseudoAndInvalidate(
    std::shared_ptr<PipelineOptions>& pipeline_options,
    CSSFragment* style_sheet, PseudoState prev, PseudoState curr) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, RADON_SHOULD_COLLECT_INVALID_SETS_FOR_PSEUDO,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!style_sheet->enable_css_invalidation()) {
    return;
  }
  css::InvalidationLists invalidation_lists;
  CSSFragment::CollectPseudoChangedInvalidation(style_sheet, invalidation_lists,
                                                prev, curr);

  bool should_patch = false;
  for (auto* invalidation_set : invalidation_lists.descendants) {
    if (invalidation_set->InvalidatesSelf() && element()) {
      should_patch |= RefreshStyle();
    }
    if (invalidation_set->WholeSubtreeInvalid() ||
        !invalidation_set->IsEmpty()) {
      Visit(false, [&should_patch, invalidation_set, this](RadonBase* child) {
        if (child->IsRadonNode() && child->element() &&
            !child->TagName().IsEqual("raw-text") &&
            invalidation_set->InvalidatesElement(
                *static_cast<RadonNode*>(child)->attribute_holder_)) {
          should_patch |= static_cast<RadonNode*>(child)->RefreshStyle();
        }
        return !child->IsRadonComponent() ||
               (child->IsRadonComponent() && GetRemoveCSSScopeEnabled());
      });
    }
  }
  if (should_patch) {
    page_proxy_->element_manager()->SetNeedsLayout();
    page_proxy_->element_manager()->RequestResolve(pipeline_options);
  }
}

void RadonNode::OnPseudoStateChanged(PseudoState prev, PseudoState curr) {
  std::shared_ptr<PipelineOptions> pipeline_options;
  PipelineContext* current_context = nullptr;
  if (element_) {
    current_context = element_->element_manager()
                          ->element_manager_delegate()
                          ->GetCurrentPipelineContext();
  }
  if (current_context) {
    pipeline_options = current_context->GetOptions();
  } else {
    pipeline_options = std::make_shared<PipelineOptions>();
  }
  CSSFragment* style_sheet =
      GetRemoveCSSScopeEnabled() ? GetPageStyleSheet() : ParentStyleSheet();
  if (style_sheet && style_sheet->enable_css_selector()) {
    return CollectInvalidationSetsForPseudoAndInvalidate(
        pipeline_options, style_sheet, prev, curr);
  }

  bool should_patch = false;
  if (page_proxy_->element_manager()->GetEnableCascadePseudo()) {
    // Refresh styles of all descendants to support nested focus pseudo class
    Visit(true, [&should_patch](RadonBase* child) {
      if (child->IsRadonNode() && child->element()) {
        should_patch |= static_cast<RadonNode*>(child)->RefreshStyle();
      }
      return !child->IsRadonComponent() ||
             (child->IsRadonComponent() &&
              static_cast<RadonNode*>(child)->GetRemoveCSSScopeEnabled());
    });
  } else {
    should_patch = RefreshStyle();
  }
  if (should_patch) {
    page_proxy_->element_manager()->SetNeedsLayout();
    page_proxy_->RequestResolve(pipeline_options);
  }
}

bool RadonNode::RefreshStyle() {
  if (element() != nullptr) {
    StyleMap old_styles = std::move(cached_styles_);
    element()->ResolveStyle(cached_styles_);
    return DiffStyleImpl(old_styles, cached_styles_, true);
  }
  return false;
}

bool RadonNode::DiffRawStyleForFiber(const RawLepusStyleMap& old_map,
                                     const RawLepusStyleMap& new_map) {
  bool need_update = false;
  // check remove
  for (auto it = old_map.begin(); it != old_map.end(); ++it) {
    auto it_new_map = new_map.find(it->first);
    // style does not exist in new_map, delete it
    if (it_new_map == new_map.end()) {
      need_update = true;
      static_cast<FiberElement*>(element())->SetStyle(it->first,
                                                      lepus::Value());
    }
  }
  // Using JSValue across multiple threads
  // is not allowed. Therefore, JSValue needs to be converted to LepusValue
  // before the parallel flush to prevent conversion through LepusContext
  // during the parallel flush phase.
  const auto should_convert_to_lepus_value =
      page_proxy_->element_manager()->GetEnableParallelElement();
  // check update and insert
  for (auto& it : new_map) {
    // try to find the corresponding style in old_map
    auto it_old_map = old_map.find(it.first);
    // if r does not exist in lhs, r is a new style to add
    // if r exist in lhs but with different value, update it
    if (it_old_map == old_map.end() || !(it.second == it_old_map->second)) {
      need_update = true;
      if (should_convert_to_lepus_value) {
        fiber_element()->SetStyle(it.first, it.second.ToLepusValue());
      } else {
        fiber_element()->SetStyle(it.first, it.second);
      }
    }
  }

  return need_update;
}

bool RadonNode::DiffAttrMapForFiber(const AttrMap& old_map,
                                    const AttrMap& new_map) {
  bool need_update = false;
  // check remove
  for (auto it = old_map.begin(); it != old_map.end(); ++it) {
    auto it_new_map = new_map.find(it->first);
    // style does not exist in new_map, delete it
    if (it_new_map == new_map.end()) {
      need_update = true;
      static_cast<FiberElement*>(element())->SetAttribute(
          it->first, lepus::Value(), false);
    }
  }
  // Using JSValue across multiple threads
  // is not allowed. Therefore, JSValue needs to be converted to LepusValue
  // before the parallel flush to prevent conversion through LepusContext
  // during the parallel flush phase.
  const auto should_convert_to_lepus_value =
      page_proxy_->element_manager()->GetEnableParallelElement();
  // check update and insert
  if (!new_map.empty()) {
    fiber_element()->MarkCanBeLayoutOnly(false);
  }
  for (auto& it : new_map) {
    // try to find the corresponding style in old_map
    auto it_old_map = old_map.find(it.first);
    // if r does not exist in lhs, r is a new style to add
    // if r exist in lhs but with different value, update it
    if (it_old_map == old_map.end() || !(it.second == it_old_map->second)) {
      // Fiber and Radon architectures handle undefined/null property updates
      // differently:
      // - Fiber architecture sets property value to empty
      // - Radon architecture ignores the update
      // Early return here to maintain backward compatibility with Radon's
      // original behavior when processing empty values in Radon-Fiber
      // architecture
      if (it.second.IsEmpty()) {
        continue;
      }
      need_update = true;
      if (should_convert_to_lepus_value) {
        fiber_element()->SetAttribute(it.first, it.second.ToLepusValue(),
                                      false);
      } else {
        fiber_element()->SetAttribute(it.first, it.second, false);
      }
    }
  }

  return need_update;
}

void RadonNode::SetStaticInlineStyle(CSSPropertyID id,
                                     const base::String& string_value,
                                     const CSSParserConfigs& configs) {
  SetRawInlineStyle(id, lepus::Value(string_value), configs);
}

void RadonNode::SetStaticInlineStyle(CSSPropertyID id,
                                     base::String&& string_value,
                                     const CSSParserConfigs& configs) {
  SetRawInlineStyle(id, lepus::Value(std::move(string_value)), configs);
}

void RadonNode::SetStaticInlineStyle(CSSPropertyID id,
                                     const tasm::CSSValue& value) {
  SetRawInlineStyle(id, lepus::Value(CSSDecoder::CSSValueToString(id, value)));
}

void RadonNode::SetStaticInlineStyle(CSSPropertyID id, tasm::CSSValue&& value) {
  SetRawInlineStyle(id, lepus::Value(CSSDecoder::CSSValueToString(id, value)));
}

void RadonNode::SetRawInlineStyle(CSSPropertyID id, lepus::Value&& value,
                                  const CSSParserConfigs& configs) {
  if (raw_inline_styles_.contains(id)) {
    // 1.  If the style property `id` already exists in the `raw_inline_styles_`
    // map:
    //   The function first validates the new `value` by calling
    //   `UnitHandler::Process`.
    //   - If the new value is valid, it updates the existing style property by
    //   delegating to the `SetRawInlineStyle(CSSPropertyID, lepus::Value&&)`
    //   overload.
    //   - If the new value is invalid, the update is ignored, and the previous
    //   valid style value is preserved. This ensures that an existing
    //   valid style is not overwritten by an invalid one.
    StyleMap styles;
    bool process_success = UnitHandler::Process(id, value, styles, configs);
    if (process_success) {
      SetRawInlineStyle(id, std::move(value));
    }
  } else {
    // 2. If the style property `id` does not exist:
    //    The function adds the new style property directly without any
    //    validation. This is a deliberate performance optimization to avoid the
    //    cost of `UnitHandler::Process` for every new style property addition,
    //    which can be frequent in rendering scenarios.
    SetRawInlineStyle(id, std::move(value));
  }
}

bool RadonNode::DiffStyleImpl(const StyleMap& old_map, const StyleMap& new_map,
                              bool check_remove) {
  bool need_update = false;
  base::InlineVector<CSSPropertyID, 16> reset_style_names;
  if (check_remove) {
    for (auto it = old_map.begin(); it != old_map.end();) {
      auto it_new_map = new_map.find(it->first);
      // style does not exist in rhs, delete it
      if (it_new_map == new_map.end()) {
        auto key = it->first;
        need_update = true;
        reset_style_names.push_back(key);
        ++it;
        if (!force_calc_new_style_) {
          // Optimized CSSStyle Diff logic: should update cached_styles_
          cached_styles_.erase(key);
        }
      } else {
        ++it;
      }
    }
    if (LynxEnv::GetInstance().FixRadonTransitionPropertyRemoveBug() == false) {
      element()->ResetStyle(reset_style_names);
    }
  }

  // iterate all styles in new_map
  StyleMap update_styles;
  update_styles.reserve(new_map.size());
  cached_styles_.reserve(new_map.size());
  for (auto& it : new_map) {
    // try to find the corresponding style in old_map
    auto it_old_map = old_map.find(it.first);
    // if r does not exist in lhs, r is a new style to add
    // if r exist in lhs but with different value, update it
    if (it_old_map == old_map.end() || !(it.second == it_old_map->second)) {
      need_update = true;
      update_styles.insert_or_assign(it.first, it.second);
      if (!force_calc_new_style_) {
        // Optimized CSSStyle Diff logic: should update cached_styles_
        cached_styles_.insert_or_assign(it.first, it.second);
      }
    }
    // no need to update: it_old_map != old_map.end() && it.second ==
    // it_old_map->second
  }
  if (LynxEnv::GetInstance().FixRadonTransitionPropertyRemoveBug() == true) {
    // In Radon, if there are transform properties in update_styles, and
    // properties value will be removed, try consume transition first.
    bool should_consume_trans_styles_in_advance =
        element()->ShouldConsumeTransitionStylesInAdvance();
    if (should_consume_trans_styles_in_advance) {
      element()->ConsumeTransitionStylesInAdvance(update_styles);
    }

    element()->ResetStyle(reset_style_names);
  }
  element()->ConsumeStyle(update_styles);
  return need_update;
}

// DevTool related functions.
bool RadonNode::GetDevToolFlag() {
  return page_proxy_->element_manager()->GetDevToolFlag() &&
         page_proxy_->element_manager()->IsDomTreeEnabled();
}

void RadonNode::NotifyElementNodeAdded() {
  EXEC_EXPR_FOR_INSPECTOR({
    if (GetDevToolFlag()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, DEVTOOL_NOTIFY_ELEMENT_NODE_ADDED);
      if (element() != nullptr) {
        page_proxy_->element_manager()->OnElementNodeAddedForInspector(
            element());
      }
    }
  });
}

void RadonNode::NotifyElementNodeRemoved() {
  EXEC_EXPR_FOR_INSPECTOR({
    if (GetDevToolFlag()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, DEVTOOL_NOTIFY_ELEMENT_NODE_REMOVED);
      if (element() != nullptr) {
        page_proxy_->element_manager()->OnElementNodeRemovedForInspector(
            element());
      }
    }
  });
}

void RadonNode::NotifyElementNodeSetted() {
  EXEC_EXPR_FOR_INSPECTOR({
    if (GetDevToolFlag()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY, DEVTOOL_NOTIFY_ELEMENT_NODE_SETTED);
      if (element() != nullptr) {
        page_proxy_->element_manager()->OnElementNodeSetForInspector(element());
      }
    }
  });
}

RadonPlug* RadonNode::GetRadonPlug() {
  EXEC_EXPR_FOR_INSPECTOR({
    RadonBase* current = this;
    RadonBase* parent = current->Parent();
    while (parent) {
      if (parent->NodeType() == kRadonPlug) {
        return static_cast<RadonPlug*>(parent);
      } else {
        return nullptr;
      }
    }
  });
  return nullptr;
}

RadonNode* RadonNode::NodeParent() {
  RadonBase* parent = radon_parent_;
  while (parent != nullptr && !parent->NeedsElement() &&
         !parent->IsRadonComponent()) {
    parent = parent->Parent();
  }
  if (parent) {
    return static_cast<RadonNode*>(parent);
  }
  return nullptr;
}

RadonNode* RadonNode::Sibling(int offset) const {
  if (!radon_parent_) return nullptr;

  if (NodeType() == kRadonPlug) {
    return static_cast<RadonNode*>(radon_parent_)->Sibling(offset);
  }
  if (radon_parent_->NodeType() == kRadonPlug) {
    auto* slot = radon_parent_->radon_parent_;
    if (slot) {
      return static_cast<RadonNode*>(slot)->Sibling(offset);
    } else {
      return nullptr;
    }
  }
  const auto& siblings = radon_parent_->radon_children_;
  auto iter =
      std::find_if(siblings.begin(), siblings.end(),
                   [id = ImplId()](auto& ptr) { return ptr->ImplId() == id; });
  auto dist = std::distance(siblings.begin(), iter) + offset;
  if (dist < 0 || dist >= static_cast<long>(siblings.size())) {
    return nullptr;
  }
  return static_cast<RadonNode*>(siblings[dist].get());
}

// the sibling function is used to get the sibling node of current node, since
// there may be many sibling nodes, so we need to speicify the sibling node by
// passing the index. if the param value is positive, means get the sibling node
// behind the current node, otherwise, negative means get the sibling node in
// front of current node.
AttributeHolder* RadonNode::NextSibling() const {
  auto next_sibling = Sibling(1);
  if (next_sibling && next_sibling->attribute_holder_) {
    return next_sibling->attribute_holder_.get();
  }
  return nullptr;
}

AttributeHolder* RadonNode::PreviousSibling() const {
  auto previous_sibling = Sibling(-1);
  if (previous_sibling && previous_sibling->attribute_holder_) {
    return previous_sibling->attribute_holder_.get();
  }
  return nullptr;
}

size_t RadonNode::ChildCount() const { return radon_children_.size(); }

RadonNode* RadonNode::FirstNodeChild() {
  RadonBase* child =
      radon_children_.empty() ? nullptr : radon_children_[0].get();
  if (child != nullptr && !child->NeedsElement() &&
      !child->IsRadonComponent()) {
    child = child->radon_children_.empty() ? nullptr
                                           : child->radon_children_[0].get();
  }
  if (child) {
    return static_cast<RadonNode*>(child);
  }
  return nullptr;
}

RadonNode* RadonNode::LastNodeChild() {
  RadonBase* child =
      radon_children_.empty() ? nullptr : radon_children_.back().get();
  if (child != nullptr && !child->NeedsElement() &&
      !child->IsRadonComponent()) {
    child = child->radon_children_.empty()
                ? nullptr
                : child->radon_children_.back().get();
  }
  if (child) {
    return static_cast<RadonNode*>(child);
  }
  return nullptr;
}

void RadonNode::UpdateInlineStylesFromOldModel(
    AttributeHolder* const old_data_model) {
  EXEC_EXPR_FOR_INSPECTOR({
    if (GetDevToolFlag()) {
      TRACE_EVENT(LYNX_TRACE_CATEGORY,
                  DEVTOOL_UPDATE_INLINE_STYLES_FROM_OLD_MODEL);
      if (element()) {
        // In the Radon-Fiber architecture, new RadonNode nodes only store
        // raw_inline_style, which cannot be consumed by the devtool. The
        // inline_styles_ in DataModel stores the parsed inline styles, which
        // are used by the DevTool. During the Diff process, we need to move the
        // parsed inline styles from the previous DataModel to the new DataModel
        // to ensure that the inline styles in the DevTool panel are always
        // up-to-date.
        attribute_holder_->SetInlineStyles(
            old_data_model->ReleaseInlineStyles());
      }
    }
  });
}

void RadonNode::CheckAndProcessSlotForInspector(Element* element) {
  EXEC_EXPR_FOR_INSPECTOR({
    // FIXME(zhengyuwei): adjust the location of the code below
    // eg: move to radon plug? to be discussed
    if (GetDevToolFlag()) {
      RadonPlug* radon_plug;
      if ((radon_plug = GetRadonPlug())) {
        page_proxy_->element_manager()->RunDevToolFunction(
            lynx::devtool::DevToolFunction::InitPlugForInspector,
            std::make_tuple(element));
      }
    }
  });
}

void RadonNode::CheckAndProcessComponentRemoveViewForInspector(
    Element* element) {
  EXEC_EXPR_FOR_INSPECTOR({
    if (GetDevToolFlag()) {
      RadonNode* current = this;
      RadonBase* parent = current->Parent();
      while (parent && !parent->NeedsElement()) {
        if (parent->IsRadonComponent() && !parent->element()) {
          // TODO(kechenglong): FiberComponent also creates an element.
          // CheckAndProcessComponentRemoveViewForInspector can be removed after
          // switching to FiberElement in RadonDiff.
          fml::RefPtr<Element> component_element = nullptr;
          auto* parent_component = static_cast<RadonComponent*>(parent);
          auto fiber_element =
              page_proxy_->element_manager()->CreateFiberComponent(
                  parent_component->ComponentStrId(),
                  parent_component->GetCSSId(),
                  parent_component->GetEntryName(), parent_component->name(),
                  parent_component->path());
          fiber_element->SetNodeIndex(parent_component->NodeIndex());
          fiber_element->SetParentComponentUniqueIdForFiber(
              parent_component->ParentComponentElementId());
          fiber_element->set_style_sheet_manager(
              parent_component->style_sheet_manager());
          component_element = fiber_element;
          page_proxy_->element_manager()->PrepareNodeForInspector(
              component_element.get());
          component_element->inspector_attribute()->needs_erase_id_ = true;

          static_cast<RadonNode*>(parent)->element_ = component_element;
        }
        parent = parent->Parent();
      }

      page_proxy_->element_manager()->RunDevToolFunction(
          lynx::devtool::DevToolFunction::InitStyleRoot,
          std::make_tuple(element));
    }
  });
}

bool RadonNode::HydrateNode(const DispatchOption& option) {
  if (option.ssr_hydrating_) {
    return page_proxy_->element_manager()->Hydrate(
        this->attribute_holder_.get(), element());
  }
  return false;
}

// Used when creating Element.
// The return value will be used to map to the original code written by users
// (UI Sourcemap). Some nodes (eg: <raw-text>) do not have node_index, we will
// search their parent nodes until a valid node_index is found.
RadonNodeIndexType RadonNode::GetOriginalNodeIndex() {
  if (node_index_ != kRadonInvalidNodeIndex) {
    return node_index_;
  } else {
    auto next = Parent();
    while (next) {
      RadonNodeIndexType node_index = next->NodeIndex();
      if (node_index != kRadonInvalidNodeIndex) {
        return node_index;
      }
      next = next->Parent();
    }
    return kRadonInvalidNodeIndex;
  }
}

}  // namespace tasm
}  // namespace lynx
