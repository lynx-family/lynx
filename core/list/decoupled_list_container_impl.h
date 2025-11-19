// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_LIST_DECOUPLED_LIST_CONTAINER_IMPL_H_
#define CORE_LIST_DECOUPLED_LIST_CONTAINER_IMPL_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/list/decoupled_item_holder.h"
#include "core/list/decoupled_list_adapter.h"
#include "core/list/decoupled_list_children_helper.h"
#include "core/list/decoupled_list_event_manager.h"
#include "core/list/decoupled_list_layout_manager.h"
#include "core/list/decoupled_list_types.h"
#include "core/list/list_container_delegate.h"
#include "core/list/list_element_delegate.h"
#include "core/list/list_item_element_delegate.h"
#include "core/public/pub_value.h"

namespace lynx {
namespace list {

class ListContainerImpl : public list::ContainerDelegate {
 public:
  ListContainerImpl(list::ElementDelegate* list_delegate,
                    const std::shared_ptr<pub::PubValueFactory>& value_factory);
  ~ListContainerImpl() override;

  // Implement list::ContainerDelegate
  bool ResolveAttribute(const pub::Value& key,
                        const pub::Value& value) override;
  void ResolveListAxisGap(lynx::tasm::CSSPropertyID id, float gap) override;
  void PropsUpdateFinish() override;
  void OnLayoutChildren(
      const std::shared_ptr<tasm::PipelineOptions>& options) override;
  void FinishBindItemHolder(
      list::ItemElementDelegate* list_item_delegate,
      const std::shared_ptr<tasm::PipelineOptions>& option) override;
  void ScrollByPlatformContainer(float content_offset_x, float content_offset_y,
                                 float original_x, float original_y) override;
  void OnListItemLayoutUpdated(
      list::ItemElementDelegate* list_item_delegate) override;
  void OnAttachedToElementManager() override;
  void ScrollToPosition(int index, float offset, int align,
                        bool smooth) override;
  void ScrollStopped() override;
  void EnableInsertPlatformView() override {
    enable_insert_platform_view_ = true;
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
  bool IsRTL() const { return list_delegate_->IsRTL(); }
  int intercept_depth() const { return intercept_depth_; }
  list::ElementDelegate* list_delegate() { return list_delegate_; }
  ListLayoutManager* list_layout_manager() {
    return list_layout_manager_.get();
  }
  int layout_id() const { return layout_id_; }
  bool sticky_enabled() const { return sticky_enabled_; }
  bool recycle_sticky_item() const { return recycle_sticky_item_; }
  float sticky_offset() const { return sticky_offset_; }
  bool should_request_state_restore() const {
    return should_request_state_restore_;
  }
  bool enable_insert_platform_view() const {
    return enable_insert_platform_view_;
  }
  const std::shared_ptr<pub::PubValueFactory>& value_factory() {
    return value_factory_;
  }
  bool has_valid_diff() const { return has_valid_diff_; }
  bool enable_batch_render() const { return false; }
  void ClearValidDiff() { has_valid_diff_ = false; }
  void MarkShouldFlushFinishLayout(bool has_layout) {
    should_flush_finish_layout_ |= has_layout;
  }
  void ResetLayoutID() { layout_id_ = -1; }

 protected:
  // Currently, the list container does not copy any member variables and is an
  // empty implementation.
  ListContainerImpl(const ListContainerImpl& list_container_impl) = delete;

 private:
  void UpdateListLayoutManager(list::LayoutType layout_type);
  //  fml::RefPtr<lepus::CArray> GenerateVisibleItemInfo() const;

 private:
  bool should_flush_finish_layout_{false};
  bool enable_insert_platform_view_{false};
  bool enable_dynamic_span_count_{true};
  bool span_count_changed_{false};
  bool recycle_available_item_before_layout_{false};
  bool sticky_enabled_{false};
  bool recycle_sticky_item_{true};
  int layout_id_{-1};
  int sticky_buffer_count_{list::kInvalidItemCount};
  float sticky_offset_{0.f};
  int intercept_depth_{0};
  float physical_pixels_per_layout_unit_{1.f};
  int initial_scroll_index_{-1};
  list::InitialScrollIndexStatus initial_scroll_index_status_{
      list::InitialScrollIndexStatus::kUnset};
  list::LayoutType layout_type_{list::LayoutType::kSingle};
  list::ElementDelegate* list_delegate_{nullptr};
  std::unique_ptr<ListLayoutManager> list_layout_manager_;
  std::unique_ptr<ListAdapter> list_adapter_;
  std::unique_ptr<ListChildrenHelper> list_children_helper_;
  std::unique_ptr<ListEventManager> list_event_manager_;
  bool need_recycle_all_item_holders_before_layout_{false};
  bool need_update_item_holders_{false};
  bool should_request_state_restore_{false};
  bool has_valid_diff_{false};
  std::shared_ptr<pub::PubValueFactory> value_factory_;
};

std::unique_ptr<ContainerDelegate> CreateListContainerDelegate(
    ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory);

}  // namespace list
}  // namespace lynx

#endif  // CORE_LIST_DECOUPLED_LIST_CONTAINER_IMPL_H_
