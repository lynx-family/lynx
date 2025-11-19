// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/list/decoupled_list_container_impl.h"

#include <algorithm>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/list/decoupled_default_list_adapter.h"
#include "core/list/decoupled_grid_layout_manager.h"
#include "core/list/decoupled_linear_layout_manager.h"
#include "core/list/decoupled_staggered_grid_layout_manager.h"
#include "core/renderer/trace/renderer_trace_event_def.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/services/timing_handler/timing_constants_deprecated.h"

namespace lynx {
namespace list {

ListContainerImpl::ListContainerImpl(
    list::ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory)
    : list_delegate_(list_delegate),
      list_layout_manager_(std::make_unique<LinearLayoutManager>(this)),
      list_adapter_(std::make_unique<DefaultListAdapter>(this)),
      list_children_helper_(std::make_unique<ListChildrenHelper>()),
      list_event_manager_(std::make_unique<ListEventManager>(this)),
      value_factory_(value_factory) {
  DLIST_LOGI("ListContainerImpl::ListContainerImpl() this=" << this);
  list_layout_manager_->InitLayoutManager(list_children_helper_.get(),
                                          list::Orientation::kVertical);
  list_event_manager_->SetChildrenHelper(list_children_helper_.get());
  physical_pixels_per_layout_unit_ =
      list_delegate_->GetPhysicalPixelsPerLayoutUnit();
  if (base::FloatsEqual(physical_pixels_per_layout_unit_, 0.f)) {
    physical_pixels_per_layout_unit_ = 1.f;
  }
}

ListContainerImpl::~ListContainerImpl() {
  DLIST_LOGI("ListContainerImpl::~ListContainerImpl this=" << this);
}

void ListContainerImpl::FinishBindItemHolder(
    list::ItemElementDelegate* list_item_delegate,
    const std::shared_ptr<tasm::PipelineOptions>& option) {
  if (list_adapter_) {
    list_adapter_->OnFinishBindItemHolder(list_item_delegate, option);
  }
}

float ListContainerImpl::RoundValueToPixelGrid(const float value) {
  return std::roundf(value * physical_pixels_per_layout_unit_) /
         physical_pixels_per_layout_unit_;
}

void ListContainerImpl::OnListItemLayoutUpdated(
    list::ItemElementDelegate* list_item_delegate) {
  if (list_item_delegate) {
    const auto& attached_delegate_item_holder_map =
        list_children_helper_->attached_delegate_item_holder_map();
    if (auto it = attached_delegate_item_holder_map.find(list_item_delegate);
        it != attached_delegate_item_holder_map.end()) {
      list_adapter_->UpdateLayoutInfoToItemHolder(list_item_delegate,
                                                  it->second);
    }
  }
}

// Get count of data source.
int ListContainerImpl::GetDataCount() const {
  return list_adapter_ ? list_adapter_->GetDataCount() : 0;
}

// Get the ItemHolder for the specified index.
ItemHolder* ListContainerImpl::GetItemHolderForIndex(int index) {
  return list_adapter_ ? list_adapter_->GetItemHolderForIndex(index) : nullptr;
}

// Flush all children's layout info patching to plaform.
void ListContainerImpl::FlushPatching() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_CONTAINER_FLUSH_PATCHING);
  bool should_flush_finish_layout = should_flush_finish_layout_;
  should_flush_finish_layout_ = false;
  list_delegate_->FlushPatching(should_flush_finish_layout);
}

// Update content offset and size to platform view.
void ListContainerImpl::UpdateContentOffsetAndSizeToPlatform(
    float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset, bool from_layout) {
  list_delegate_->UpdateContentOffsetAndSizeToPlatform(
      content_size, delta_x, delta_y, is_init_scroll_offset, from_layout);
}

// Update scroll info to platform view.
void ListContainerImpl::UpdateScrollInfo(float estimated_offset, bool smooth,
                                         bool scrolling) {
  if (smooth) {
    list_delegate_->UpdateScrollInfo(estimated_offset, smooth, scrolling);
  }
}

// This function should be called before any code that may trigger list's
// OnListElementUpdated() to enable list avoid reacting to additional redundant
// OnListElementUpdated() calls.
void ListContainerImpl::StartInterceptListElementUpdated() {
  intercept_depth_++;
}

// This method should be called after any code that may trigger list's
// OnListElementUpdated().
void ListContainerImpl::StopInterceptListElementUpdated() {
  if (intercept_depth_ < 1) {
    intercept_depth_ = 1;
  }
  intercept_depth_--;
}

void ListContainerImpl::UpdateListLayoutManager(list::LayoutType layout_type) {
  int span_count = list_layout_manager_->span_count();
  list::Orientation orientation = list_layout_manager_->orientation();
  float main_axis_gap = list_layout_manager_->main_axis_gap();
  float cross_axis_gap = list_layout_manager_->cross_axis_gap();
  float preload_buffer_count = list_layout_manager_->preload_buffer_count();
  float content_size = list_layout_manager_->content_size();
  // Store the previous content_offset_ or the delta calculation may be
  // incorrect
  float content_offset = list_layout_manager_->content_offset();
  if (layout_type == list::LayoutType::kSingle) {
    list_layout_manager_ = std::make_unique<LinearLayoutManager>(this);
  } else if (layout_type == list::LayoutType::kFlow) {
    list_layout_manager_ = std::make_unique<GridLayoutManager>(this);
  } else if (layout_type == list::LayoutType::kWaterFall) {
    list_layout_manager_ = std::make_unique<StaggeredGridLayoutManager>(this);
  }
  list_layout_manager_->InitLayoutManager(list_children_helper_.get(),
                                          orientation);
  list_layout_manager_->SetSpanCount(span_count);
  list_layout_manager_->SetMainAxisGap(main_axis_gap);
  list_layout_manager_->SetCrossAxisGap(cross_axis_gap);
  list_layout_manager_->ResetContentOffsetAndContentSize(content_offset,
                                                         content_size);
  list_layout_manager_->SetPreloadBufferCount(preload_buffer_count);
  list_adapter_->OnDataSetChanged();
  need_recycle_all_item_holders_before_layout_ = true;
}

bool ListContainerImpl::ResolveAttribute(const pub::Value& key,
                                         const pub::Value& value) {
  if (!key.IsString()) {
    return true;
  }
  const std::string& key_str = key.str();
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LIST_CONTAINER_RESOLVE_ATTRIBUTE, "key",
              key_str.c_str());
  bool should_set_props = true;
  bool should_mark_layout_dirty = false;
  if (key_str == list::kPropCustomListName && value.IsString() &&
      value.str() == list::kListContainer) {
    // list-container
    list_delegate_->UpdateListLayoutNodeAttribute();
  } else if (key_str == list::kPropScrollOrientation && value.IsString()) {
    // scroll-orientation
    list::Orientation orientation;
    const std::string& value_str = value.str();
    if (value_str == "horizontal") {
      orientation = list::Orientation::kHorizontal;
    } else if (value_str == "vertical") {
      orientation = list::Orientation::kVertical;
    } else {
      orientation = list::Orientation::kVertical;
    }
    list_layout_manager_->SetOrientation(orientation);
    list_layout_manager_->SetListAnchorManager(list_children_helper_.get());
  } else if (key_str == list::kPropEnableDynamicSpanCount) {
    // enable-dynamic-span-count
    enable_dynamic_span_count_ = value.Bool();
    should_set_props = false;
  } else if ((key_str == list::kPropSpanCount) && value.IsNumber()) {
    // span-count
    int span_count = static_cast<int>(value.Number());
    if (span_count <= 0) {
      span_count = 1;
    }
    if (list_layout_manager_->span_count() != span_count) {
      span_count_changed_ = true;
    }
    list_layout_manager_->SetSpanCount(span_count);
    should_mark_layout_dirty = true;
    should_set_props = false;
  } else if (key_str == list::kPropListPlatformInfo) {
    // list-platform-info
    should_mark_layout_dirty = list_adapter_->UpdateDataSource(value);
    has_valid_diff_ = should_mark_layout_dirty;
    //    if (should_mark_layout_dirty) {
    //      list_layout_manager_->UpdateDiffAnchorReference();
    //    }
    should_set_props = false;
    need_update_item_holders_ = true;
  } else if (key_str == list::kPropFiberListDiffInfo) {
    // fiber-list-info
    should_mark_layout_dirty = list_adapter_->UpdateFiberDataSource(value);
    has_valid_diff_ = should_mark_layout_dirty;
    //    if (should_mark_layout_dirty) {
    //      list_layout_manager_->UpdateDiffAnchorReference();
    //    }
    should_set_props = false;
    need_update_item_holders_ = true;
  } else if (key_str == list::kPropListType && value.IsString()) {
    // list-type
    list::LayoutType last_layout_type = layout_type_;
    const std::string& value_str = value.str();
    if (value_str == list::kListTypeSingle) {
      layout_type_ = list::LayoutType::kSingle;
    } else if (value_str == list::kListTypeFlow) {
      layout_type_ = list::LayoutType::kFlow;
    } else if (value_str == list::kListTypeWaterFall) {
      layout_type_ = list::LayoutType::kWaterFall;
    }
    if (layout_type_ != last_layout_type) {
      UpdateListLayoutManager(layout_type_);
    }
    should_mark_layout_dirty = true;
    should_set_props = false;
  } else if (key_str == list::kPropInitialScrollIndex && value.IsNumber()) {
    // initial-scroll-index
    initial_scroll_index_ = static_cast<int>(value.Number());
  } else if (key_str == list::kPropUpperThresholdItemCount &&
             value.IsNumber()) {
    // upper-threshold-item-count
    if (list_event_manager_) {
      list_event_manager_->SetUpperThresholdItemCount(
          static_cast<int>(value.Number()));
    }
    should_set_props = false;
  } else if (key_str == list::kPropLowerThresholdItemCount &&
             value.IsNumber()) {
    // lower-threshold-item-count
    if (list_event_manager_) {
      list_event_manager_->SetLowerThresholdItemCount(
          static_cast<int>(value.Number()));
    }
    should_set_props = false;
  } else if (key_str == list::kPropNeedLayoutCompleteInfo && value.IsBool()) {
    // need-layout-complete-info
    list_event_manager_->SetNeedLayoutCompleteInfo(value.Bool());
  } else if (key_str == list::kPropLayoutId && value.IsNumber()) {
    // layout-id
    layout_id_ = static_cast<int>(value.Number());
  } else if (key_str == list::kPropScrollEventThrottle && value.IsNumber()) {
    // scroll-event-throttle
    list_event_manager_->SetScrollEventThrottleMS(
        static_cast<int>(value.Number()));
    should_set_props = false;
  } else if (key_str == list::kPropNeedVisibleItemInfo && value.IsBool()) {
    // need-visible-item-info
    if (list_event_manager_) {
      list_event_manager_->SetVisibleCell(value.Bool());
    }
    should_set_props = true;
  } else if (key_str == list::kPropShouldRequestStateRestore &&
             value.IsBool()) {
    // should-request-state-restore
    should_request_state_restore_ = value.Bool();
    should_set_props = false;
  } else if (key_str == list::kPropStickyOffset && value.IsNumber()) {
    // sticky-offset
    sticky_offset_ = value.Number();
  } else if (key_str == list::kPropSticky && value.IsBool()) {
    // sticky
    sticky_enabled_ = value.Bool();
  } else if (key_str == list::kPropExperimentalRecycleStickyItem &&
             value.IsBool()) {
    // experimental-recycle-sticky-item
    // TODO(dingwang.wxx): experimental prop, the default value is true in
    // release3.4.
    recycle_sticky_item_ = value.Bool();
  } else if (key_str == list::kPropStickyBufferCount && value.IsNumber()) {
    // sticky-buffer-count
    sticky_buffer_count_ = static_cast<int>(value.Number());
  } else if (key_str == list::kPropPreloadBufferCount && value.IsNumber()) {
    // preload-buffer-count
    should_mark_layout_dirty = list_layout_manager_->SetPreloadBufferCount(
        static_cast<int>(value.Number()));
    should_set_props = false;
  } else if (key_str ==
                 list::kPropExperimentalRecycleAvailableItemBeforeLayout &&
             value.IsBool()) {
    // experimental-recycle-available-item-before-layout
    recycle_available_item_before_layout_ = value.Bool();
    should_set_props = false;
  }
  if (should_mark_layout_dirty) {
    list_delegate_->MarkListElementLayoutDirty();
  }
  return should_set_props;
};

void ListContainerImpl::OnLayoutChildren(
    const std::shared_ptr<tasm::PipelineOptions>& options) {
  if (list_layout_manager_) {
    if (need_recycle_all_item_holders_before_layout_) {
      list_adapter_->RecycleAllItemHolders();
      need_recycle_all_item_holders_before_layout_ = false;
    }
    if (intercept_depth_ == 0) {
      // Note: we should reset should_flush_finish_layout_ to
      // options->has_layout to make sure invoke FinishLayoutOperation() to
      // trigger layoutDidFinished lifecycle of all list's children.
      should_flush_finish_layout_ = options->has_layout;
      // Try to enqueue all available items before layout.
      if (recycle_available_item_before_layout_) {
        list_adapter_->EnqueueElementsIfNeeded();
      }
      if (!enable_batch_render()) {
        list_layout_manager_->OnLayoutChildren();
      } else {
        list_layout_manager_->OnBatchLayoutChildren();
      }
    }
  }
}

void ListContainerImpl::PropsUpdateFinish() {
  if ((initial_scroll_index_ >= 0 && initial_scroll_index_ < GetDataCount()) &&
      initial_scroll_index_status_ == list::InitialScrollIndexStatus::kUnset) {
    initial_scroll_index_status_ = list::InitialScrollIndexStatus::kSet;
    list_layout_manager_->SetInitialScrollIndex(initial_scroll_index_);
  }

  if (span_count_changed_) {
    span_count_changed_ = false;
    if (!enable_dynamic_span_count_) {
      // Note: OnDataSetChanged() should be invoked before
      // UpdateItemHolderToLatest() if needed.
      list_adapter_->OnDataSetChanged();
      need_recycle_all_item_holders_before_layout_ = true;
    }
  }
  list_event_manager_->RecordDiffResultIfNeeded();
  if (need_update_item_holders_) {
    list_adapter_->UpdateItemHolderToLatest(list_children_helper_.get());
    need_update_item_holders_ = false;
  }
  if (sticky_buffer_count_ > list::kInvalidItemCount) {
    if (!recycle_sticky_item_) {
      // Note: A valid sticky buffer count means need to recycle sticky
      // item.
      recycle_sticky_item_ = true;
    }
    list_children_helper_->SetCustomStickyItemHolderCapacity(
        sticky_buffer_count_);
  }
  list_children_helper_->SetRecycleStickyItem(recycle_sticky_item_);
  list_adapter()->list_adapter_helper()->ClearDiffInfo();
}

void ListContainerImpl::ScrollByPlatformContainer(float content_offset_x,
                                                  float content_offset_y,
                                                  float original_x,
                                                  float original_y) {
  if (list_layout_manager_) {
    // reset should_flush_finish_layout_ flag to false.
    should_flush_finish_layout_ = false;
    list_layout_manager_->ScrollByPlatformContainer(
        content_offset_x, content_offset_y, original_x, original_y);
  }
}

void ListContainerImpl::ScrollToPosition(int index, float offset, int align,
                                         bool smooth) {
  if (list_layout_manager_) {
    list_layout_manager_->ScrollToPosition(index, offset, align, smooth);
  }
}

void ListContainerImpl::ScrollStopped() {
  if (list_layout_manager_) {
    list_layout_manager_->ScrollStopped();
  }
}

void ListContainerImpl::OnAttachedToElementManager() {
  physical_pixels_per_layout_unit_ =
      list_delegate_->GetPhysicalPixelsPerLayoutUnit();
  if (base::FloatsEqual(physical_pixels_per_layout_unit_, 0.f)) {
    physical_pixels_per_layout_unit_ = 1.f;
  }
}

void ListContainerImpl::ResolveListAxisGap(tasm::CSSPropertyID id, float gap) {
  if (tasm::CSSPropertyID::kPropertyIDListMainAxisGap == id) {
    if (base::FloatsNotEqual(gap, list_layout_manager_->main_axis_gap())) {
      list_layout_manager_->SetMainAxisGap(gap);
      list_delegate_->MarkListElementLayoutDirty();
    }
  } else if (tasm::CSSPropertyID::kPropertyIDListCrossAxisGap == id) {
    if (base::FloatsNotEqual(gap, list_layout_manager_->cross_axis_gap())) {
      list_layout_manager_->SetCrossAxisGap(gap);
      list_delegate_->MarkListElementLayoutDirty();
    }
  }
}

std::unique_ptr<ContainerDelegate> CreateListContainerDelegate(
    ElementDelegate* list_delegate,
    const std::shared_ptr<pub::PubValueFactory>& value_factory) {
  return std::make_unique<ListContainerImpl>(list_delegate, value_factory);
}

}  // namespace list
}  // namespace lynx
