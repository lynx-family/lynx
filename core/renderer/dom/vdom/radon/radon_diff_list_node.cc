// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/renderer/dom/vdom/radon/radon_diff_list_node.h"

#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/page_proxy.h"
#include "core/renderer/template_assembler.h"
#include "core/services/feature_count/feature_counter.h"

namespace lynx {
namespace tasm {

// TODO: 1. check component name valid.  2. read diffable attribute.
RadonDiffListNode::RadonDiffListNode(lepus::Context* context,
                                     PageProxy* page_proxy,
                                     TemplateAssembler* tasm,
                                     uint32_t node_index)
    : RadonListBase(context, page_proxy, tasm, node_index) {
  tasm::report::FeatureCounter::Instance()->Count(
      tasm::report::LynxFeature::CPP_LIST_NEW_ARCH_NOT_ENABLED);
}

void RadonDiffListNode::SyncComponentExtraInfo(RadonComponent* comp,
                                               uint32_t index,
                                               int64_t operation_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonDiffListNode::SyncComponentExtraInfo",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  std::unique_ptr<RadonComponent> original_component_node;
  if (comp->dispatched()) {
    PtrLookupMap lookup_map;
    original_component_node =
        std::make_unique<RadonComponent>(*comp, lookup_map);
  }

  RadonListBase::SyncComponentExtraInfo(comp, index, operation_id);
  auto& comp_info = components_[index];
  const lepus::Value& props = comp_info->properties_;
  DispatchOption dispatch_option(page_proxy_);
  PipelineOptions pipeline_options;

  if (!comp->dispatched()) {
    comp->UpdateRadonComponentWithoutDispatch(
        RadonComponent::RenderType::UpdateByNative, props, comp_info->data_);

    RenderOption render_option;
    render_option.recursively = true;
    comp->RenderRadonComponentIfNeeded(render_option);
    comp->Dispatch(dispatch_option);
  } else {
    bool should_flush =
        comp->ShouldFlush(std::move(original_component_node), dispatch_option);
    if (should_flush) {
      comp->element()->FlushProps();
      dispatch_option.has_patched_ = true;
    }
    dispatch_option.css_variable_changed_ =
        comp_info->list_component_dispatch_option_.css_variable_changed_;
    dispatch_option.global_properties_changed_ =
        comp_info->list_component_dispatch_option_.global_properties_changed_;
    dispatch_option.force_diff_entire_tree_ =
        comp_info->list_component_dispatch_option_.force_diff_entire_tree_;
    dispatch_option.use_new_component_data_ =
        comp_info->list_component_dispatch_option_.use_new_component_data_;
    dispatch_option.refresh_lifecycle_ =
        comp_info->list_component_dispatch_option_.refresh_lifecycle_;
    PipelineOptions pipeline_options;
    comp->UpdateRadonComponent(RadonComponent::RenderType::UpdateByNative,
                               props, comp_info->data_, dispatch_option,
                               pipeline_options);
    root_node()->proxy_->OnComponentPropertyChanged(comp);
    comp_info->list_component_dispatch_option_.reset();
  }
  if (dispatch_option.has_patched_) {
    page_proxy_->element_manager()->SetNeedsLayout();
  };

  pipeline_options.operation_id = operation_id;
  page_proxy_->element_manager()->OnPatchFinish(pipeline_options);
}

bool RadonDiffListNode::ShouldFlush(
    const std::unique_ptr<RadonBase>& old_radon_child,
    const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonDiffListNode::ShouldFlush",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!old_radon_child || old_radon_child->NodeType() != kRadonListNode) {
    return false;
  }
  bool should_flush = RadonNode::ShouldFlush(old_radon_child, option);
  components_ = std::move(new_components_);
  FilterComponents(components_, tasm_);
  platform_info_.Generate(components_);
  platform_info_.diffable_list_result_ = true;
  auto* old = static_cast<RadonDiffListNode*>(old_radon_child.get());
  bool list_updated = MyersDiff(old->components_, option.ShouldForceUpdate());
  for (size_t i = 0; i < platform_info_.update_actions_.update_from_.size();
       i++) {
    auto from = platform_info_.update_actions_.update_from_[i];
    auto to = platform_info_.update_actions_.update_to_[i];
    TransmitDispatchOptionFromOldComponentToNewComponent(
        *old->components_[from], *components_[to]);
  }
  // For Radon-Fiber Arch, should force flush props.
  if (list_updated && element()->is_fiber_element()) {
    // forcibly trigger a UI update, even if there is no valid content within
    // the prop_bundle.
    fiber_element()->MarkPropsDirty();
    fiber_element()->PreparePropBundleIfNeed();
  }
  return should_flush || list_updated;
}

void RadonDiffListNode::TransmitDispatchOptionFromOldComponentToNewComponent(
    const ListComponentInfo& old_component, ListComponentInfo& new_component) {
  new_component.list_component_dispatch_option_.global_properties_changed_ |=
      old_component.list_component_dispatch_option_.global_properties_changed_;

  new_component.list_component_dispatch_option_.css_variable_changed_ |=
      old_component.list_component_dispatch_option_.css_variable_changed_;

  new_component.list_component_dispatch_option_.force_diff_entire_tree_ |=
      old_component.list_component_dispatch_option_.force_diff_entire_tree_;

  new_component.list_component_dispatch_option_.use_new_component_data_ |=
      old_component.list_component_dispatch_option_.use_new_component_data_;

  new_component.list_component_dispatch_option_.refresh_lifecycle_ |=
      old_component.list_component_dispatch_option_.refresh_lifecycle_;
}

void RadonDiffListNode::RadonDiffChildren(
    const std::unique_ptr<RadonBase>& old_radon_child,
    const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonDiffListNode::RadonDiffChildren",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!old_radon_child || old_radon_child->NodeType() != kRadonListNode) {
    return;
  }
  auto* old = static_cast<RadonDiffListNode*>(old_radon_child.get());
  for (auto& child : old->radon_children_) {
    AddChild(std::move(child));
  }
  NeedModifySubTreeComponent(component());
  TransmitDispatchOptionFromListNodeToListComponent(option);
}

void RadonDiffListNode::TransmitDispatchOptionFromListNodeToListComponent(
    const DispatchOption& option) {
  if (option.css_variable_changed_) {
    for (auto& comp : components_) {
      comp->list_component_dispatch_option_.css_variable_changed_ = true;
    }
  }
  if (option.global_properties_changed_) {
    for (auto& comp : components_) {
      comp->list_component_dispatch_option_.global_properties_changed_ = true;
    }
  }
  if (option.force_diff_entire_tree_) {
    for (auto& comp : components_) {
      comp->list_component_dispatch_option_.force_diff_entire_tree_ = true;
    }
  }
  if (option.use_new_component_data_) {
    for (auto& comp : components_) {
      comp->list_component_dispatch_option_.use_new_component_data_ = true;
    }
  }
  if (option.refresh_lifecycle_) {
    for (auto& comp : components_) {
      comp->list_component_dispatch_option_.refresh_lifecycle_ = true;
    }
  }
}

void RadonDiffListNode::DispatchFirstTime() {
  platform_info_.diffable_list_result_ = false;
  DiffListComponents();
  RadonNode::DispatchFirstTime();
}

}  // namespace tasm
}  // namespace lynx
