// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_H_
#define CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "core/renderer/dom/element_manager.h"
#include "core/renderer/dom/fiber/fiber_element.h"
#include "core/renderer/ui_component/list/list_container_delegate_internal.h"
#include "core/renderer/ui_component/list/mediator/list_mediator.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/runtime/lepusng/jsvalue_helper.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class ListElement;

class ListElementSSRHelper {
 public:
  explicit ListElementSSRHelper(ListElement* list) : list_element_(list) {}

  // move only
  ListElementSSRHelper(const ListElementSSRHelper&) = delete;
  ListElementSSRHelper& operator=(const ListElementSSRHelper&) = delete;
  ListElementSSRHelper(ListElementSSRHelper&&) = default;
  ListElementSSRHelper& operator=(ListElementSSRHelper&&) = default;

  void OnEnqueueComponent(int32_t sign);
  void HydrateListNode();
  // on list element get callback function.
  void OnListElementHydrateFinish();
  int32_t ComponentAtIndexInSSR(uint32_t index, int64_t operationId);

  bool HasHydrate() { return has_hydrate_; }
  void AppendChild(fml::RefPtr<FiberElement> child) {
    ssr_elements_.push_back({child, SSRItemStatus::kWaitingRender});
  }

 private:
  enum class SSRItemStatus : uint32_t {
    kWaitingRender = 0,
    kRendered = 1,
    kEnqueued = 2,
  };

  bool has_hydrate_ = false;
  ListElement* list_element_;
  std::vector<std::pair<fml::RefPtr<FiberElement>, SSRItemStatus>>
      ssr_elements_;
};

class ListElement : public FiberElement, public tasm::ListNode {
 public:
  ListElement(ElementManager* manager, const base::String& tag,
              const lepus::Value& component_at_index,
              const lepus::Value& enqueue_component,
              const lepus::Value& component_at_indexes);

  fml::RefPtr<FiberElement> CloneElement(
      bool clone_resolved_props) const override {
    return fml::AdoptRef<FiberElement>(
        new ListElement(*this, clone_resolved_props));
  }
  void visitor(void* rt, void* func, uint64_t trace_tool) override {
    LEPUSRuntime* runtime = reinterpret_cast<LEPUSRuntime*>(rt);
    LEPUS_MarkFunc* mark_func = reinterpret_cast<LEPUS_MarkFunc*>(func);
    LEPUSValue v = WRAP_AS_JS_VALUE(component_at_index_.value());
    mark_func(runtime, v, trace_tool);
    v = WRAP_AS_JS_VALUE(component_at_indexes_.value());
    mark_func(runtime, v, trace_tool);
    v = WRAP_AS_JS_VALUE(enqueue_component_.value());
    mark_func(runtime, v, trace_tool);
    FiberElement::visitor(rt, reinterpret_cast<void*>(mark_func), trace_tool);
  }

  ~ListElement() override = default;

  virtual ListNode* GetListNode() override;

  void set_tasm(TemplateAssembler* tasm) { tasm_ = tasm; }

  bool is_list() const override { return true; }

  void TickElement(fml::TimePoint& time) override;
  void AppendComponentInfo(std::unique_ptr<ListComponentInfo> info) override {}
  void RemoveComponent(uint32_t sign) override {}
  void RenderComponentAtIndex(uint32_t row, int64_t operationId = 0) override {}
  void UpdateComponent(uint32_t sign, uint32_t row,
                       int64_t operationId = 0) override {}

  int32_t ComponentAtIndex(uint32_t index, int64_t operationId,
                           bool enable_reuse_notification) override;

  void ComponentAtIndexes(const fml::RefPtr<lepus::CArray>& index_array,
                          const fml::RefPtr<lepus::CArray>& operation_id_array,
                          bool enable_reuse_notification = false) override;

  void EnqueueComponent(int32_t sign) override;

  void UpdateCallbacks(const lepus::Value& component_at_index,
                       const lepus::Value& enqueue_component,
                       const lepus::Value& component_at_indexes);

  void NotifyListReuseNode(const fml::RefPtr<FiberElement>& child,
                           const base::String& item_key);

  void OnListItemBatchFinished(
      const std::shared_ptr<PipelineOptions>& options) override;

  // When the list element changes, this method will be invoked. For example, if
  // the list's width or height changes, or if the List itself has new diff
  // information.
  void OnListElementUpdated(
      const std::shared_ptr<PipelineOptions>& options) override;
  // When the rendering of the list's child node is complete, this method will
  // be invoked. In this method, we can obtain the correct layout information
  // of the child node.
  void OnComponentFinished(
      Element* component,
      const std::shared_ptr<PipelineOptions>& option) override;
  // Receive drag distance from platform list container.
  void ScrollByListContainer(float content_offset_x, float content_offset_y,
                             float original_x, float original_y) override;
  void ScrollToPosition(int index, float offset, int align,
                        bool smooth) override;
  void OnListItemLayoutUpdated(Element* component) override;
  void ScrollStopped() override;
  bool DisableListPlatformImplementation() const override {
    return disable_list_platform_implementation_
               ? *disable_list_platform_implementation_
               : false;
  }
  void SetEventHandler(const base::String& name,
                       EventHandler* handler) override;

  void ResetEventHandlers() override;

  ParallelFlushReturn PrepareForCreateOrUpdate() override;

  bool ResolveStyleValue(CSSPropertyID id, const CSSValue& value) override;

  void PropsUpdateFinish() override;

  virtual void ParallelFlushAsRoot() override;

  void SetSsrHelper(ListElementSSRHelper ssr_helper) {
    ssr_helper_ = std::move(ssr_helper);
  }

  void set_will_destroy(bool destroy) override;

  // ssr hydrate.
  void Hydrate();
  void HydrateFinish();

  void SetupFragmentBehavior(Fragment* fragment) override;

  virtual const base::String& GetPlatformNodeTag() const override {
    return platform_node_tag_;
  };

  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

  void UpdateLayoutNodeAttribute(starlight::LayoutAttribute key,
                                 const lepus::Value& value) override;

  void FlushListContainerInfo(const base::String& key,
                              const lepus::Value& value);

 protected:
  // Currently, the list element does not copy any member variables and is an
  // empty implementation.
  // TODO(WUJINTIAN): copy fiber list element
  ListElement(const ListElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props) {}

  void OnNodeAdded(FiberElement* child) override;
  void FilterComponents(
      std::vector<std::unique_ptr<ListComponentInfo>>& components,
      tasm::TemplateAssembler* tasm) override {}
  bool HasComponent(const std::string& component_name,
                    const std::string& current_entry) override {
    return false;
  }
  void SetAttributeInternal(const base::String& key,
                            const lepus::Value& value) override;

 private:
  void ResolveEnableNativeList();
  void ResolvePlatformNodeTag();
  void ResolveEnableDecoupledList();
  bool NeedAsyncResolveListItem();
  bool UseDecoupledList() const;
  bool UseInternalList() const;
  list::BatchRenderStrategy
  ResolveBatchRenderStrategyFromPipelineSchedulerConfig(
      uint64_t pipeline_scheduler_config, bool enable_parallel_element);

 private:
  bool continuous_resolve_tree_{false};
  tasm::TemplateAssembler* tasm_{nullptr};
  lepus::Value component_at_index_{};
  lepus::Value enqueue_component_{};
  lepus::Value component_at_indexes_{};
  std::optional<bool> disable_list_platform_implementation_;
  std::optional<bool> enable_decoupled_list_;
  base::String platform_node_tag_{BASE_STATIC_STRING(kListNodeTag)};
  std::optional<ListElementSSRHelper> ssr_helper_;
  bool batch_render_strategy_flushed_{false};
  std::unique_ptr<ListMediator> list_mediator_{nullptr};
  std::unique_ptr<ListContainerDelegateInternal>
      list_container_delegate_internal_{nullptr};
  list::BatchRenderStrategy batch_render_strategy_{
      list::BatchRenderStrategy::kDefault};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_H_
