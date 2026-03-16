// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/dom/fiber/list_element.h"

#include <string>
#include <vector>

#include "core/list/decoupled_list_types.h"
#include "core/renderer/dom/fragment/fragment.h"
#include "core/renderer/dom/fragment/list_fragment_behavior.h"
#include "core/renderer/dom/list_component_info.h"
#include "core/renderer/dom/vdom/radon/radon_list_base.h"
#include "core/renderer/template_assembler.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/ui_component/list/list_types.h"
#include "core/services/feature_count/feature_counter.h"
#include "core/services/long_task_timing/long_task_monitor.h"

namespace lynx {
namespace tasm {

ListElement::ListElement(ElementManager* manager, const base::String& tag,
                         const lepus::Value& component_at_index,
                         const lepus::Value& enqueue_component,
                         const lepus::Value& component_at_indexes)
    : FiberElement(manager, tag),
      component_at_index_(component_at_index),
      enqueue_component_(enqueue_component),
      component_at_indexes_(component_at_indexes) {
  LOGI("[List] ListElement::ListElement: this=" << this
                                                << ", impl_id=" << impl_id());
  if (manager == nullptr) {
    return;
  }
  batch_render_strategy_ =
      ResolveBatchRenderStrategyFromPipelineSchedulerConfig(
          manager->GetConfig()->GetPipelineSchedulerConfig(),
          manager->GetEnableParallelElement());
}

ListNode* ListElement::GetListNode() {
  if (IsFiberArch()) {
    return static_cast<ListElement*>(this);
  }
  // For Radon-Fiber Arch, the ListNode should be RadonListBase.
  return static_cast<RadonListBase*>(data_model_->radon_node_ptr());
}

bool ListElement::NeedAsyncResolveListItem() {
  auto batch_render_strategy = (UseDecoupledList() || UseInternalList())
                                   ? batch_render_strategy_
                                   : list::BatchRenderStrategy::kDefault;
  return batch_render_strategy ==
             list::BatchRenderStrategy::kAsyncResolveProperty ||
         batch_render_strategy ==
             list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree;
}

void ListElement::OnNodeAdded(FiberElement* child) {
  // List's child should not be flatten.
  child->set_config_flatten(false);
  // List's child should not be layout only.
  child->MarkCanBeLayoutOnly(false);
  // Mark list's child as list item
  child->MarkAsListItem();
  // Create scheduler for each list-item
  if (NeedAsyncResolveListItem()) {
    child->CreateListItemScheduler(batch_render_strategy_,
                                   element_context_delegate_,
                                   continuous_resolve_tree_);
    // Mark inserted child as render_root of its subtree
    // TODO: Override UpdateRenderRootElementIfNecessary when list-item-element
    // concept is introduced.
    child->RecursivelyMarkRenderRootElement(child);
  }
}

void ListElement::ParallelFlushAsRoot() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_PARALLEL_FLUSH_AS_ROOT);
  if (!element_manager()->GetEnableParallelElement()) {
    return;
  }
  if (!NeedAsyncResolveListItem()) {
    FiberElement::ParallelFlushAsRoot();
    return;
  }

  // step1:wait for the tasm worker queue to complete execution

  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, TASM_TASK_RUNNER_WAIT_FOR_COMPLETION);
    element_manager()->GetTasmWorkerTaskRunner()->WaitForCompletion();
  }

  // step2:consume the reduce task of the list item after resloving
  // the props
  {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ASYNC_RESOLVE_PROPERTY);
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
    TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ASYNC_RESOLVE_TREE);

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

void ListElement::set_will_destroy(bool destroy) {
  Element::set_will_destroy(destroy);
  // FIXME(linxs): remove this flag soon
  if (destroy && element_manager_ &&
      element_manager_->FixListCallbackLeakFlag()) {
    component_at_index_ = lepus::Value();
    enqueue_component_ = lepus::Value();
    component_at_indexes_ = lepus::Value();
  }
}

int32_t ListElement::ComponentAtIndex(uint32_t index, int64_t operationId,
                                      bool enable_reuse_notification) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ELEMENT_RENDER_COMPONENT,
              [this, instance_id = tasm_->GetInstanceId()](
                  lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
                ctx.event()->add_debug_annotations(INSTANCE_ID,
                                                   std::to_string(instance_id));
              });

  tasm::timing::LongTaskMonitor::Scope longTaskScope(
      tasm_->GetPageOptions(), tasm::timing::kListNodeTask,
      tasm::timing::kTaskNameListElementComponentAtIndex);
  if (ssr_helper_ && !ssr_helper_->HasHydrate()) {
    // ComponentAtIndex is an interface for the list to create list items.
    // In SSR, list items have already been created on the server, so just need
    // to add item elements to the list element.
    return ssr_helper_->ComponentAtIndexInSSR(index, operationId);
  }
  if (element_manager_ && element_manager_->DisableListCallbackIfDetached() &&
      IsDetached()) {
    return list::kInvalidIndex;
  }
  std::vector<lepus::Value> args = {
      lepus::Value(fml::RefPtr<ListElement>(this)), lepus::Value(impl_id()),
      lepus::Value(index), lepus::Value(operationId),
      lepus::Value(enable_reuse_notification)};

  lepus::Value value = tasm_->CallLepusMethod(component_at_index_, args);

  return static_cast<int32_t>(value.Number());
}

void ListElement::ComponentAtIndexes(
    const fml::RefPtr<lepus::CArray>& index_array,
    const fml::RefPtr<lepus::CArray>& operation_id_array,
    bool enable_reuse_notification /* = false */) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_COMPONENT_AT_INDEXES,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  // Note: here we need to check if component_at_indexes_ is callable to ensure
  // the compatibility with lower versions of the front-end framework.
  if (!component_at_indexes_.IsCallable() ||
      (element_manager_ && element_manager_->DisableListCallbackIfDetached() &&
       IsDetached())) {
    return;
  }
  const size_t index_size = index_array->size();
  const size_t operation_id_size = operation_id_array->size();
  if (!index_size || !operation_id_size || index_size != operation_id_size) {
    return;
  }
  bool async_resolve = NeedAsyncResolveListItem();
  element_manager()->SetCSSFragmentParsingOnTASMWorkerMTSRender(async_resolve);

  std::vector<lepus::Value> args = {
      lepus::Value(fml::RefPtr<ListElement>(this)),
      lepus::Value(impl_id()),
      lepus::Value(std::move(index_array)),
      lepus::Value(std::move(operation_id_array)),
      lepus::Value(enable_reuse_notification),
      lepus::Value(async_resolve)};

  tasm_->CallLepusMethod(component_at_indexes_, args);

  element_manager()->SetCSSFragmentParsingOnTASMWorkerMTSRender(false);
}

void ListElement::EnqueueComponent(int32_t sign) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ENQUEUE_COMPONENT,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  if (ssr_helper_ && !ssr_helper_->HasHydrate()) {
    ssr_helper_->OnEnqueueComponent(sign);
    return;
  }
  if (element_manager_ && element_manager_->DisableListCallbackIfDetached() &&
      IsDetached()) {
    return;
  }
  std::vector<lepus::Value> args = {
      lepus::Value(fml::RefPtr<ListElement>(this)), lepus::Value(impl_id()),
      lepus::Value(sign)};
  tasm_->CallLepusMethod(enqueue_component_, args);
}

void ListElement::TickElement(fml::TimePoint& time) {
  if (UseDecoupledList()) {
    list_mediator_->OnNextFrame();
  } else if (UseInternalList()) {
    list_container_delegate_internal_->OnNextFrame();
  }
}

void ListElement::UpdateCallbacks(const lepus::Value& component_at_index,
                                  const lepus::Value& enqueue_component,
                                  const lepus::Value& component_at_indexes) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_UPDATE_CALLBACKS,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });
  component_at_index_.CopyWeakValue(component_at_index);
  enqueue_component_.CopyWeakValue(enqueue_component);
  component_at_indexes_.CopyWeakValue(component_at_indexes);
}

void ListElement::NotifyListReuseNode(const fml::RefPtr<FiberElement>& child,
                                      const base::String& item_key) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_NOTIFY_REUSE_NODE,
              [this](lynx::perfetto::EventContext ctx) {
                UpdateTraceDebugInfo(ctx.event());
              });

  if (child) {
    element_container()->ListReusePaintingNode(child->impl_id(), item_key);
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
  // native list except the case that using page config.

  // Case 1: Resolve "custom-list-name" property.
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

void ListElement::ResolveEnableDecoupledList() {
  if (!enable_decoupled_list_) {
    const auto& attr_map = updated_attr_map();
    auto it =
        attr_map.find(BASE_STATIC_STRING(lynx::list::kPropEnableDecoupledList));
    // Priority: attribute > settings.
    if (it != attr_map.end() && it->second.IsBool()) {
      enable_decoupled_list_ = it->second.Bool();
    } else {
      // Return true from env by default.
      enable_decoupled_list_ = LynxEnv::GetInstance().EnableDecoupledList();
    }
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
    // Resolve whether to use decoupled list.
    ResolveEnableDecoupledList();
    // Report feature count.
    HandleDelayTask([platform_node_tag = platform_node_tag_,
                     disable_list_platform_implementation =
                         *disable_list_platform_implementation_]() {
      // add feature count for cpp list
      if (disable_list_platform_implementation) {
        tasm::report::FeatureCounter::Instance()->Count(
            tasm::report::LynxFeature::CPP_ENABLE_NATIVE_LIST);
      }
      // add feature count for custom-list-name
      if (platform_node_tag.IsEqual(BASE_STATIC_STRING(list::kListContainer))) {
        // list-container
        tasm::report::FeatureCounter::Instance()->Count(
            tasm::report::LynxFeature::CPP_LIST_CONTAINER);
      } else if (!platform_node_tag.IsEqual(list::kList)) {
        // custom-list
        tasm::report::FeatureCounter::Instance()->Count(
            tasm::report::LynxFeature::CPP_CUSTOM_LIST);
      }
    });
    LOGI("[List] ListElement::PrepareForCreateOrUpdate: this="
         << this << ", impl_id=" << impl_id()
         << ", enable_native_list=" << *disable_list_platform_implementation_
         << ", enable_decoupled_list=" << *enable_decoupled_list_);
    if (*disable_list_platform_implementation_) {
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kListContainer,
                                lepus::Value(true));
      // Note: Because we move create ListMediator or ListContainerImpl in
      // resolving attr, so in ListMediator's constructor or ListContainerImpl's
      // constructor can get PhysicalPixelsPerLayoutUnit from element manager.
      if (*enable_decoupled_list_) {
        list_mediator_ = std::make_unique<ListMediator>(this);
      } else {
        list_container_delegate_internal_ =
            list::CreateListContainerDelegateInternal(this);
      }
    }
  }

  // Only handle experimental-batch-render-strategy property once time
  if (DisableListPlatformImplementation() && !batch_render_strategy_flushed_) {
    batch_render_strategy_flushed_ = true;
    // Get batch_render_strategy from prop.
    auto it = attr_map.find(
        BASE_STATIC_STRING(list::kExperimentalBatchRenderStrategy));
    if (it != attr_map.end()) {
      const int value = static_cast<int>(it->second.Number());
      if (value >= static_cast<int>(list::BatchRenderStrategy::kDefault) &&
          value <= static_cast<int>(list::BatchRenderStrategy::
                                        kAsyncResolvePropertyAndElementTree)) {
        batch_render_strategy_ = static_cast<list::BatchRenderStrategy>(value);
        if (!element_manager()->GetEnableParallelElement() &&
            batch_render_strategy_ != list::BatchRenderStrategy::kDefault) {
          // If not enable parallel element, we should reset
          // batch_render_strategy_from_prop to batch render.
          batch_render_strategy_ = list::BatchRenderStrategy::kBatchRender;
        }
      }
    }
    // Flush to platform ui and list container once time.
    bool enable_batch_render =
        batch_render_strategy_ > list::BatchRenderStrategy::kDefault;
    if (UseDecoupledList()) {
      list_mediator_->SetEnableBatchRender(enable_batch_render);
    } else if (UseInternalList()) {
      list_container_delegate_internal_->SetEnableBatchRender(
          enable_batch_render);
    }
    FiberElement::SetAttributeInternal(
        BASE_STATIC_STRING(list::kExperimentalBatchRenderStrategy),
        lepus::Value(static_cast<int>(batch_render_strategy_)));
    // Report batch render statistic.
    HandleDelayTask([this]() {
      std::string id_selector = data_model_->idSelector().str();
      bool enable_parallel_element =
          element_manager_->GetEnableParallelElement();
      list::BatchRenderStrategy batch_render_strategy = batch_render_strategy_;
      report::EventTracker::OnEvent(
          [batch_render_strategy, id_selector,
           enable_parallel_element](report::MoveOnlyEvent& event) {
            event.SetName("lynxsdk_list_batch_render_statistic");
            event.SetProps("id_selector", id_selector);
            event.SetProps("strategy", static_cast<int>(batch_render_strategy));
            event.SetProps("enable_parallel_element", enable_parallel_element);
          });
    });
  }
  // Handle experimental-continuous-resolve-tree
  auto it = attr_map.find(
      BASE_STATIC_STRING(list::kExperimentalContinuousResolveTree));
  if (it != attr_map.end() && it->second.IsBool()) {
    continuous_resolve_tree_ = it->second.Bool();
  }
  return FiberElement::PrepareForCreateOrUpdate();
}

void ListElement::SetAttributeInternal(const base::String& key,
                                       const lepus::Value& value) {
  if (!DisableListPlatformImplementation() ||
      (UseDecoupledList() && list_mediator_->ResolveAttribute(key, value)) ||
      (UseInternalList() &&
       list_container_delegate_internal_->ResolveAttribute(key, value))) {
    FiberElement::SetAttributeInternal(key, value);
  } else if (UseInternalList() && (key.IsEqual(list::kFiberListDiffInfo) ||
                                   key.IsEqual(list::kListPlatformInfo))) {
    // Note: Only use internal list, we need to create and generate
    // list_container_info here. If use decoupled list, the decoupled list will
    // flush list_container_info by using
    // lynx::list::ElementDelegate::FlushListContainerInfo() method.
    auto list_container_info = lepus::Dictionary::Create();
    list_container_delegate_internal_->UpdateListContainerDataSource(
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
          CSSValue(starlight::LinearOrientationType::kVertical));
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                lepus::Value(true));
    } else if (value_str == kHorizontal) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue(starlight::LinearOrientationType::kHorizontal));
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                lepus::Value(true));
    }
  } else if (key.IsEquals(kVerticalOrientation)) {
    const auto& value_str = value.StdString();
    if (value_str == kTrue) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue(starlight::LinearOrientationType::kVertical));
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                lepus::Value(true));
    } else if (value_str == kFalse) {
      attr_styles.insert_or_assign(
          kPropertyIDLinearOrientation,
          CSSValue(starlight::LinearOrientationType::kHorizontal));
      UpdateLayoutNodeAttribute(starlight::LayoutAttribute::kScroll,
                                lepus::Value(true));
    }
  }
  ConsumeStyle(attr_styles, nullptr);
}

void ListElement::PropsUpdateFinish() {
  if (UseDecoupledList()) {
    list_mediator_->PropsUpdateFinish();
  } else if (UseInternalList()) {
    list_container_delegate_internal_->PropsUpdateFinish();
  }
}

/**
 * @description: When the list element changes, this method will be invoked. For
 *example, if the list's width or height changes, or if the List itself has new
 *diff information.
 **/
void ListElement::OnListElementUpdated(
    const std::shared_ptr<PipelineOptions>& options) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_ON_ELEMENT_UPDATED);
  if (UseDecoupledList()) {
    list_mediator_->OnLayoutChildren(options);
  } else if (UseInternalList()) {
    list_container_delegate_internal_->OnLayoutChildren(options);
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
void ListElement::OnComponentFinished(
    Element* component, const std::shared_ptr<PipelineOptions>& option) {
  if (component && option->operation_id != 0) {
    if (UseDecoupledList()) {
      list_mediator_->FinishBindItemHolder(component, option);
    } else if (UseInternalList()) {
      list_container_delegate_internal_->FinishBindItemHolder(component,
                                                              option);
    }
  }
}

void ListElement::OnListItemLayoutUpdated(Element* component) {
  if (component) {
    if (UseDecoupledList()) {
      list_mediator_->OnListItemLayoutUpdated(component);
    } else if (UseInternalList()) {
      list_container_delegate_internal_->OnListItemLayoutUpdated(component);
    }
  }
}

void ListElement::OnListItemBatchFinished(
    const std::shared_ptr<PipelineOptions>& options) {
  std::vector<Element*> list_items;
  for (const auto& list_item_id : options->list_item_ids_) {
    list_items.emplace_back(
        element_manager()->node_manager()->Get(list_item_id));
  }
  if (UseDecoupledList()) {
    list_mediator_->FinishBindItemHolders(list_items, options);
  } else if (UseInternalList()) {
    list_container_delegate_internal_->FinishBindItemHolders(list_items,
                                                             options);
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
  if (UseDecoupledList()) {
    list_mediator_->ScrollByPlatformContainer(
        content_offset_x, content_offset_y, original_x, original_y);
  } else if (UseInternalList()) {
    list_container_delegate_internal_->ScrollByPlatformContainer(
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
  if (UseDecoupledList()) {
    list_mediator_->ScrollToPosition(index, offset, align, smooth);
  } else if (UseInternalList()) {
    list_container_delegate_internal_->ScrollToPosition(index, offset, align,
                                                        smooth);
  }
}

/**
 * @description: Finish ScrollToPosition
 **/
void ListElement::ScrollStopped() {
  if (UseDecoupledList()) {
    list_mediator_->ScrollStopped();
  } else if (UseInternalList()) {
    list_container_delegate_internal_->ScrollStopped();
  }
}

void ListElement::SetEventHandler(const base::String& name,
                                  EventHandler* handler) {
  Element::SetEventHandler(name, handler);
  if (UseInternalList()) {
    list_container_delegate_internal_->AddEvent(name);
  }
}

void ListElement::ResetEventHandlers() {
  Element::ResetEventHandlers();
  if (UseInternalList()) {
    list_container_delegate_internal_->ClearEvents();
  }
}

bool ListElement::ResolveStyleValue(CSSPropertyID id, const CSSValue& value) {
  bool ret = Element::ResolveStyleValue(id, value);
  switch (id) {
    case CSSPropertyID::kPropertyIDListMainAxisGap: {
      float main_axis_gap =
          computed_css_style()->GetLayoutComputedStyle()->GetListMainAxisGap();
      if (UseDecoupledList()) {
        list_mediator_->ResolveListAxisGap(id, main_axis_gap);
      } else if (UseInternalList()) {
        list_container_delegate_internal_->ResolveListAxisGap(id,
                                                              main_axis_gap);
      }
    } break;
    case CSSPropertyID::kPropertyIDListCrossAxisGap: {
      float cross_axis_gap =
          computed_css_style()->GetLayoutComputedStyle()->GetListCrossAxisGap();
      if (UseDecoupledList()) {
        list_mediator_->ResolveListAxisGap(id, cross_axis_gap);
      } else if (UseInternalList()) {
        list_container_delegate_internal_->ResolveListAxisGap(id,
                                                              cross_axis_gap);
      }
    } break;
    default:
      break;
  }
  return ret;
}

void ListElement::SetupFragmentBehavior(Fragment* fragment) {
  fragment->SetBehavior(std::make_unique<ListFragmentBehavior>(fragment));
}

void ListElement::AttachToElementManager(
    ElementManager* manager,
    const std::shared_ptr<CSSStyleSheetManager>& style_manager,
    bool keep_element_id) {
  FiberElement::AttachToElementManager(manager, style_manager, keep_element_id);
  batch_render_strategy_ =
      ResolveBatchRenderStrategyFromPipelineSchedulerConfig(
          manager->GetConfig()->GetPipelineSchedulerConfig(),
          manager->GetEnableParallelElement());
  if (UseDecoupledList()) {
    list_mediator_->OnAttachToElementManager();
  } else if (UseInternalList()) {
    list_container_delegate_internal_->OnAttachToElementManager(manager);
  }
}

void ListElement::UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                            const lepus::Value& value) {
  FiberElement::UpdateLayoutNodeAttribute(key, value.ToLepusValue());
}

void ListElement::Hydrate() {
  if (ssr_helper_ && !ssr_helper_->HasHydrate()) {
    ssr_helper_->HydrateListNode();
  }
}

void ListElement::HydrateFinish() {
  if (ssr_helper_) {
    ssr_helper_->OnListElementHydrateFinish();
    // remove ssr helper when hydrate finish.
    ssr_helper_ = std::nullopt;
  }
}

int32_t ListElementSSRHelper::ComponentAtIndexInSSR(uint32_t index,
                                                    int64_t operationId) {
  if (index >= ssr_elements_.size() || ssr_elements_[index].first == nullptr) {
    DCHECK(false) << "SSR loaded list nodes exceed the node size range.";
    return 0;
  }

  auto& [item, item_status] = ssr_elements_[index];
  if (item_status == SSRItemStatus::kWaitingRender) {
    list_element_->InsertNode(item);

    auto options = std::make_shared<PipelineOptions>();
    options->trigger_layout_ = true;
    options->operation_id = operationId;
    options->list_comp_id_ = item->impl_id();
    if (list_element_->DisableListPlatformImplementation()) {
      options->list_id_ = list_element_->impl_id();
    }
    auto* element_manager = list_element_->element_manager();
    element_manager->OnPatchFinish(options, item.get());
    EXEC_EXPR_FOR_INSPECTOR(
        element_manager->FiberAttachToInspectorRecursively(item.get()));
  }

  item_status = SSRItemStatus::kRendered;
  return item->impl_id();
}

void ListElementSSRHelper::HydrateListNode() {
  for (auto& [item, item_status] : ssr_elements_) {
    if (item && (item_status == SSRItemStatus::kWaitingRender)) {
      list_element_->InsertNode(item);
      EXEC_EXPR_FOR_INSPECTOR(
          list_element_->element_manager()->FiberAttachToInspectorRecursively(
              item.get()));
      item_status = SSRItemStatus::kEnqueued;
    }
  }

  has_hydrate_ = true;
}

void ListElementSSRHelper::OnListElementHydrateFinish() {
  for (uint32_t list_index = 0; list_index < ssr_elements_.size();
       ++list_index) {
    // If the status of the current list item is kRendered, the node is shown on
    // the screen.
    if (ssr_elements_[list_index].second == SSRItemStatus::kRendered) {
      // Call ComponentAtIndex to release the enqueue status of the Lepus node,
      // which was added during Lepus hydration.
      list_element_->ComponentAtIndex(list_index, -1, false);
    }
  }
}

void ListElementSSRHelper::OnEnqueueComponent(int32_t sign) {
  for (auto& itemInfo : ssr_elements_) {
    // Make the SSR element node be enqueued.
    if (itemInfo.first->impl_id() == sign) {
      itemInfo.second = SSRItemStatus::kEnqueued;
      break;
    }
  }
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

bool ListElement::UseDecoupledList() const {
  return DisableListPlatformImplementation() && enable_decoupled_list_ &&
         (*enable_decoupled_list_) && list_mediator_;
}

bool ListElement::UseInternalList() const {
  return DisableListPlatformImplementation() && enable_decoupled_list_ &&
         !(*enable_decoupled_list_) && list_container_delegate_internal_;
}

void ListElement::FlushListContainerInfo(const base::String& key,
                                         const lepus::Value& value) {
  FiberElement::SetAttributeInternal(key, value);
}

}  // namespace tasm
}  // namespace lynx
