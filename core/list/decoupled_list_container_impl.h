// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_CONTAINER_IMPL_H_
#define CORE_LIST_DECOUPLED_LIST_CONTAINER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "core/list/decoupled_list_adapter.h"
#include "core/list/decoupled_list_children_helper.h"
#include "core/list/decoupled_list_event_manager.h"
#include "core/list/decoupled_list_layout_manager.h"
#include "core/list/decoupled_list_types.h"
#include "core/list/list_animation_manager.h"
#include "core/list/list_container_delegate.h"
#include "core/list/list_element_delegate.h"
#include "core/list/list_item_element_delegate.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace list {

class ListContainerImpl : public ContainerDelegate {
 public:
  ListContainerImpl(ElementDelegate* list_delegate,
                    const std::shared_ptr<pub::PubValueFactory>& value_factory);
  ~ListContainerImpl() override;

  // Implement ContainerDelegate
  void OnAttachToElementManager() override;
  bool ResolveAttribute(const pub::Value& key,
                        const pub::Value& value) override;
  void ResolveListAxisGap(tasm::CSSPropertyID id, float gap) override;
  void PropsUpdateFinish() override;
  void OnLayoutChildren(
      const std::shared_ptr<tasm::PipelineOptions>& options) override;
  void FinishBindItemHolder(
      ItemElementDelegate* list_item_delegate,
      const std::shared_ptr<tasm::PipelineOptions>& options) override;
  void FinishBindItemHolders(
      const std::vector<ItemElementDelegate*>& list_item_delegate_array,
      const std::shared_ptr<tasm::PipelineOptions>& options) override;
  void ScrollByPlatformContainer(float content_offset_x, float content_offset_y,
                                 float original_x, float original_y) override;
  void OnListItemLayoutUpdated(
      ItemElementDelegate* list_item_delegate) override;
  void OnAttachedToElementManager() override;
  void ScrollToPosition(int index, float offset, int align,
                        bool smooth) override;
  void ScrollStopped() override;
  void EnableInsertPlatformView() override {
    enable_insert_platform_view_operation_ = true;
  }
  void OnNextFrame() override;
  void SetEnableBatchRender(bool enable_batch_render) override {
    enable_batch_render_ = enable_batch_render;
  }

  int GetDataCount() const;
  ItemHolder* GetItemHolderForIndex(int index);
  void FlushPatching();
  void UpdateContentOffsetAndSizeToPlatform(float content_size,
                                            float target_content_offset_x,
                                            float target_content_offset_y,
                                            bool is_init_scroll_offset,
                                            bool from_layout);
  void UpdateScrollInfo(float estimated_offset, bool smooth, bool scrolling);
  void StartInterceptListElementUpdated();
  void StopInterceptListElementUpdated();
  float RoundValueToPixelGrid(const float value);
  void ClearValidDiff() { has_valid_diff_ = false; }
  void MarkShouldFlushFinishLayout(bool has_layout) {
    should_flush_finish_layout_ |= has_layout;
  }
  void ResetLayoutID() { layout_id_ = -1; }
  bool ShouldSearchRefAnchor() const {
    return search_ref_anchor_strategy_ == SearchRefAnchorStrategy::kToStart ||
           search_ref_anchor_strategy_ == SearchRefAnchorStrategy::kToEnd;
  }

  // Getter
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
  ListAnimationManager* list_animation_manager() const {
    return list_animation_manager_.get();
  }
  bool IsRTL() const { return list_delegate_->IsRTL(); }
  int intercept_depth() const { return intercept_depth_; }
  ElementDelegate* list_delegate() { return list_delegate_; }
  int layout_id() const { return layout_id_; }
  bool sticky_enabled() const { return sticky_enabled_; }
  bool recycle_sticky_item() const { return recycle_sticky_item_; }
  float sticky_offset() const { return sticky_offset_; }
  bool should_request_state_restore() const {
    return should_request_state_restore_;
  }
  bool enable_batch_render() const { return enable_batch_render_; }
  bool enable_insert_platform_view_operation() const {
    return enable_insert_platform_view_operation_;
  }
  bool has_valid_diff() const { return has_valid_diff_; }
  const std::shared_ptr<pub::PubValueFactory>& value_factory() {
    return value_factory_;
  }
  bool need_preload_section_on_next_frame() const {
    return need_preload_section_on_next_frame_;
  }
  bool should_flush_finish_layout() const {
    return should_flush_finish_layout_;
  }
  SearchRefAnchorStrategy search_ref_anchor_strategy() const {
    return search_ref_anchor_strategy_;
  }

 protected:
  // Currently, the list container does not copy any member variables and is an
  // empty implementation.
  ListContainerImpl(const ListContainerImpl& list_container_impl) = delete;

 private:
  void UpdateListLayoutManager(LayoutType layout_type);
  //  fml::RefPtr<lepus::CArray> GenerateVisibleItemInfo() const;

 private:
  bool enable_dynamic_span_count_{true};
  bool enable_insert_platform_view_operation_{false};
  bool span_count_changed_{false};
  bool batch_adapter_initialized_{false};
  bool recycle_available_item_before_layout_{false};
  bool sticky_enabled_{false};
  bool recycle_sticky_item_{true};
  SearchRefAnchorStrategy search_ref_anchor_strategy_{
      SearchRefAnchorStrategy::kNone};
  int sticky_buffer_count_{kInvalidItemCount};
  float sticky_offset_{0.f};
  int intercept_depth_{0};
  bool should_flush_finish_layout_{false};
  LayoutType layout_type_{LayoutType::kSingle};
  ElementDelegate* list_delegate_{nullptr};
  float physical_pixels_per_layout_unit_{1.f};
  int initial_scroll_index_{kInvalidIndex};
  InitialScrollIndexStatus initial_scroll_index_status_{
      InitialScrollIndexStatus::kUnset};
  std::unique_ptr<ListLayoutManager> list_layout_manager_;
  std::unique_ptr<ListAdapter> list_adapter_;
  std::unique_ptr<ListChildrenHelper> list_children_helper_;
  std::unique_ptr<ListEventManager> list_event_manager_;
  std::unique_ptr<ListAnimationManager> list_animation_manager_;
  bool need_recycle_all_item_holders_before_layout_{false};
  bool need_update_item_holders_{false};
  int layout_id_{kInvalidIndex};
  bool should_request_state_restore_{false};
  bool has_valid_diff_{false};
  bool update_animation_{false};
  bool need_preload_section_on_next_frame_{false};
  bool enable_batch_render_{false};
  ListAdapterDiffResult animation_diff_result_{ListAdapterDiffResult::kNone};
  std::shared_ptr<pub::PubValueFactory> value_factory_;
};

std::unique_ptr<ContainerDelegate> CreateListContainerDelegate(
    ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory);

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_CONTAINER_IMPL_H_
