// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_IMPL_H_
#define CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/renderer/dom/element.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/ui_component/list/list_adapter.h"
#include "core/renderer/ui_component/list/list_children_helper.h"
#include "core/renderer/ui_component/list/list_container.h"
#include "core/renderer/ui_component/list/list_event_manager.h"
#include "core/renderer/ui_component/list/list_layout_manager.h"
#include "core/renderer/ui_component/list/list_types.h"

namespace lynx {
namespace tasm {

class ListContainerImpl : public ListContainer::Delegate {
 public:
  ListContainerImpl(Element* element);
  ~ListContainerImpl() = default;

  bool ResolveAttribute(const base::String& key,
                        const lepus::Value& value) override;
  void FinishBindItemHolder(Element* component,
                            const PipelineOptions& option) override;
  void FinishBindItemHolders(const std::vector<Element*>& list_items,
                             const PipelineOptions& options) override;
  void OnLayoutChildren() override;
  void ScrollByPlatformContainer(float content_offset_x, float content_offset_y,
                                 float original_x, float original_y) override;
  void ScrollToPosition(int index, float offset, int align,
                        bool smooth) override;
  void ScrollStopped() override;
  void OnNextFrame() override;
  void PropsUpdateFinish() override;
  void OnListItemLayoutUpdated(Element* component) override;
  void UpdateListContainerDataSource(
      fml::RefPtr<lepus::Dictionary>& list_container_info) override;
  void AddEvent(const base::String& name) override;
  void ClearEvents() override;
  void ResolveListAxisGap(CSSPropertyID id, const lepus::Value& value) override;
  int GetDataCount() const;
  ItemHolder* GetItemHolderForIndex(int index);
  void FlushPatching();
  void UpdateContentOffsetAndSizeToPlatform(float content_size,
                                            float target_content_offset_x,
                                            float target_content_offset_y,
                                            bool is_init_scroll_offset);
  void UpdateScrollInfo(float estimated_offset, bool smooth, bool scrolling);
  void StartInterceptListElementUpdated();
  void StopInterceptListElementUpdated();
  ListAdapter* list_adapter() const { return list_adapter_.get(); }
  ListLayoutManager* list_layout_manager() const {
    return list_layout_manager_.get();
  }
  ListEventManager* list_event_manager() const {
    return list_event_manager_.get();
  }
  ListChildrenHelper* list_children_helper() const {
    return list_children_helper_.get();
  }
  bool IsRTL() const {
    return element_ &&
           (element_->Direction() == starlight::DirectionType::kRtl ||
            element_->Direction() == starlight::DirectionType::kLynxRtl);
  }
  int intercept_depth() const { return intercept_depth_; }
  Element* element() const { return element_; }
  ElementManager* element_manager() const {
    return element_ ? element_->element_manager() : nullptr;
  }
  ListLayoutManager* list_layout_manager() {
    return list_layout_manager_.get();
  }
  fml::RefPtr<lepus::Dictionary> layout_complete_info() {
    return layout_complete_info_;
  }
  void ClearLayoutCompleteInfo() {
    layout_complete_info_ = lepus::Dictionary::Create();
  }
  bool need_layout_complete_info() const { return need_layout_complete_info_; }
  int layout_id() const { return layout_id_; }
  bool sticky_enabled() const { return sticky_enabled_; }
  float sticky_offset() const { return sticky_offset_; }
  void ResetLayoutID() { layout_id_ = -1; }
  bool should_request_state_restore() const {
    return should_request_state_restore_;
  }
  bool ShouldGenerateDebugInfo(list::ListDebugInfoLevel targetLevel);
  void RecordVisibleItemIfNeeded(bool is_layout_before);
  bool has_valid_diff() const { return has_valid_diff_; }
  bool enable_batch_render() const {
    return list_option_.batch_render_strategy !=
           list::BatchRenderStrategy::kDefault;
  }
  void ClearValidDiff() { has_valid_diff_ = false; }
  void ReportListItemLifecycleStatistic(const PipelineOptions& option,
                                        const std::string& item_key);
  void CheckZIndex(Element* child) const;
  void SendDebugEvent(const fml::RefPtr<lepus::Dictionary>& detail) {
    list_event_manager_->SendDebugEvent(detail);
  }
  void MarkShouldFlushFinishLayout(bool has_layout) {
    should_flush_finish_layout_ = has_layout;
  }
  bool should_flush_finish_layout() const {
    return should_flush_finish_layout_;
  }

  void UpdateBatchRenderStrategy(list::BatchRenderStrategy strategy) override;

  list::BatchRenderStrategy GetBatchRenderStrategy() override;

 protected:
  // Currently, the list container does not copy any member variables and is an
  // empty implementation.
  ListContainerImpl(const ListContainerImpl& list_container_impl) {}
  void UpdateListLayoutManager(list::LayoutType layout_type);
  void RecycleAllChildren();
  fml::RefPtr<lepus::CArray> GenerateVisibleItemInfo() const;

 private:
  using BindingItemHolderMap = std::unordered_map<int64_t, ItemHolder*>;
  bool batch_adapter_initialized_{false};
  bool sticky_enabled_{false};
  float sticky_offset_{0.f};
  int intercept_depth_{0};
  bool should_flush_finish_layout_{false};
  list::LayoutType layout_type_{list::LayoutType::kSingle};
  Element* element_{nullptr};
  std::unique_ptr<ListLayoutManager> list_layout_manager_;
  std::unique_ptr<ListAdapter> list_adapter_;
  std::unique_ptr<ListChildrenHelper> list_children_helper_;
  std::unique_ptr<ListEventManager> list_event_manager_;
  BindingItemHolderMap binding_item_holder_map_;
  fml::RefPtr<lepus::Dictionary> layout_complete_info_;
  bool need_layout_complete_info_{false};
  bool need_recycle_all_item_holders_before_layout_{false};
  list::ListDebugInfoLevel debug_info_level_{
      list::ListDebugInfoLevel::kListDebugInfoLevelInfo};
  bool need_update_item_holders_{false};
  bool enable_preload_section_{false};
  int layout_id_{-1};
  bool should_request_state_restore_{false};
  bool has_valid_diff_{false};

  struct ListOption {
    list::BatchRenderStrategy batch_render_strategy{
        list::BatchRenderStrategy::kDefault};
  };

  ListOption list_option_;

 public:
  bool need_preload_section_on_next_frame_{false};
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UI_COMPONENT_LIST_LIST_CONTAINER_IMPL_H_
