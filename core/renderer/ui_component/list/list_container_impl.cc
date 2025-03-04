// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_component/list/list_container_impl.h"

#include <algorithm>
#include <utility>

#include "base/trace/native/trace_event.h"
#include "core/renderer/dom/element_manager.h"
#include "core/renderer/ui_component/list/batch_list_adapter.h"
#include "core/renderer/ui_component/list/default_list_adapter.h"
#include "core/renderer/ui_component/list/grid_layout_manager.h"
#include "core/renderer/ui_component/list/linear_layout_manager.h"
#include "core/renderer/ui_component/list/staggered_grid_layout_manager.h"
#include "core/renderer/ui_wrapper/layout/list_node.h"
#include "core/renderer/utils/lynx_env.h"

namespace lynx {
namespace tasm {

ListContainerImpl::ListContainerImpl(Element* element)
    : element_(element),
      list_layout_manager_(std::make_unique<LinearLayoutManager>(this)),
      list_adapter_(std::make_unique<DefaultListAdapter>(this, element)),
      list_children_helper_(std::make_unique<ListChildrenHelper>()),
      list_event_manager_(std::make_unique<ListEventManager>(this)) {
  list_layout_manager_->InitLayoutManager(list_children_helper_.get(),
                                          list::Orientation::kVertical);
  list_event_manager_->SetChildrenHelper(list_children_helper_.get());
  NLIST_LOGI("ListContainerImpl::ListContainerImpl() this="
             << this << ", list_element=" << element_);
}

void ListContainerImpl::FinishBindItemHolder(Element* component,
                                             const PipelineOptions& option) {
  if (list_adapter_) {
    list_adapter_->OnFinishBindItemHolder(component, option);
  }
}

void ListContainerImpl::FinishBindItemHolders(
    const std::vector<Element*>& list_items, const PipelineOptions& options) {
  if (list_adapter_) {
    list_adapter_->OnFinishBindItemHolders(list_items, options);
  }
}

void ListContainerImpl::ReportListItemLifecycleStatistic(
    const PipelineOptions& option, const std::string& item_key) {
  if (option.enable_report_list_item_life_statistic_) {
    std::string id_selector;
    if (element() && element()->data_model()) {
      id_selector = element()->data_model()->idSelector().str();
    }
    report::EventTracker::OnEvent(
        [option = option, item_key, id_selector](report::MoveOnlyEvent& event) {
          event.SetName(list::kListItemLifecycleStatistic);
          event.SetProps(list::kListIdSelector, id_selector);
          event.SetProps(list::kItemKey, item_key);
          if (option.list_item_life_option_.update_duration() > 0.) {
            event.SetProps(list::kListItemUpdateDuration,
                           option.list_item_life_option_.update_duration());
          } else {
            event.SetProps(list::kListItemRenderDuration,
                           option.list_item_life_option_.render_duration());
            event.SetProps(list::kListItemDispatchDuration,
                           option.list_item_life_option_.dispatch_duration());
          }
          event.SetProps(list::kListItemLayoutDuration,
                         option.list_item_life_option_.layout_duration());
        });
  }
}

void ListContainerImpl::CheckZIndex(Element* child) const {
  if (!element_ || !child) {
    return;
  }
  if (child->has_z_props() && !element_->IsStackingContextNode()) {
    NLIST_LOGE("list is not stacking context node when child has z-index.");
  }
}

void ListContainerImpl::OnNextFrame() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListContainerImpl::OnNextFrame");
  list_layout_manager_->PreloadSection();
}

void ListContainerImpl::OnListItemLayoutUpdated(Element* component) {
  if (component) {
    const auto& attached_element_item_holder_map =
        list_children_helper_->attached_element_item_holder_map();
    auto it = attached_element_item_holder_map.end();
    if ((it = attached_element_item_holder_map.find(component)) !=
        attached_element_item_holder_map.end()) {
      list_adapter_->UpdateLayoutInfoToItemHolder(component, it->second);
    }
  }
}

void ListContainerImpl::RecordVisibleItemIfNeeded(bool is_layout_before) {
  if (!need_layout_complete_info_ || !layout_complete_info_) {
    return;
  }
  layout_complete_info_->SetValue(BASE_STATIC_STRING(list::kEventUnit),
                                  BASE_STATIC_STRING(list::kEventUnitPx));
  layout_complete_info_->SetValue(
      is_layout_before ? BASE_STATIC_STRING(list::kVisibleItemBeforeUpdate)
                       : BASE_STATIC_STRING(list::kVisibleItemAfterUpdate),
      GenerateVisibleItemInfo());
}

fml::RefPtr<lepus::CArray> ListContainerImpl::GenerateVisibleItemInfo() const {
  auto visible_item_info = lepus::CArray::Create();
  BASE_STATIC_STRING_DECL(kItemKey, "itemKey");
  BASE_STATIC_STRING_DECL(kIndex, "index");
  BASE_STATIC_STRING_DECL(kOriginX, "originX");
  BASE_STATIC_STRING_DECL(kOriginY, "originY");
  BASE_STATIC_STRING_DECL(kWidth, "width");
  BASE_STATIC_STRING_DECL(kHeight, "height");
  BASE_STATIC_STRING_DECL(kUpdated, "updated");
  BASE_STATIC_STRING_DECL(kIsBinding, "isBinding");
  const float layouts_unit_per_px =
      element_manager()->GetLynxEnvConfig().LayoutsUnitPerPx();
  if (base::FloatsLarger(layouts_unit_per_px, 0.f)) {
    list_children_helper_->ForEachChild(
        list_children_helper_->on_screen_children(),
        [&, this, layouts_unit_per_px](ItemHolder* item_holder) {
          if (list_layout_manager_->ItemHolderVisibleInList(item_holder)) {
            auto item_info = lepus::Dictionary::Create();
            item_info->SetValue(kItemKey, item_holder->item_key());
            item_info->SetValue(kIndex, item_holder->index());
            item_info->SetValue(kOriginX,
                                item_holder->left() / layouts_unit_per_px);
            item_info->SetValue(kOriginY,
                                item_holder->top() / layouts_unit_per_px);
            item_info->SetValue(kWidth,
                                item_holder->width() / layouts_unit_per_px);
            item_info->SetValue(kHeight,
                                item_holder->height() / layouts_unit_per_px);
            item_info->SetValue(kUpdated,
                                list_adapter_->IsUpdated(item_holder));
            item_info->SetValue(kIsBinding,
                                list_adapter_->IsBinding(item_holder));
            visible_item_info->emplace_back(item_info);
          }
          return false;
        });
  }
  return visible_item_info;
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
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListContainerImpl::FlushPatching");
  if (element_) {
    element_->painting_context()->UpdateLayoutPatching();
    element_->painting_context()->OnNodeReady(element_->impl_id());
    element_->painting_context()->UpdateNodeReadyPatching();
    if (should_flush_finish_layout_) {
      should_flush_finish_layout_ = false;
      PipelineOptions options;
      options.has_layout = true;
      element_->painting_context()->FinishLayoutOperation(options);
    }
    element_->painting_context()->FlushImmediately();
  }
}

// Update content offset and size to platform view.
void ListContainerImpl::UpdateContentOffsetAndSizeToPlatform(
    float content_size, float delta_x, float delta_y,
    bool is_init_scroll_offset) {
  if (element_) {
    element_->painting_context()->UpdateContentOffsetForListContainer(
        element_->impl_id(), content_size, delta_x, delta_y,
        is_init_scroll_offset);
  }
}

// Update scroll info to platform view.
void ListContainerImpl::UpdateScrollInfo(float estimated_offset, bool smooth,
                                         bool scrolling) {
  if (!element_ || !smooth) {
    return;
  }

  element_->painting_context()->UpdateScrollInfo(element_->impl_id(), smooth,
                                                 estimated_offset, scrolling);
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
  int initial_scroll_index = list_layout_manager_->GetInitialScrollIndex();
  list::InitialScrollIndexStatus initial_scroll_status =
      list_layout_manager_->GetInitialScrollIndexStatus();
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
  list_layout_manager_->SetInitialScrollIndex(initial_scroll_index);
  list_layout_manager_->SetInitialScrollStatus(initial_scroll_status);
  list_layout_manager_->SetSpanCount(span_count);
  list_layout_manager_->SetMainAxisGap(main_axis_gap);
  list_layout_manager_->SetCrossAxisGap(cross_axis_gap);
  list_layout_manager_->ResetContentOffsetAndContentSize(content_offset,
                                                         content_size);
  list_layout_manager_->SetPreloadBufferCount(preload_buffer_count);
  list_layout_manager_->SetEnablePreloadSection(enable_preload_section_);
  list_adapter_->OnDataSetChanged();
  need_recycle_all_item_holders_before_layout_ = true;
}

bool ListContainerImpl::ResolveAttribute(const base::String& key,
                                         const lepus::Value& value) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "ListContainerImpl::ResolveAttribute", "key",
              key.c_str());
  bool should_set_props = true;
  bool should_mark_layout_dirty = false;
  if (key.IsEqual(list::kCustomLisName) &&
      value.String() == BASE_STATIC_STRING(list::kListContainer)) {
    // native-list
    if (element_) {
      element_->UpdateLayoutNodeAttribute(
          starlight::LayoutAttribute::kListContainer, lepus::Value(true));
    }
  } else if (key.IsEqual(list::kListVerticalOrientation)) {
    // TODO: @deprecated vertical-orientation
    // vertical-orientation
    list::Orientation orientation = value.StdString() == "true"
                                        ? list::Orientation::kVertical
                                        : list::Orientation::kHorizontal;
    list_layout_manager_->SetOrientation(orientation);
    list_layout_manager_->SetListAnchorManager(list_children_helper_.get());
  } else if (key.IsEqual(list::kScrollOrientation)) {
    // scroll-orientation
    list::Orientation orientation;
    if (value.StdString() == "horizontal") {
      orientation = list::Orientation::kHorizontal;
    } else if (value.StdString() == "vertical") {
      orientation = list::Orientation::kVertical;
    } else {
      orientation = list::Orientation::kVertical;
    }
    list_layout_manager_->SetOrientation(orientation);
    list_layout_manager_->SetListAnchorManager(list_children_helper_.get());
  } else if ((key.IsEqual(list::kSpanCount) ||
              key.IsEqual(list::kColumnCount)) &&
             value.IsNumber()) {
    // TODO: @deprecated column-count
    int span_count = static_cast<int>(value.Number());
    if (span_count <= 0) {
      span_count = 1;
    }
    if (list_layout_manager_->span_count() != span_count) {
      list_adapter_->OnDataSetChanged();
      need_recycle_all_item_holders_before_layout_ = true;
    }
    list_layout_manager_->SetSpanCount(span_count);
    should_mark_layout_dirty = true;
    should_set_props = false;
  } else if (key.IsEqual(list::kAnchorPriority)) {
    list_layout_manager_->SetAnchorPriorityFromBegin(
        value.StdString() == list::kAnchorPriorityFromBegin);
    should_set_props = false;
  } else if (key.IsEqual(list::kAnchorAlign)) {
    list_layout_manager_->SetAnchorAlignToBottom(value.StdString() ==
                                                 list::kAnchorAlignToBottom);
    should_set_props = false;
  } else if (key.IsEqual(list::kAnchorVisibility)) {
    if (value.StdString() == list::kAnchorVisibilityHide) {
      list_layout_manager_->SetAnchorVisibility(
          list::AnchorVisibility::kAnchorVisibilityHide);
    } else if (value.StdString() == list::kAnchorVisibilityShow) {
      list_layout_manager_->SetAnchorVisibility(
          list::AnchorVisibility::kAnchorVisibilityShow);
    } else {
      list_layout_manager_->SetAnchorVisibility(
          list::AnchorVisibility::kAnchorVisibilityNoAdjustment);
    }
    should_set_props = false;
  } else if (key.IsEqual(list::kListPlatformInfo)) {
    // list-platform-info
    should_mark_layout_dirty = list_adapter_->UpdateDataSource(value);
    has_valid_diff_ = should_mark_layout_dirty;
    need_preload_section_on_next_frame_ = should_mark_layout_dirty;
    if (should_mark_layout_dirty) {
      list_layout_manager_->UpdateDiffAnchorReference();
    }
    should_set_props = false;
    need_update_item_holders_ = true;
  } else if (key.IsEqual(list::kFiberListDiffInfo)) {
    // fiber-list-info
    should_mark_layout_dirty = list_adapter_->UpdateFiberDataSource(value);
    has_valid_diff_ = should_mark_layout_dirty;
    need_preload_section_on_next_frame_ = should_mark_layout_dirty;
    if (should_mark_layout_dirty) {
      list_layout_manager_->UpdateDiffAnchorReference();
    }
    should_set_props = false;
    need_update_item_holders_ = true;
  } else if (key.IsEqual(list::kListType)) {
    // list-type
    list::LayoutType last_layout_type = layout_type_;
    const auto& value_str = value.StdString();
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
  } else if (key.IsEqual(list::kInitialScrollIndex)) {
    // initial-scroll-index
    list_layout_manager_->SetInitialScrollIndex(
        static_cast<int>(value.Number()));
  } else if (key.IsEqual(list::kUpperThresholdItemCount)) {
    // upper-threshold-item-count
    if (list_event_manager_) {
      list_event_manager_->SetUpperThresholdItemCount(
          static_cast<int>(value.Number()));
    }
    should_set_props = false;
  } else if (key.IsEqual(list::kLowerThresholdItemCount)) {
    // lower-threshold-item-count
    if (list_event_manager_) {
      list_event_manager_->SetLowerThresholdItemCount(
          static_cast<int>(value.Number()));
    }
    should_set_props = false;
  } else if (key.IsEqual(list::kNeedLayoutCompleteInfo)) {
    // need-layout-complete-info
    need_layout_complete_info_ = value.Bool();
  } else if (key.IsEqual(list::kLayoutID)) {
    layout_id_ = static_cast<int>(value.Number());
  } else if (key.IsEqual(list::kScrollEventThrottle)) {
    // scroll-event-throttle
    list_event_manager_->SetScrollEventThrottleMS(
        static_cast<int>(value.Number()));
    should_set_props = false;
  } else if (key.IsEqual(list::kNeedsVisibleCells) ||
             key.IsEqual(list::kNeedVisibleItemInfo)) {
    // TODO: @deprecated needs-visible-cells
    // need-visible-item-info
    if (list_event_manager_) {
      list_event_manager_->SetVisibleCell(value.Bool());
    }
    should_set_props = false;
  } else if (key.IsEqual(list::kShouldRequestStateRestore)) {
    should_request_state_restore_ = value.Bool();
    should_set_props = false;
  } else if (key.IsEqual(list::kStickyOffset)) {
    sticky_offset_ = value.Number();
  } else if (key.IsEqual(list::kSticky)) {
    sticky_enabled_ = value.Bool();
  } else if (key.IsEqual(list::kEnablePreloadSection)) {
    enable_preload_section_ = value.Bool();
    list_layout_manager_->SetEnablePreloadSection(enable_preload_section_);
    should_set_props = false;
  } else if (key.IsEqual(list::kPreloadBufferCount)) {
    should_mark_layout_dirty = list_layout_manager_->SetPreloadBufferCount(
        static_cast<int>(value.Number()));
    should_set_props = false;
  } else if (key.IsEqual(list::kExperimentalBatchRenderStrategy)) {
    // If parse experimental-batch-render-strategy in list property, we should
    // block flush this property to platform because before parsing all
    // properties of list element, we has pushed this property to prop_bundle.
    should_set_props = false;
  } else if (key.IsEqual(list::kListDebugInfoLevel)) {
    debug_info_level_ =
        std::min(list::ListDebugInfoLevel::kListDebugInfoLevelVerbose,
                 static_cast<list::ListDebugInfoLevel>(value.Number()));
    should_set_props = false;
  }
  if (should_mark_layout_dirty) {
    element_->MarkLayoutDirty();
  }
  return should_set_props;
};

void ListContainerImpl::OnLayoutChildren() {
  if (list_layout_manager_) {
    list_layout_manager_->SetListLayoutInfoToAllItemHolders();
    if (need_recycle_all_item_holders_before_layout_) {
      list_adapter_->RecycleAllItemHolders();
      need_recycle_all_item_holders_before_layout_ = false;
    }
    if (intercept_depth_ == 0) {
      // reset should_flush_finish_layout_ flag to false.
      should_flush_finish_layout_ = false;
      if (!enable_batch_render()) {
        list_layout_manager_->OnLayoutChildren();
      } else {
        list_layout_manager_->OnBatchLayoutChildren();
      }
    }
  }
}

bool ListContainerImpl::ShouldGenerateDebugInfo(
    list::ListDebugInfoLevel targetLevel) {
  return list::IsInDebugMode() && list_event_manager_ &&
         list_event_manager_->IsDebugEventBound() &&
         debug_info_level_ >= targetLevel;
}

void ListContainerImpl::PropsUpdateFinish() {
  if (need_layout_complete_info_) {
    if (!layout_complete_info_) {
      layout_complete_info_ = lepus::Dictionary::Create();
    }
    layout_complete_info_->SetValue(
        BASE_STATIC_STRING(list::kDiffResult),
        list_adapter_->list_adapter_helper()->GenerateDiffInfo());
  }
  if (ShouldGenerateDebugInfo(
          list::ListDebugInfoLevel::kListDebugInfoLevelInfo)) {
    auto detail = lepus::Dictionary::Create();
    detail->SetValue(BASE_STATIC_STRING(list::kDiffResult),
                     list_adapter_->list_adapter_helper()->GenerateDiffInfo());
    list_event_manager_->SendDebugEvent(detail);
  }
  // Note: need to move from DefaultListAdapter to BatchListAdapter before
  // invoke UpdateItemHolderToLatest().
  if (enable_batch_render() && !batch_adapter_initialized_) {
    // Move construct from DefaultListAdapter to BatchListAdapter.
    list_adapter_ =
        std::make_unique<BatchListAdapter>(std::move(*list_adapter_));
    // Note: set new list adapter to AnchorManager.
    list_layout_manager_->SetListAnchorManager(list_children_helper_.get());
    batch_adapter_initialized_ = true;
  }
  if (need_update_item_holders_) {
    list_adapter_->UpdateItemHolderToLatest(list_children_helper_.get());
    need_update_item_holders_ = false;
  }
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

void ListContainerImpl::UpdateListContainerDataSource(
    fml::RefPtr<lepus::Dictionary>& list_container_info) {
  if (list_adapter_) {
    list_adapter_->UpdateListContainerDataSource(list_container_info);
  }
}

void ListContainerImpl::AddEvent(const base::String& name) {
  if (list_event_manager_) {
    list_event_manager_->AddEvent(name.str());
  }
}

void ListContainerImpl::ClearEvents() {
  if (list_event_manager_) {
    list_event_manager_->ClearEvents();
  }
}

void ListContainerImpl::ResolveListAxisGap(CSSPropertyID id,
                                           const lepus::Value& value) {
  if (!list_layout_manager_ || !element_) {
    return;
  }
  float gap = 0.f;
  if (CSSPropertyID::kPropertyIDListMainAxisGap == id && value.IsNumber()) {
    gap = static_cast<float>(value.Number());
    if (base::FloatsNotEqual(gap, list_layout_manager_->main_axis_gap())) {
      list_layout_manager_->SetMainAxisGap(gap);
      element_->MarkLayoutDirty();
    }
  } else if (CSSPropertyID::kPropertyIDListCrossAxisGap == id &&
             value.IsNumber()) {
    gap = static_cast<float>(value.Number());
    if (base::FloatsNotEqual(gap, list_layout_manager_->cross_axis_gap())) {
      list_layout_manager_->SetCrossAxisGap(gap);
      element_->MarkLayoutDirty();
    }
  }
}

void ListContainerImpl::UpdateBatchRenderStrategy(
    list::BatchRenderStrategy strategy) {
  list_option_.batch_render_strategy = strategy;
}

list::BatchRenderStrategy ListContainerImpl::GetBatchRenderStrategy() {
  return list_option_.batch_render_strategy;
}

namespace list {
std::unique_ptr<ListContainer::Delegate> CreateListContainerDelegate(
    Element* element) {
  return std::make_unique<ListContainerImpl>(element);
}

bool IsInDebugMode() {
  return lynx::tasm::LynxEnv::GetInstance().IsDevToolComponentAttach();
}
}  // namespace list

}  // namespace tasm
}  // namespace lynx
