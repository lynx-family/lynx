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
#include "core/renderer/ui_component/list/list_container.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"

namespace lynx {
namespace tasm {

class TemplateAssembler;
class ListElement;

using ListActions = std::vector<int32_t>;

class ListElementSSRHelper {
 public:
  explicit ListElementSSRHelper(ListElement* list) : list_element_(list) {}

  // move only
  ListElementSSRHelper(const ListElementSSRHelper&) = delete;
  ListElementSSRHelper& operator=(const ListElementSSRHelper&) = delete;
  ListElementSSRHelper(ListElementSSRHelper&&) = default;
  ListElementSSRHelper& operator=(ListElementSSRHelper&&) = default;

  void HydrateListNode();
  int32_t ComponentAtIndexInSSR(uint32_t index, int64_t operationId);
  void AppendChild(fml::RefPtr<FiberElement> child) {
    ssr_element_.emplace_back(child);
  }
  ListElement* GetListElement() { return list_element_; }

 private:
  ListElement* list_element_;
  std::vector<fml::RefPtr<FiberElement>> ssr_element_;
};

class ListElement : public FiberElement,
                    public ListContainer,
                    public tasm::ListNode {
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

  void OnListItemBatchFinished(const PipelineOptions& options) override;

  // When the list element changes, this method will be invoked. For example, if
  // the list's width or height changes, or if the List itself has new diff
  // information.
  void OnListElementUpdated(const PipelineOptions& options) override;
  // When the rendering of the list's child node is complete, this method will
  // be invoked. In this method, we can obtain the correct layout information
  // of the child node.
  void OnComponentFinished(Element* component,
                           const PipelineOptions& option) override;
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

  void ResolveStyleValue(CSSPropertyID id, const tasm::CSSValue& value,
                         bool force_update) override;
  void PropsUpdateFinish() override;

  virtual void ParallelFlushAsRoot() override;

  void SetSsrHelper(ListElementSSRHelper ssr_helper) {
    ssr_helper_ = std::move(ssr_helper);
  }

  // ssr hydrate.
  void Hydrate();

  virtual const base::String& GetPlatformNodeTag() const override {
    return platform_node_tag_;
  };

  void AttachToElementManager(
      ElementManager* manager,
      const std::shared_ptr<CSSStyleSheetManager>& style_manager,
      bool keep_element_id) override;

 protected:
  // Currently, the list element does not copy any member variables and is an
  // empty implementation.
  // TODO(WUJINTIAN): copy fiber list element
  ListElement(const ListElement& element, bool clone_resolved_props)
      : FiberElement(element, clone_resolved_props), ListContainer(element) {}

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
  bool NeedAsyncResolveListItem() {
    auto batch_render_strategy =
        list_container_delegate()->GetBatchRenderStrategy();
    return batch_render_strategy ==
               list::BatchRenderStrategy::kAsyncResolveProperty ||
           batch_render_strategy ==
               list::BatchRenderStrategy::kAsyncResolvePropertyAndElementTree;
  }

  list::BatchRenderStrategy
  ResolveBatchRenderStrategyFromPipelineSchedulerConfig(
      uint64_t pipeline_scheduler_config, bool enable_parallel_element);

  tasm::TemplateAssembler* tasm_{nullptr};
  lepus::Value component_at_index_{};
  lepus::Value enqueue_component_{};
  lepus::Value component_at_indexes_{};
  std::optional<bool> disable_list_platform_implementation_;
  base::String platform_node_tag_{BASE_STATIC_STRING(kListNodeTag)};
  std::optional<ListElementSSRHelper> ssr_helper_;
  bool batch_render_strategy_flushed_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DOM_FIBER_LIST_ELEMENT_H_
