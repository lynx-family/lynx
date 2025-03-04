// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/list_element.h"

#include <string>
#include <vector>

#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/vdom/radon/radon_list_base.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/ui_component/list/list_types.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/long_task_timing/long_task_monitor.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace tasm {

ListElement::ListElement(ElementManager* manager, const base::String& tag,
                         const lepus::Value& component_at_index,
                         const lepus::Value& enqueue_component,
                         const lepus::Value& component_at_indexes)
    : FiberElement(manager, tag),
      ListContainer(this),
      component_at_index_(component_at_index),
      enqueue_component_(enqueue_component),
      component_at_indexes_(component_at_indexes) {
  if (manager == nullptr) {
    return;
  }
  auto batch_render_strategy =
      ResolveBatchRenderStrategyFromPipelineSchedulerConfig(
          manager->GetConfig()->GetPipelineSchedulerConfig(),
          manager->GetEnableParallelElement());
  if (list_container_delegate()) {
    list_container_delegate()->UpdateBatchRenderStrategy(batch_render_strategy);
  }
}

ListNode* ListElement::GetListNode() {
  if (IsFiberArch()) {
    return static_cast<ListElement*>(this);
  }
  // For Radon-Fiber Arch, the ListNode should be RadonListBase.
  return static_cast<RadonListBase*>(data_model_->radon_node_ptr());
}

void ListElement::OnNodeAdded(FiberElement* child) {
  // List's child should not be flatten.
  child->set_config_flatten(false);
  // List's child should not be layout only.
  child->MarkCanBeLayoutOnly(false);
  // Mark list's child as list item
  child->MarkAsListItem();
  // Create scheduler for each list-item
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    child->CreateListItemScheduler(
        list_container_delegate()->GetBatchRenderStrategy());
  }
  // Mark inserted child as render_root of its subtree
  // TODO: Override UpdateRenderRootElementIfNecessary when list-item-element
  // concept is introduced.
  child->RecursivelyMarkRenderRootElement(child);
}

void ListElement::ParallelFlushAsRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListElement::ParallelFlushAsRoot");
  if (!element_manager()->GetEnableParallelElement()) {
    return;
  }
  auto batch_render_strategy =
      DisableListPlatformImplementation() && list_container_delegate()
          ? list_container_delegate()->GetBatchRenderStrategy()
          : list::BatchRenderStrategy::kDefault;
  if (batch_render_strategy !=
          list::BatchRenderStrategy::kAsyncResolveProperty &&
      batch_render_strategy !=
          list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree) {
    FiberElement::ParallelFlushAsRoot();
    return;
  }

  // step1:wait for the tasm worker queue to complete execution

  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "TasmTaskRunner::WaitForCompletion");
    element_manager()->GetTasmWorkerTaskRunner()->WaitForCompletion();
  }

  // step2:consume the reduce task of the list item after resloving
  // the props
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "AsyncResolveListElementProperty");
    auto& parallel_task_queue = element_manager()->ParallelTasks();
    while (!parallel_task_queue.empty()) {
      parallel_task_queue.front().get()->Run();
      parallel_task_queue.front().get()->GetFuture().get()();
      parallel_task_queue.pop_front();
    }
  }

  // step 3:consume the reduce task of the list item after resolving the element
  // tree
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, "AsyncResolveListElementTree");

    // TODO(@hujing.1) move ParallelResolveTreeTasks to the list_element
    auto& parallel_resolve_element_tree_task_queue =
        element_manager()->ParallelResolveTreeTasks();

    while (!parallel_resolve_element_tree_task_queue.empty()) {
      if (parallel_resolve_element_tree_task_queue.front()
              .get()
              ->GetFuture()
              .wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        parallel_resolve_element_tree_task_queue.front()
            .get()
            ->GetFuture()
            .get()();
        parallel_resolve_element_tree_task_queue.pop_front();
      } else if (parallel_resolve_element_tree_task_queue.back().get()->Run()) {
        parallel_resolve_element_tree_task_queue.back()
            .get()
            ->GetFuture()
            .get()();
        parallel_resolve_element_tree_task_queue.pop_back();
      } else {
        ParallelFlushReturn task =
            parallel_resolve_element_tree_task_queue.front()
                .get()
                ->GetFuture()
                .get();
        task();
        parallel_resolve_element_tree_task_queue.pop_front();
      }
    }
  }
}

int32_t ListElement::ComponentAtIndex(uint32_t index, int64_t operationId,
                                      bool enable_reuse_notification) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListElement::ComponentAtIndex",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  int32_t instance_id = tasm_ == nullptr ? tasm::report::kUnknownInstanceId
                                         : tasm_->GetInstanceId();
  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      instance_id, tasm::timing::kListNodeTask,
      tasm::timing::kTaskNameListElementComponentAtIndex);
  if (ssr_helper_) {
    // ComponentAtIndex is an interface for the list to create list items.
    // In SSR, list items have already been created on the server, so just need
    // to add item elements to the list element.
    return ssr_helper_->ComponentAtIndexInSSR(index, operationId);
  }

  lepus::Value value =
      tasm_->context(tasm::DEFAULT_ENTRY_NAME)
          ->CallClosure(component_at_index_,
                        lepus::Value(fml::RefPtr<ListElement>(this)),
                        lepus::Value(impl_id()), lepus::Value(index),
                        lepus::Value(operationId),
                        lepus::Value(enable_reuse_notification));

  return static_cast<int32_t>(value.Number());
}

void ListElement::ComponentAtIndexes(
    const fml::RefPtr<lepus::CArray>& index_array,
    const fml::RefPtr<lepus::CArray>& operation_id_array,
    bool enable_reuse_notification /* = false */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListElement::ComponentAtIndexes",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  // Note: here we need to check if component_at_indexes_ is callable to ensure
  // the compatibility with lower versions of the front-end framework.
  if (!component_at_indexes_.IsCallable()) {
    return;
  }
  const size_t index_size = index_array->size();
  const size_t operation_id_size = operation_id_array->size();
  if (!index_size || !operation_id_size || index_size != operation_id_size) {
    return;
  }
  bool async_resolve = NeedAsyncResolveListItem();

  tasm_->context(tasm::DEFAULT_ENTRY_NAME)
      ->CallClosure(
          component_at_indexes_, lepus::Value(fml::RefPtr<ListElement>(this)),
          lepus::Value(impl_id()), lepus::Value(std::move(index_array)),
          lepus::Value(std::move(operation_id_array)),
          lepus::Value(enable_reuse_notification), lepus::Value(async_resolve));
}

void ListElement::EnqueueComponent(int32_t sign) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListElement::EnqueueComponent",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (ssr_helper_) {
    return;
  }

  tasm_->context(tasm::DEFAULT_ENTRY_NAME)
      ->CallClosure(enqueue_component_,
                    lepus::Value(fml::RefPtr<ListElement>(this)),
                    lepus::Value(impl_id()), lepus::Value(sign));
}

void ListElement::TickElement(fml::TimePoint& time) {
  if (list_container_delegate() && DisableListPlatformImplementation()) {
    list_container_delegate()->OnNextFrame();
  }
}

void ListElement::UpdateCallbacks(const lepus::Value& component_at_index,
                                  const lepus::Value& enqueue_component,
                                  const lepus::Value& component_at_indexes) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListElement::UpdateCallbacks",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  // remove ssr helper when js ready.
  if (ssr_helper_) {
    ssr_helper_ = std::nullopt;
  }
  component_at_index_ = component_at_index;
  enqueue_component_ = enqueue_component;
  component_at_indexes_ = component_at_indexes;
}

void ListElement::NotifyListReuseNode(const fml::RefPtr<FiberElement>& child,
                                      const base::String& item_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListElement::NotifyListReuseNode",
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  if (child) {
    element_manager_->painting_context()->ListReusePaintingNode(
        child->impl_id(), item_key);
  }
}

void ListElement::ResolveEnableNativeList() {
  // The priority is: shell(Case1) > property(Case2) > page config(Case3).
  if (element_manager_->GetEnableNativeListFromShell()) {
    // Case 1. Resolve enable native list from shell.
    disable_list_platform_implementation_ = true;
    return;
  }
  const auto& attr_map = updated_attr_map();
  // Case 2. Resolve "custom-list-name" property.
  auto it = attr_map.find(BASE_STATIC_STRING(list::kCustomLisName));
  if (it != attr_map.end()) {
    disable_list_platform_implementation_ =
        it->second.String() == BASE_STATIC_STRING(list::kListContainer);
    return;
  }
  // Case 3: Not set "custom-list-name" property, we get enable native list from
  // page config.
  disable_list_platform_implementation_ =
      element_manager_->GetEnableNativeListFromPageConfig();
}

void ListElement::ResolvePlatformNodeTag() {
  // When resolve platform node tag, we no need to consider whether enable
  // native list except the case that using page config. Case 1: Resolve
  // "custom-list-name" property.
  const auto& attr_map = updated_attr_map();
  auto it = attr_map.find(BASE_STATIC_STRING(list::kCustomLisName));
  if (it != attr_map.end()) {
    platform_node_tag_ = it->second.String();
    return;
  }
  // Case 2: If get enable native list from page config, we modify
  // platform_node_tag_ to "list-container".
  if (element_manager_->GetEnableNativeListFromPageConfig()) {
    platform_node_tag_ = BASE_STATIC_STRING(list::kListContainer);
  }
}

ParallelFlushReturn ListElement::PrepareForCreateOrUpdate() {
  const auto& attr_map = updated_attr_map();
  // Use optional to make sure only run once.
  if (AttrDirty() && !disable_list_platform_implementation_) {
    // Resolve whether to use native list.
    ResolveEnableNativeList();
    // Resolve platform node tag.
    ResolvePlatformNodeTag();
    if (*disable_list_platform_implementation_) {
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kListContainer,
                                lepus::Value(true));
      tasm::report::FeatureCounter::Instance()->Count(
          tasm::report::LynxFeature::CPP_ENABLE_NATIVE_LIST);
    }
  }
  // Handle experimental-batch-render-strategy property.
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    auto it = attr_map.find(
        BASE_STATIC_STRING(list::kExperimentalBatchRenderStrategy));
    if (it != attr_map.end()) {
      const int value = static_cast<int>(it->second.Number());
      if (value >= static_cast<int>(list::BatchRenderStrategy::kDefault) &&
          value <= static_cast<int>(list::BatchRenderStrategy::
                                        kAsyncResolvePropertyAndElementTree)) {
        list::BatchRenderStrategy batch_render_strategy_from_prop =
            static_cast<list::BatchRenderStrategy>(value);
        if (!element_manager()->GetEnableParallelElement() &&
            batch_render_strategy_from_prop !=
                list::BatchRenderStrategy::kDefault) {
          // If not enable parallel element, we should reset
          // batch_render_strategy_from_prop to batch render.
          batch_render_strategy_from_prop =
              list::BatchRenderStrategy::kBatchRender;
        }
        list_container_delegate()->UpdateBatchRenderStrategy(
            batch_render_strategy_from_prop);
      }
    }
    if (!batch_render_strategy_flushed_) {
      // Flush to platform ui once time.
      batch_render_strategy_flushed_ = true;
      list::BatchRenderStrategy batch_render_strategy =
          list_container_delegate()->GetBatchRenderStrategy();
      FiberElement::SetAttributeInternal(
          BASE_STATIC_STRING(list::kExperimentalBatchRenderStrategy),
          lepus::Value(static_cast<int>(batch_render_strategy)));
    }
  }
  return FiberElement::PrepareForCreateOrUpdate();
}

void ListElement::SetAttributeInternal(const base::String& key,
                                       const lepus::Value& value) {
  if (!DisableListPlatformImplementation() ||
      (DisableListPlatformImplementation() && list_container_delegate() &&
       list_container_delegate()->ResolveAttribute(key, value))) {
    FiberElement::SetAttributeInternal(key, value);
  } else if (list_container_delegate() &&
             (key.IsEqual(list::kFiberListDiffInfo) ||
              key.IsEqual(list::kListPlatformInfo))) {
    // fiber-list-info
    auto list_container_info = lepus::Dictionary::Create();
    list_container_delegate()->UpdateListContainerDataSource(
        list_container_info);
    FiberElement::SetAttributeInternal(
        BASE_STATIC_STRING(list::kListContainerInfo),
        lepus::Value(list_container_info));
  }

  if (key.IsEqual(kColumnCount) || key.IsEqual(kSpanCount)) {
    // layout node should use column-count to compute width.
    UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kColumnCount, value);
  }
  StyleMap attr_styles;
  if (key.IsEquals(kScrollOrientation) && value.IsString()) {
    const auto& value_str = value.StdString();
    if (value_str == kVertical) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(
              static_cast<int>(starlight::LinearOrientationType::kVertical)));
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                lepus::Value(true));
    } else if (value_str == kHorizontal) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(
              static_cast<int>(starlight::LinearOrientationType::kHorizontal)));
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                lepus::Value(true));
    }
  } else if (key.IsEquals(kVerticalOrientation)) {
    const auto& value_str = value.StdString();
    if (value_str == kTrue) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(
              static_cast<int>(starlight::LinearOrientationType::kVertical)));
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                lepus::Value(true));
    } else if (value_str == kFalse) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue::MakeEnum(
              static_cast<int>(starlight::LinearOrientationType::kHorizontal)));
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                lepus::Value(true));
    }
  }
  ConsumeStyle(attr_styles, nullptr);
}

void ListElement::PropsUpdateFinish() {
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    list_container_delegate()->PropsUpdateFinish();
  }
}

/**
 * @description: When the list element changes, this method will be invoked. For
 *example, if the list's width or height changes, or if the List itself has new
 *diff information.
 **/
void ListElement::OnListElementUpdated(const PipelineOptions& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListElement::OnListElementUpdated");
  if (DisableListPlatformImplementation() && list_container_delegate()) {
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
void ListElement::OnComponentFinished(Element* component,
                                      const PipelineOptions& option) {
  if (DisableListPlatformImplementation() && list_container_delegate() &&
      component && option.operation_id != 0) {
    list_container_delegate()->FinishBindItemHolder(component, option);
  }
}

void ListElement::OnListItemLayoutUpdated(Element* component) {
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    list_container_delegate()->OnListItemLayoutUpdated(component);
  }
}

void ListElement::OnListItemBatchFinished(const PipelineOptions& options) {
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    std::vector<Element*> list_items;
    for (const auto& list_item_id : options.list_item_ids_) {
      list_items.emplace_back(
          element_manager()->node_manager()->Get(list_item_id));
    }
    list_container_delegate()->FinishBindItemHolders(list_items, options);
  }
}

/**
 * @description: Send scroll distance to list element.
 * @param delta_x: scroll distance in horizontal direction.
 * @param delta_y: scroll distance in vertical direction.
 **/
void ListElement::ScrollByListContainer(float content_offset_x,
                                        float content_offset_y,
                                        float original_x, float original_y) {
  if (DisableListPlatformImplementation() && list_container_delegate()) {
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
void ListElement::ScrollToPosition(int index, float offset, int align,
                                   bool smooth) {
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    list_container_delegate()->ScrollToPosition(index, offset, align, smooth);
  }
}

/**
 * @description: Finish ScrollToPosition
 **/
void ListElement::ScrollStopped() {
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    list_container_delegate()->ScrollStopped();
  }
}

void ListElement::SetEventHandler(const base::String& name,
                                  EventHandler* handler) {
  Element::SetEventHandler(name, handler);
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    list_container_delegate()->AddEvent(name);
  }
}

void ListElement::ResetEventHandlers() {
  Element::ResetEventHandlers();
  if (DisableListPlatformImplementation() && list_container_delegate()) {
    list_container_delegate()->ClearEvents();
  }
}

void ListElement::ResolveStyleValue(CSSPropertyID id,
                                    const tasm::CSSValue& value,
                                    bool force_update) {
  FiberElement::ResolveStyleValue(id, value, force_update);
  if (DisableListPlatformImplementation() && list_container_delegate() &&
      (CSSPropertyID::kPropertyIDListMainAxisGap == id ||
       CSSPropertyID::kPropertyIDListCrossAxisGap == id)) {
    lepus::Value axis_gap_value = computed_css_style()->GetValue(id);
    list_container_delegate()->ResolveListAxisGap(id, axis_gap_value);
  }
}

void ListElement::Hydrate() {
  if (ssr_helper_) {
    ssr_helper_->HydrateListNode();
  }
}

void ListElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  FiberElement::AttachToElementManager(manager, style_manager, keep_element_id);
  auto batch_render_strategy =
      ResolveBatchRenderStrategyFromPipelineSchedulerConfig(
          manager->GetConfig()->GetPipelineSchedulerConfig(),
          manager->GetEnableParallelElement());
  if (list_container_delegate()) {
    list_container_delegate()->UpdateBatchRenderStrategy(batch_render_strategy);
  }
}

int32_t ListElementSSRHelper::ComponentAtIndexInSSR(uint32_t index,
                                                    int64_t operationId) {
  if (index >= ssr_element_.size() ||
      (!ssr_element_[index] && list_element_->children().size() <= index)) {
    DCHECK(false) << "SSR loaded list nodes exceed the node size range.";
    return 0;
  }

  // has rendered.
  if (!ssr_element_[index]) {
    return list_element_->children()[index]->impl_id();
  }

  auto* item_element = ssr_element_[index].get();
  auto impl_id = ssr_element_[index]->impl_id();
  auto* element_manager = list_element_->element_manager();

  list_element_->InsertNode(std::move(ssr_element_[index]));
  ssr_element_[index] = nullptr;

  tasm::PipelineOptions options;
  options.trigger_layout_ = true;
  options.operation_id = operationId;
  options.list_comp_id_ = impl_id;
  element_manager->OnPatchFinish(options, item_element);
  EXEC_EXPR_FOR_INSPECTOR(
      element_manager->FiberAttachToInspectorRecursively(item_element));

  return impl_id;
}

void ListElementSSRHelper::HydrateListNode() {
  for (auto& element : ssr_element_) {
    if (element) {
      list_element_->InsertNode(element);
      EXEC_EXPR_FOR_INSPECTOR(
          list_element_->element_manager()->FiberAttachToInspectorRecursively(
              element.get()));
    }
  }

  ssr_element_.clear();
}

list::BatchRenderStrategy
ListElement::ResolveBatchRenderStrategyFromPipelineSchedulerConfig(
    uint64_t pipeline_scheduler_config, bool enable_parallel_element) {
  bool enable_batch_render =
      (pipeline_scheduler_config & kEnableListBatchRenderMask) > 0;
  bool enable_batch_render_async_resolve_property =
      (pipeline_scheduler_config &
       kEnableListBatchRenderAsyncResolvePropertyMask) > 0;
  bool enable_batch_render_async_resolve_tree =
      (pipeline_scheduler_config & kEnableListBatchRenderAsyncResolveTreeMask) >
      0;
  if (!enable_parallel_element) {
    if (enable_batch_render) {
      return list::BatchRenderStrategy::kBatchRender;
    } else {
      return list::BatchRenderStrategy::kDefault;
    }
  }

  if (!enable_batch_render) {
    return list::BatchRenderStrategy::kDefault;
  }

  if (enable_batch_render_async_resolve_tree &&
      enable_batch_render_async_resolve_property) {
    return list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree;
  }

  if (enable_batch_render_async_resolve_property) {
    return list::BatchRenderStrategy::kAsyncResolveProperty;
  }

  return list::BatchRenderStrategy::kBatchRender;
}

}  // namespace tasm
}  // namespace lynx
