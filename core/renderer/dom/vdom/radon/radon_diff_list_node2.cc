// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/vdom/radon/radon_diff_list_node2.h"

#include <map>
#include <string>
#include <unordered_set>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/base/lynx_trace_categories.h"
#include "core/renderer/dom/vdom/radon/list_reuse_pool.h"
#include "core/renderer/dom/vdom/radon/radon_component.h"
#include "core/renderer/dom/vdom/radon/radon_page.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/ui_component/list/list_types.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/diff_algorithm.h"
#include "core/services/long_task_timing/long_task_monitor.h"

namespace {
constexpr static const char kListDefaultItemKeyPrefix[] =
    "lynx-list-default-item-key";
static uint64_t kAnonymousItemCount = 0;
}  // namespace

namespace lynx {
namespace tasm {

RadonDiffListNode2::RadonDiffListNode2(lepus::Context* context,
                                       PageProxy* page_proxy,
                                       TemplateAssembler* tasm,
                                       uint32_t node_index)
    : RadonListBase(context, page_proxy, tasm, node_index),
      reuse_pool_{std::make_unique<ListReusePool>()} {
  platform_info_.new_arch_list_ = true;
}

bool RadonDiffListNode2::ShouldFlush(
    const std::unique_ptr<RadonBase>& old_radon_child,
    const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonDiffListNode::ShouldFlush",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!old_radon_child || old_radon_child->NodeType() != kRadonListNode) {
    return false;
  }
  auto* old = static_cast<RadonDiffListNode2*>(old_radon_child.get());

  auto& new_components = new_components_;
  auto& old_components = old->components_;

  // check if the list node itself needs flush.
  bool should_flush = RadonNode::ShouldFlush(old_radon_child, option);

  // move resources from the old component to the new one.
  reuse_pool_ = std::move(old->reuse_pool_);

  // filter illegal components, i.e. components whose name do not exist.
  // all illegal components are removed before the generation of the
  // platform_info thus they are treated as if they have never appended
  // themselves into the `components_`
  FilterComponents(new_components_, tasm_);
  platform_info_.Generate(new_components_);
  platform_info_.new_arch_list_ = true;
  platform_info_.diffable_list_result_ = true;

  // generate the `platform_info_.update_actions_` by diff'ing.
  bool list_updated =
      MyersDiff(old_components, new_components, option.ShouldForceUpdate());

  // if a item-key is removed and inserted again, reset list_need_remove flag,
  // so that it can be reused
  for (auto index : platform_info_.update_actions_.insertions_) {
    auto item_key = new_components[index]->diff_key_.String();
    auto* component =
        reuse_pool_->GetComponentFromListKeyComponentMap(item_key);
    if (component != nullptr) {
      component->set_list_need_remove(false);
      component->set_list_need_remove_after_reused(false);
    }
  }

  // remove the JS components here, and mark the Native components as "need to
  // reset data", so that next time when the same component is inserted, it data
  // will be reset

  bool remove_component = page_proxy_->GetListRemoveComponent();

  for (auto index : platform_info_.update_actions_.removals_) {
    auto item_key = old_components[index]->diff_key_.String();
    auto* component =
        reuse_pool_->GetComponentFromListKeyComponentMap(item_key);
    // We always save its JS counterpart, no matter whether it's plug or not.
    if (component) {
      component->OnComponentRemovedInPostOrder();
      component->set_need_reset_data(true);
      // remove outdated radon from reuse_pool
      if (remove_component) {
        reuse_pool_->Remove(item_key,
                            base::String{old_components[index]->name_});
      }
    }
  }

  for (size_t i = 0; i < platform_info_.update_actions_.update_from_.size();
       i++) {
    auto from = platform_info_.update_actions_.update_from_[i];
    auto to = platform_info_.update_actions_.update_to_[i];
    TransmitDispatchOptionFromOldComponentToNewComponent(*old_components[from],
                                                         *new_components[to]);
  }

  components_ = std::move(new_components_);

  SetupListInfo(list_updated);
  if (list_updated) {
    element()->PropsUpdateFinish();
  }
  return should_flush || list_updated;
}

void RadonDiffListNode2::TransmitDispatchOptionFromOldComponentToNewComponent(
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

void RadonDiffListNode2::SetupListInfo(bool list_updated) {
  // assemble diff_result and current components, and dispatch them to platform
  // by updating props
  auto lepus_platform_result = lepus::Dictionary::Create();

  BASE_STATIC_STRING_DECL(kDiffable, "diffable");
  lepus_platform_result->SetValue(kDiffable,
                                  platform_info_.diffable_list_result_);

  BASE_STATIC_STRING_DECL(kNewarch, "newarch");
  lepus_platform_result->SetValue(kNewarch, platform_info_.new_arch_list_);

  {
    auto lepus_view_types = lepus::CArray::Create();
    lepus_view_types->reserve(platform_info_.components_.size());
    for (const auto& cur : platform_info_.components_) {
      lepus_view_types->emplace_back(cur);
    }
    BASE_STATIC_STRING_DECL(kViewTypes, "viewTypes");
    lepus_platform_result->SetValue(kViewTypes, std::move(lepus_view_types));
  }

  {
    auto lepus_full_spans = lepus::CArray::Create();
    lepus_full_spans->reserve(platform_info_.fullspan_.size());
    for (auto cur : platform_info_.fullspan_) {
      lepus_full_spans->emplace_back(static_cast<int32_t>(cur));
    }
    BASE_STATIC_STRING_DECL(kFullspan, "fullspan");
    lepus_platform_result->SetValue(kFullspan, std::move(lepus_full_spans));
  }

  {
    auto lepus_item_keys = lepus::CArray::Create();
    lepus_item_keys->reserve(platform_info_.item_keys_.size());
    for (const auto& cur : platform_info_.item_keys_) {
      lepus_item_keys->emplace_back(cur);
    }
    BASE_STATIC_STRING_DECL(kItemkeys, "itemkeys");
    lepus_platform_result->SetValue(kItemkeys, std::move(lepus_item_keys));
  }

  {
    auto lepus_stick_tops = lepus::CArray::Create();
    lepus_stick_tops->reserve(platform_info_.stick_top_items_.size());
    for (auto cur : platform_info_.stick_top_items_) {
      lepus_stick_tops->emplace_back(static_cast<int32_t>(cur));
    }
    BASE_STATIC_STRING_DECL(kStickyTop, "stickyTop");
    lepus_platform_result->SetValue(kStickyTop, std::move(lepus_stick_tops));
  }

  {
    auto lepus_stick_bottoms = lepus::CArray::Create();
    lepus_stick_bottoms->reserve(platform_info_.stick_bottom_items_.size());
    for (auto cur : platform_info_.stick_bottom_items_) {
      lepus_stick_bottoms->emplace_back(static_cast<int32_t>(cur));
    }
    BASE_STATIC_STRING_DECL(kStickyBottom, "stickyBottom");
    lepus_platform_result->SetValue(kStickyBottom,
                                    std::move(lepus_stick_bottoms));
  }

  {
    auto lepus_estimated_height = lepus::CArray::Create();
    lepus_estimated_height->reserve(platform_info_.estimated_heights_.size());
    for (auto cur : platform_info_.estimated_heights_) {
      lepus_estimated_height->emplace_back(static_cast<int32_t>(cur));
    }
    BASE_STATIC_STRING_DECL(kEstimatedHeight, "estimatedHeight");
    lepus_platform_result->SetValue(kEstimatedHeight,
                                    std::move(lepus_estimated_height));
  }

  {
    auto lepus_estimated_height_px = lepus::CArray::Create();
    lepus_estimated_height_px->reserve(
        platform_info_.estimated_heights_px_.size());
    for (auto cur : platform_info_.estimated_heights_px_) {
      lepus_estimated_height_px->emplace_back(static_cast<int32_t>(cur));
    }
    BASE_STATIC_STRING_DECL(kEstimatedHeightPx, "estimatedHeightPx");
    lepus_platform_result->SetValue(kEstimatedHeightPx,
                                    std::move(lepus_estimated_height_px));
  }

  {
    auto lepus_estimated_main_axis_size_px = lepus::CArray::Create();
    lepus_estimated_main_axis_size_px->reserve(
        platform_info_.estimated_main_axis_size_px_.size());
    for (auto cur : platform_info_.estimated_main_axis_size_px_) {
      lepus_estimated_main_axis_size_px->emplace_back(
          static_cast<int32_t>(cur));
    }
    BASE_STATIC_STRING_DECL(kEstimatedMainAxisSizePx,
                            "estimatedMainAxisSizePx");
    lepus_platform_result->SetValue(
        kEstimatedMainAxisSizePx, std::move(lepus_estimated_main_axis_size_px));
  }

  BASE_STATIC_STRING_DECL(kDiffResult, "diffResult");
  if (list_updated) {
    auto lepus_diff_insertions = lepus::CArray::Create();
    lepus_diff_insertions->reserve(
        platform_info_.update_actions_.insertions_.size());
    for (const auto& cur : platform_info_.update_actions_.insertions_) {
      lepus_diff_insertions->emplace_back(cur);
    }

    auto lepus_diff_removals_ = lepus::CArray::Create();
    lepus_diff_removals_->reserve(
        platform_info_.update_actions_.removals_.size());
    for (const auto& cur : platform_info_.update_actions_.removals_) {
      lepus_diff_removals_->emplace_back(cur);
    }

    auto lepus_diff_update_from = lepus::CArray::Create();
    lepus_diff_update_from->reserve(
        platform_info_.update_actions_.update_from_.size());
    for (const auto& cur : platform_info_.update_actions_.update_from_) {
      lepus_diff_update_from->emplace_back(cur);
    }

    auto lepus_diff_update_to = lepus::CArray::Create();
    lepus_diff_update_to->reserve(
        platform_info_.update_actions_.update_to_.size());
    for (const auto& cur : platform_info_.update_actions_.update_to_) {
      lepus_diff_update_to->emplace_back(cur);
    }

    auto lepus_diff_move_from = lepus::CArray::Create();
    lepus_diff_move_from->reserve(
        platform_info_.update_actions_.move_from_.size());
    for (const auto& cur : platform_info_.update_actions_.move_from_) {
      lepus_diff_move_from->emplace_back(cur);
    }

    auto lepus_diff_move_to = lepus::CArray::Create();
    lepus_diff_move_to->reserve(platform_info_.update_actions_.move_to_.size());
    for (const auto& cur : platform_info_.update_actions_.move_to_) {
      lepus_diff_move_to->emplace_back(cur);
    }

    auto lepus_diff_result = lepus::Dictionary::Create();

    BASE_STATIC_STRING_DECL(kInsertions, "insertions");
    BASE_STATIC_STRING_DECL(kRemovals, "removals");
    BASE_STATIC_STRING_DECL(kUpdateFrom, "updateFrom");
    BASE_STATIC_STRING_DECL(kUpdateTo, "updateTo");
    BASE_STATIC_STRING_DECL(kMoveFrom, "moveFrom");
    BASE_STATIC_STRING_DECL(kMoveTo, "moveTo");
    lepus_diff_result->SetValue(kInsertions, std::move(lepus_diff_insertions));
    lepus_diff_result->SetValue(kRemovals, std::move(lepus_diff_removals_));
    lepus_diff_result->SetValue(kUpdateFrom, std::move(lepus_diff_update_from));
    lepus_diff_result->SetValue(kUpdateTo, std::move(lepus_diff_update_to));
    lepus_diff_result->SetValue(kMoveFrom, std::move(lepus_diff_move_from));
    lepus_diff_result->SetValue(kMoveTo, std::move(lepus_diff_move_to));
    lepus_platform_result->SetValue(kDiffResult, std::move(lepus_diff_result));
  } else {
    lepus_platform_result->SetValue(kDiffResult, lepus::Dictionary::Create());
  }

  BASE_STATIC_STRING_DECL(kListPlatformInfo, "list-platform-info");
  element()->SetAttribute(
      kListPlatformInfo, lepus::Value(std::move(lepus_platform_result)), false);
}

void RadonDiffListNode2::RadonDiffChildren(
    const std::unique_ptr<RadonBase>& old_radon_child,
    const DispatchOption& option) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonDiffListNode::RadonDiffChildren",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (!old_radon_child || old_radon_child->NodeType() != kRadonListNode) {
    return;
  }
  //  when the vertical list has  same children which are the horizontal list.
  // if the child list A reuse list B. should erase the old list B's child which
  // is null_ptr to keep the right data.
  auto it = old_radon_child->radon_children_.begin();
  while (it != old_radon_child->radon_children_.end()) {
    auto* component = static_cast<RadonComponent*>(it->get());
    if (!component->list_need_remove()) {
      // only add useful component
      AddChild(std::move(*it));
      it = old_radon_child->radon_children_.erase(it);
    } else {
      it++;
    }
  }

  NeedModifySubTreeComponent(component());
  TransmitDispatchOptionFromListNodeToListComponent(option);
}

void RadonDiffListNode2::TransmitDispatchOptionFromListNodeToListComponent(
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

void RadonDiffListNode2::DispatchFirstTime() {
  platform_info_.diffable_list_result_ = false;
  bool list_updated = DiffListComponents();
  SetupListInfo(list_updated);
  RadonNode::DispatchFirstTime();
  element()->PropsUpdateFinish();
}

int32_t RadonDiffListNode2::ComponentAtIndex(uint32_t index,
                                             int64_t operationId,
                                             bool enable_reuse_notification) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonDiffListNode2::ComponentAtIndex");
  if (index >= components_.size()) {
    LOGE("index out of range in RadonDiffListNode2::ComponentAtIndex.");
    return 0;
  }
  int32_t instance_id = tasm_ == nullptr ? tasm::report::kUnknownInstanceId
                                         : tasm_->GetInstanceId();
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      instance_id, tasm::timing::kListNodeTask,
      tasm::timing::kTaskNameRadonDiffListNode2ComponentAtIndex);
  // try to get reuse_identifier and item_key.
  ListComponentInfo& component_info = *components_[index];
  const auto& reuse_identifier = base::String{component_info.name_};
  auto item_key = component_info.diff_key_.String();
  bool component_is_newly_created{false};
  auto* component = reuse_pool_->GetComponentFromListKeyComponentMap(item_key);

  if (!component || !component->name().IsEqual(component_info.name_)) {
    // if component is null or component.name is not the same with
    // component_info, the component need to be created.
    component = CreateComponentWithType(index);
    RadonListBase::SyncComponentExtraInfo(component, index, operationId);
    reuse_pool_->InsertIntoListKeyComponentMap(
        component_info.diff_key_.String(), component);
    component_is_newly_created = true;
  }

  if (!component) {
    LOGE("Component is nullptr in ComponentAtIndex of list new arch.");
    return 0;
  }

  auto reuse_action =
      reuse_pool_->Dequeue(item_key, reuse_identifier, component);
  tasm_->page_proxy()->InsertEmptyComponent(component);

  PipelineOptions pipeline_options;
  static bool enable_report =
      LynxEnv::GetInstance().EnableReportListItemLifeStatistic();
  pipeline_options.enable_report_list_item_life_statistic_ = enable_report;
  if (reuse_action.type_ == ListReusePool::Action::Type::UPDATE) {
    LOGI("UPDATE key: " << item_key.str() << " , index: " << index);
    if (enable_report) {
      pipeline_options.list_item_life_option_.start_update_time_ =
          base::CurrentTimeMicroseconds();
    }
    SyncComponentExtraInfo(component, index, operationId);
    if (enable_report) {
      pipeline_options.list_item_life_option_.end_update_time_ =
          base::CurrentTimeMicroseconds();
    }
  } else {
    if (enable_report) {
      pipeline_options.list_item_life_option_.start_render_time_ =
          base::CurrentTimeMicroseconds();
    }
    RadonListBase::SyncComponentExtraInfo(component, index, operationId);
    bool ignore_component_lifecycle = false;
    // Check whether the component is newly created.
    if (component_is_newly_created) {
      // use component info's data and property to render new component.
      // After render, the component tree structure should be complete and
      // determined.
      UpdateAndRenderNewComponent(component, component_info.properties_,
                                  component_info.data_);
      // if the component is new created, should call component's lifecycle
      // later.
      ignore_component_lifecycle = false;
    } else {
      component->ResetElementRecursively();
      // diff old component with component info, but not handle element.
      // After diff, the component tree structure should be complete and
      // determined.
      UpdateOldComponent(component, component_info);
      // if the component has been updated, shouldn't call component's lifecycle
      // later.
      ignore_component_lifecycle = true;
    }

    if (enable_report) {
      uint64_t time = base::CurrentTimeMicroseconds();
      pipeline_options.list_item_life_option_.end_render_time_ = time;
      pipeline_options.list_item_life_option_.start_dispatch_time_ = time;
    }

    DispatchOption dispatch_option(page_proxy_);
    dispatch_option.ignore_component_lifecycle_ = ignore_component_lifecycle;
    switch (reuse_action.type_) {
      case ListReusePool::Action::Type::CREATE: {
        LOGI("CREATE key: " << item_key.str() << " , index: " << index);
        component->ResetElementRecursively();
        component->Dispatch(dispatch_option);
        break;
      }
      case ListReusePool::Action::Type::REUSE: {
        const auto& from_item_key = reuse_action.key_to_reuse_;
        LOGI("REUSE from key: " << from_item_key.str() << " to key: "
                                << item_key.str() << " , index: " << index);
        auto* reuse =
            reuse_pool_->GetComponentFromListKeyComponentMap(from_item_key);
        if (!reuse) {
          LOGE(
              "REUSE component doesn't exist, key is: " << from_item_key.str());
          break;
        }

        if (component->ComponentId() == 0) {
          component->GenerateAndSetComponentId();
        }

        auto* reuser = component;
        dispatch_option.only_swap_element_ = true;
        std::unique_ptr<RadonBase> fake_unique_reuse{reuse};

        if (enable_reuse_notification) {
          // reuser will reuse the element from fake_unique_reuse,
          // pass through the element's impl_id and reuser's item_key to
          // platform, so that the Native UI can be notified that it will be
          // reused
          auto* element = fake_unique_reuse->element();
          if (element) {
            page_proxy_->element_manager()
                ->painting_context()
                ->ListReusePaintingNode(element->impl_id(), item_key);
          }
        }

        reuser->SwapElement(fake_unique_reuse, dispatch_option);
        reuser->RadonDiffChildren(fake_unique_reuse, dispatch_option);
        // Should reset the  whole element structure
        reuse->ResetElementRecursively();
        fake_unique_reuse.release();

        // remove outdated component after being reused
        if (reuse->list_need_remove_after_reused()) {
          // remove from reuse_pool
          reuse_pool_->Remove(from_item_key, reuse_identifier);
          // remove from parent
          auto* parent = reuse->Parent();
          if (parent != nullptr) {
            // dtor its radon subtree in post order
            reuse->ClearChildrenRecursivelyInPostOrder();
            // remove it from its parent
            parent->RemoveChild(reuse);
          }
        }
        break;
      }
      default: {
        break;
      }
    }

    if (enable_report) {
      pipeline_options.list_item_life_option_.end_dispatch_time_ =
          base::CurrentTimeMicroseconds();
    }
  }
  component->SetListItemKey(item_key);
  pipeline_options.operation_id = operationId;
  pipeline_options.list_comp_id_ = component->element()->impl_id();
  pipeline_options.list_id_ =
      DisablePlatformImplementation() ? element()->impl_id() : 0;
  // TODO(kechenglong): SetNeedsLayout if and only if needed.
  page_proxy_->element_manager()->SetNeedsLayout();
  page_proxy_->element_manager()->OnPatchFinish(pipeline_options);
  if (!DisablePlatformImplementation()) {
    page_proxy_->element_manager()->painting_context()->FlushImmediately();
  }
  component_info.list_component_dispatch_option_.reset();
  return component->element()->impl_id();
}

void RadonDiffListNode2::EnqueueComponent(int32_t sign) {
  // EnqueueComponent is a public API which might be called without care
  // Rigorous checks must be done to avoid crash.

  if (!tasm_ || !tasm_->page_proxy() ||
      !tasm_->page_proxy()->element_manager() ||
      !tasm_->page_proxy()->element_manager()->node_manager()) {
    return;
  }
  auto* element =
      tasm_->page_proxy()->element_manager()->node_manager()->Get(sign);
  if (!element) {
    return;
  }
  auto* component =
      static_cast<RadonComponent*>(element->data_model()->radon_node_ptr());
  if (!component) {
    return;
  }

  LOGI("EnqueueComponent component, component name: "
       << component->name().str()
       << ", component item_key_: " << component->GetListItemKey().str());
  tasm_->page_proxy()->EraseFromEmptyComponentMap(component);
  reuse_pool_->Enqueue(component->GetListItemKey(), component->name());
}

// helper function; It's essentially a wrapper of UpdateRadonComponent().
void UpdateRadonComponentWithInitialData(RadonComponent* comp,
                                         const lepus::Value& props,
                                         DispatchOption& option) {
  option.need_create_js_counterpart_ = true;
  option.use_new_component_data_ = true;
  option.refresh_lifecycle_ = true;
  PipelineOptions pipeline_options;
  comp->UpdateRadonComponent(RadonComponent::RenderType::UpdateByNativeList,
                             props, comp->GetInitialData(), option,
                             pipeline_options);
}

void RadonDiffListNode2::SyncComponentExtraInfo(RadonComponent* comp,
                                                uint32_t index,
                                                int64_t operation_id) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "RadonDiffListNode2::SyncComponentExtraInfo",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  std::unique_ptr<RadonComponent> original_component_node;
  PtrLookupMap lookup_map;
  original_component_node = std::make_unique<RadonComponent>(*comp, lookup_map);
  RadonListBase::SyncComponentExtraInfo(comp, index, operation_id);
  auto& comp_info = components_[index];
  const lepus::Value& props = comp_info->properties_;
  DispatchOption dispatch_option(page_proxy_);

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
  bool should_flush =
      comp->ShouldFlush(std::move(original_component_node), dispatch_option);
  if (should_flush) {
    comp->element()->FlushProps();
  }
  if (comp->need_reset_data()) {
    UpdateRadonComponentWithInitialData(comp, props, dispatch_option);
    comp->set_need_reset_data(false);
    return;
  }
  PipelineOptions pipeline_options;
  comp->UpdateRadonComponent(RadonComponent::RenderType::UpdateByNativeList,
                             props, comp_info->data_, dispatch_option,
                             pipeline_options);
}

void RadonDiffListNode2::UpdateAndRenderNewComponent(
    RadonComponent* component, const lepus::Value& incoming_property,
    const lepus::Value& incoming_data) {
  auto config = tasm_->page_proxy()->GetConfig();
  component->UpdateSystemInfo(GenerateSystemInfo(&config));
  component->UpdateRadonComponentWithoutDispatch(
      RadonComponent::RenderType::FirstRender, incoming_property,
      incoming_data);
  RenderOption render_option;
  render_option.recursively = true;
  component->RenderRadonComponentIfNeeded(render_option);
}

void RadonDiffListNode2::UpdateOldComponent(RadonComponent* component,
                                            ListComponentInfo& component_info) {
  DispatchOption dispatch_update_option(page_proxy_);
  dispatch_update_option.need_update_element_ = false;
  dispatch_update_option.force_diff_entire_tree_ =
      component_info.list_component_dispatch_option_.force_diff_entire_tree_;
  dispatch_update_option.css_variable_changed_ =
      component_info.list_component_dispatch_option_.css_variable_changed_;
  dispatch_update_option.global_properties_changed_ =
      component_info.list_component_dispatch_option_.global_properties_changed_;
  dispatch_update_option.use_new_component_data_ =
      component_info.list_component_dispatch_option_.use_new_component_data_;
  dispatch_update_option.refresh_lifecycle_ =
      component_info.list_component_dispatch_option_.refresh_lifecycle_;
  component_info.list_component_dispatch_option_.reset();
  if (component->need_reset_data()) {
    UpdateRadonComponentWithInitialData(component, component_info.properties_,
                                        dispatch_update_option);
    component->set_need_reset_data(false);
    return;
  }
  PipelineOptions pipeline_options;
  component->UpdateRadonComponent(
      RadonComponent::RenderType::UpdateByNativeList,
      component_info.properties_, component_info.data_, dispatch_update_option,
      pipeline_options);
}

void RadonDiffListNode2::CheckItemKeys(
    std::vector<std::unique_ptr<ListComponentInfo>>& components) {
  if (!tasm_) {
    return;
  }
  std::unordered_set<base::String> cache;

  for (auto& curr : components) {
    bool use_default_item_key = false;

    // check if the item-key is a string and has been specified for the
    // current component
    if (curr->diff_key_.IsString()) {
      auto diff_key_str = curr->diff_key_.String();
      if (diff_key_str.empty()) {
        use_default_item_key = true;
      } else {
        // check if another component shares a same item-key with the current
        // component.
        if (!cache.emplace(diff_key_str).second) {
          use_default_item_key = true;
        }
      }
    } else {
      use_default_item_key = true;
    }

    if (use_default_item_key) {
      kAnonymousItemCount++;
      curr->diff_key_ = lepus::Value((std::string(kListDefaultItemKeyPrefix) +
                                      std::to_string(kAnonymousItemCount)));
    }
  }
}

void RadonDiffListNode2::FilterComponents(
    std::vector<std::unique_ptr<ListComponentInfo>>& components,
    TemplateAssembler* tasm) {
  ListNode::FilterComponents(components, tasm);
  CheckItemKeys(components);
}

bool RadonDiffListNode2::DisablePlatformImplementation() {
  if (page_proxy_->element_manager()->GetEnableNativeListFromShell()) {
    return true;
  }
  if (!disable_platform_implementation_) {
    const auto& attrs = attributes();
    const auto& iterator = attrs.find(BASE_STATIC_STRING(list::kCustomLisName));
    if (iterator != attrs.end()) {
      disable_platform_implementation_ =
          iterator->second.String() == BASE_STATIC_STRING(list::kListContainer);
    } else if (page_proxy_->element_manager()
                   ->GetEnableNativeListFromPageConfig()) {
      disable_platform_implementation_ = true;
    } else {
      disable_platform_implementation_ = false;
    }
  }
  return *disable_platform_implementation_;
}

}  // namespace tasm
}  // namespace lynx
