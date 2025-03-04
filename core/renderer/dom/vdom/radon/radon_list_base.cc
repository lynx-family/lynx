// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_list_base.h"

#include <base/include/string/string_utils.h>

#include <memory>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/component_attributes.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_lazy_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/diff_algorithm.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/vm/lepus/array.h"
#include "core/services/long_task_timing/long_task_monitor.h"

namespace lynx {
namespace tasm {

// Although the exact number of list nodes cannot be known, given that the
// array only stores pointer-sized data, some space can be reserved to avoid
// frequent reallocations and data movements when vector auto-expands.
static constexpr size_t kListComponentsReservingSize = 64;

void RadonListBase::AppendComponentInfo(
    std::unique_ptr<ListComponentInfo> info) {
  new_components_.push_back(std::move(info));
}

RadonListBase::RadonListBase(const RadonListBase& node, PtrLookupMap& map)
    : RadonNode{node, map}, context_{node.context_}, tasm_{node.tasm_} {
  new_components_.reserve(kListComponentsReservingSize);
}

// TODO: 1. check component name valid.  2. read diffable attribute.
RadonListBase::RadonListBase(lepus::Context* context, PageProxy* page_proxy,
                             TemplateAssembler* tasm, uint32_t node_index)
    : RadonNode{page_proxy, BASE_STATIC_STRING(kListNodeTag), node_index},
      context_{context},
      tasm_{tasm} {
  RadonNode::node_type_ = kRadonListNode;
  if (page_proxy) {
    platform_info_.enable_move_operation_ =
        page_proxy->GetListEnableMoveOperation();
    platform_info_.enable_plug_ = page_proxy->GetListEnablePlug();
  }
  new_components_.reserve(kListComponentsReservingSize);
}

RadonComponent* RadonListBase::CreateComponentWithType(uint32_t index) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonListBase::CreateComponentWithType",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  const auto& component_info = *components_[index];
  auto lepus_component_name = base::String{component_info.name_};

  RadonComponent* result = nullptr;
  if (!IsStaticComponent(component_info.name_)) {
    // lazy component
    auto current_entry = component_info.current_entry_;
    const auto& url = tasm_->GetTargetUrl(current_entry, component_info.name_);
    // for lazy components in list, tid is 0 and index is 0;
    RadonLazyComponent* lazy_bundle =
        RadonLazyComponent::CreateRadonLazyComponent(
            tasm_, url, lepus_component_name, 0, 0);
    result = static_cast<RadonComponent*>(lazy_bundle);
  } else {
    // static component
    const lepus::Value& info =
        component()->GetComponentInfoMap().GetProperty(lepus_component_name);
    const lepus::Value& path =
        component()->GetComponentPathMap().GetProperty(lepus_component_name);

    // DCHECK(info.Array().Get()->size() == 2);
    const auto tid = static_cast<int>(info.Array().get()->get(0).Number());

    const auto& name = component()->GetEntryName().empty()
                           ? DEFAULT_ENTRY_NAME
                           : component()->GetEntryName();
    ComponentMould* cm = tasm_->component_moulds(name).find(tid)->second.get();
    auto* page_proxy = tasm_->page_proxy();
    result = new RadonListComponent{page_proxy,
                                    tid,
                                    nullptr,
                                    tasm_->style_sheet_manager(name),
                                    cm,
                                    context_,
                                    kRadonInvalidNodeIndex,
                                    component_info.distance_from_root_};
    result->SetPath(path.String());
    if (tasm_->GetPageConfig()->GetDSL() == PackageInstanceDSL::REACT) {
      result->SetDSL(PackageInstanceDSL::REACT);
      // set "getDerivedStateFromProps" function for react component
      result->SetGetDerivedStateFromPropsProcessor(
          tasm_->GetComponentProcessorWithName(result->path().str(),
                                               REACT_PRE_PROCESS_LIFECYCLE,
                                               context_->name()));
      // set "getDerivedStateFromError" function for react component
      result->SetGetDerivedStateFromErrorProcessor(
          tasm_->GetComponentProcessorWithName(result->path().str(),
                                               REACT_ERROR_PROCESS_LIFECYCLE,
                                               context_->name()));
      result->SetShouldComponentUpdateProcessor(
          tasm_->GetComponentProcessorWithName(result->path().str(),
                                               REACT_SHOULD_COMPONENT_UPDATE,
                                               context_->name()));
    }
    result->SetName(std::move(lepus_component_name));
  }
  BASE_STATIC_STRING_DECL(kFlatten, "flatten");
  BASE_STATIC_STRING_DECL(kFalse, "false");
  result->SetDynamicAttribute(kFlatten, lepus_value(kFalse));
  AddChild(std::unique_ptr<RadonBase>{result});
  return result;
}

void RadonListBase::SyncComponentExtraInfo(RadonComponent* comp, uint32_t index,
                                           int64_t operation_id) {
  auto* timing = tasm::timing::LongTaskMonitor::Instance()->GetTopTimingPtr();
  if (timing != nullptr) {
    timing->task_info_ = comp->name().str();
  }
  const auto& comp_info = components_[index];
  const lepus::Value& props = comp_info->properties_;
  DCHECK(props.IsObject());
  comp->SetStaticAttribute(
      BASE_STATIC_STRING(ListComponentInfo::kListCompType),
      lepus::Value(static_cast<uint32_t>(comp_info->type_)));

  comp->SetClass(comp_info->clazz_.String());
  comp->SetIdSelector(comp_info->ids_.String());
  ForEachLepusValue(
      props, [comp](const lepus::Value& key, const lepus::Value& val) {
        auto key_str = key.String();
        if (ComponentAttributes::GetAttrNames().count(key_str.str()) > 0) {
          comp->SetDynamicAttribute(key_str, val);
        }
      });
  auto splits =
      base::SplitStringByCharsOrderly<':', ';'>(comp_info->style_.StringView());
  auto& parser_configs = tasm_->GetPageConfig()->GetCSSParserConfigs();
  for (size_t i = 0; i + 1 < splits.size(); i = i + 2) {
    std::string key = base::TrimString(splits[i]);
    std::string value = base::TrimString(splits[i + 1]);

    CSSPropertyID id = CSSProperty::GetPropertyID(key);
    if (CSSProperty::IsPropertyValid(id) && value.length() > 0) {
      comp->SetInlineStyle(id, base::String(std::move(value)), parser_configs);
    }
  }
  if (comp_info->event_.IsArrayOrJSArray()) {
    BASE_STATIC_STRING_DECL(kType, "type");
    BASE_STATIC_STRING_DECL(kName, "name");
    BASE_STATIC_STRING_DECL(kScript, "script");
    BASE_STATIC_STRING_DECL(kValue, "value");
    ForEachLepusValue(
        comp_info->event_, [comp, &kType, &kName, &kScript, &kValue](
                               const auto& key, const auto& value) {
          if (value.Contains("script")) {
            comp->SetLepusEvent(value.GetProperty(kType).String(),
                                value.GetProperty(kName).String(),
                                value.GetProperty(kScript),
                                value.GetProperty(kValue));
          } else {
            comp->SetStaticEvent(value.GetProperty(kType).String(),
                                 value.GetProperty(kName).String(),
                                 value.GetProperty(kValue).String());
          }
        });
  }

  if (comp_info->dataset_.IsObject()) {
    ForEachLepusValue(comp_info->dataset_, [&comp](const lepus::Value& key,
                                                   const lepus::Value& value) {
      comp->SetDataSet(key.String(), value);
    });
  }
  comp->SetDSL(tasm_->GetPageDSL());
  BASE_STATIC_STRING_DECL(item_key, "item-key");
  if (props.Contains(item_key)) {
    comp->SetDynamicAttribute(item_key, props.GetProperty(item_key));
  }

  if (platform_info_.enable_plug_) {
    static_cast<RadonListComponent*>(comp)->set_distance_from_root(
        comp_info->distance_from_root_);
  }
}

void RadonListBase::RenderComponentAtIndex(uint32_t index,
                                           int64_t operation_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "List::RenderComponent",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetInstanceId(), tasm::timing::kListNodeTask,
      tasm::timing::kTaskNameRadonListBaseRenderAtIndex);
  DCHECK(index < platform_info_.components_.size());
  auto* comp = CreateComponentWithType(index);
  if (comp != nullptr) {
    auto config = tasm_->page_proxy()->GetConfig();
    comp->UpdateSystemInfo(GenerateSystemInfo(&config));
    SyncComponentExtraInfo(comp, index, operation_id);
  }
  // FIXME(heshan):invoke RenderComponentAtIndex in LynxEngine
  tasm_->page_proxy()
      ->element_manager()
      ->painting_context()
      ->FlushImmediately();
}

RadonComponent* RadonListBase::GetComponent(uint32_t sign) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "List::GetComponent",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto& patching = tasm_->page_proxy()->element_manager();
  auto* node = patching->node_manager()->Get(sign);
  if (node == nullptr) {
    return nullptr;
  }
  auto* comp =
      static_cast<RadonComponent*>(node->data_model()->radon_node_ptr());
  return comp;
}

// this is only called when the platform list is __DEALLOCATED__
// thus we do not need to care
// @param sign is the sign of the __LynxListTableViewCell__
// use GetParam could get the associated RadonComponent
void RadonListBase::RemoveComponent(uint32_t sign) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "List::RemoveComponent",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  auto* comp = GetComponent(sign);
  if (comp == nullptr) {
    return;
  }
  // remove its element
  comp->RemoveElementFromParent();
  // dtor its radon subtree in post order
  comp->ClearChildrenRecursivelyInPostOrder();
  // notify its element is removed
  comp->OnElementRemoved(0);
  // remove it from its parent
  auto unique_comp_ptr = this->RemoveChild(comp);
  // comp deleted here

  // FIXME(heshan):invoke RemoveComponent in LynxEngine
  tasm_->page_proxy()
      ->element_manager()
      ->painting_context()
      ->FlushImmediately();
}

void RadonListBase::UpdateComponent(uint32_t sign, uint32_t row,
                                    int64_t operation_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, "List::UpdateComponent",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  DCHECK(row < platform_info_.components_.size());
  if (row < 0 || row >= components_.size()) {
    LOGE("row out of range in RadonListBase::UpdateComponent.");
    return;
  }
  auto* comp = GetComponent(sign);
  if (!comp) {
    LOGE("comp is nullptr in RadonListBase::UpdateComponent.");
    return;
  }
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetInstanceId(), tasm::timing::kListNodeTask,
      tasm::timing::kTaskNameRadonListBaseUpdateComponent);
  SyncComponentExtraInfo(comp, row, operation_id);

  // FIXME(heshan):invoke UpdateComponent in LynxEngine
  tasm_->page_proxy()
      ->element_manager()
      ->painting_context()
      ->FlushImmediately();
}

void RadonListBase::DispatchFirstTime() {
  platform_info_.diffable_list_result_ = false;
  RadonNode::DispatchFirstTime();
}

bool RadonListBase::IsStaticComponent(const std::string& name) {
  const auto& info_map = component()->GetComponentInfoMap();
  if (!info_map.Contains(name)) {
    return false;
  }
  const auto& info = info_map.GetProperty(name);
  if (!info.IsArrayOrJSArray() || info.GetLength() < 1) {
    return false;
  }
  auto tid = info.GetProperty(0);
  if (!tid.IsNumber() || tid.Number() < 0) {
    return false;
  }
  return true;
}

bool RadonListBase::HasComponent(const std::string& component_name,
                                 const std::string& current_entry) {
  if (IsStaticComponent(component_name)) {
    return true;
  } else {
    // component is not a static component
    // should component exist, it must be a lazy component, current_entry is
    // required to check its existence.
    const auto& url = tasm_->GetTargetUrl(current_entry, component_name);
    auto entry = tasm_->FindTemplateEntry(url);
    if (!entry) {
      return true;
    }
    auto cm_it = entry->lazy_bundle_moulds().find(0);
    if (cm_it == entry->lazy_bundle_moulds().end()) {
      return false;
    }
    auto& cm = cm_it->second;
    if (!cm || cm->path().empty()) {
      return false;
    }
    return true;
  }
}

bool RadonListBase::DiffListComponents() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonListBase::DiffListComponents",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  FilterComponents(new_components_, tasm_);
  bool is_updating_config = page_proxy_->is_updating_config();
  platform_info_.update_actions_ = myers_diff::MyersDiff(
      false, components_.begin(), components_.end(), new_components_.begin(),
      new_components_.end(),
      [](const auto& lhs, const auto& rhs) { return lhs->CanBeReusedBy(*rhs); },
      [is_updating_config](const auto& lhs, const auto& rhs) {
        return !is_updating_config && (*lhs == *rhs);
      });

  auto need_flush = !platform_info_.update_actions_.Empty();

  if (need_flush) {
    components_ = std::move(new_components_);
    platform_info_.Generate(components_);
  } else {
    platform_info_.update_actions_.Clear();
    new_components_.clear();
  }
  return need_flush;
}

}  // namespace tasm
}  // namespace lynx
