// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/component/list/base_list_view.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/include/float_comparison.h"
#include "base/include/timer/time_utils.h"
#include "base/trace/native/trace_event.h"
#include "clay/fml/logging.h"
#include "clay/gfx/geometry/float_point.h"
#include "clay/ui/common/attribute_utils.h"
#include "clay/ui/common/macros.h"
#include "clay/ui/component/base_view.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/keywords.h"
#include "clay/ui/component/list/list_adapter.h"
#include "clay/ui/component/list/list_adapter_updater.h"
#include "clay/ui/component/list/list_children_helper.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_layout_manager.h"
#include "clay/ui/component/list/list_layout_manager_linear.h"
#include "clay/ui/component/list/list_orientation_helper.h"
#include "clay/ui/component/list/list_recycler.h"
#include "clay/ui/component/list/list_scroller.h"
#include "clay/ui/component/list/lynx_list_adapter.h"
#include "clay/ui/component/list/macros.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/event/focus_manager.h"
#include "clay/ui/rendering/render_list.h"

namespace clay {

static bool sAllowListPrefetch = true;

void SetAllowListPrefetch(bool allow) {
  if (sAllowListPrefetch == allow) {
    return;
  }
  sAllowListPrefetch = allow;
}

void LayoutPrefetchRegistry::AddPosition(int layout_position,
                                         int pixel_distance) {
  FML_DCHECK(pixel_distance > 0);
  prefetch_item_infos_[layout_position] = pixel_distance;
}

void LayoutPrefetchRegistry::CollectPrefetchPostionFromView(
    BaseListView* list_view, int prefetch_delta_x, int prefetch_delta_y) {
  ListLayoutManager* layout_manager = list_view->GetLayoutManager();
  if (layout_manager) {
    layout_manager->CollectAdjacentPrefetchPositions(
        prefetch_delta_x, prefetch_delta_y,
        list_view->recycler_->GetCacheMaxLimit(), list_view->list_state_, this);
  }
}

bool LayoutPrefetchRegistry::LastPrefetchIncludePosition(int position) {
  return prefetch_item_infos_.count(position) > 0;
}

void LayoutPrefetchRegistry::ClearPrefetchPositions() {
  prefetch_item_infos_.clear();
}

void BaseListView::ListPrefetchTask::Run() {
  if (host_view_) {
    TRACE_EVENT("clay", "ListPrefetchTask::Run");
    auto list_view = static_cast<BaseListView*>(host_view_.get());
    ListRecycler* recycler = list_view->recycler_.get();
    if (recycler->HasItemCached(id_)) {
      return;
    }
    ListItemViewHolder* view_holder = recycler->GetItemForPosition(id_);
    if (view_holder && view_holder->IsBound() && !view_holder->IsInvalid()) {
      view_holder->AddFlags(ListItemViewHolder::Flag::kFlagPrefetch);
      recycler->RecycleItem(view_holder);
    }
  }
}

BaseListView::BaseListView(int32_t id, std::string tag, PageView* page_view)
    : BaseListView(id, id, tag, page_view) {}

BaseListView::BaseListView(int32_t id, int32_t callback_id, std::string tag,
                           PageView* page_view)
    : WithTypeInfo(id, std::move(tag), std::make_unique<RenderList>(),
                   page_view, ScrollDirection::kVertical, true),
      callback_id_(callback_id),
      children_helper_(std::make_unique<ListChildrenHelper>()),
      callback_manager_(
          std::make_unique<ListEventCallbackManager>(this, callback_id)) {
  SetIsFocusScope();
  GetFocusManager()->SetFirstFocusedNodeExpandRatio(1.f / 3.f);
  if (sAllowListPrefetch) {
    gap_task_collector_ = [weak_this = GetWeakPtr()]() {
      if (weak_this) {
        BaseListView* list_view = static_cast<BaseListView*>(weak_this.get());
        list_view->StartPrefetch(-list_view->current_scroll_width_,
                                 -list_view->current_scroll_height_);
      }
    };
  }

#if defined(OS_IOS) || defined(OS_OSX)
  // Enable bounce effect by default on iOS and macOS.
  SetOverscrollEnabled(true);
#endif
}

BaseListView::~BaseListView() = default;

void BaseListView::PostLowPriorityTask(std::function<void()> task) {
  if (GetListScroller()->Scrolling()) {
    pending_tasks_.push_back(task);
    FlushTasks();
  } else {
    task();
  }
}

void BaseListView::FlushTasks() {
  if (GetListScroller()->Scrolling()) {
    if (!task_timer_) {
      task_timer_ =
          std::make_unique<fml::Timer>(page_view()->GetTaskRunner(), false);
    } else if (!task_timer_->Stopped()) {
      return;
    }
    task_timer_->Start(
        fml::TimeDelta::FromMilliseconds(100), [weak_this = GetWeakPtr()] {
          if (weak_this) {
            static_cast<BaseListView*>(weak_this.get())->FlushTasks();
          }
        });
    return;
  }

  auto tasks = std::move(pending_tasks_);
  for (auto task : tasks) {
    task();
  }
}

void BaseListView::SetWidth(float width) {
  if (width_ == width) {
    return;
  }
  if (layout_manager_ && layout_manager_->CanScrollVertically()) {
    layout_manager_->InvalidateLayoutCache();
  }
  BaseView::SetWidth(width);
  MarkNeedsLayout();
}

void BaseListView::SetHeight(float height) {
  if (height_ == height) {
    return;
  }
  if (layout_manager_ && layout_manager_->CanScrollHorizontally()) {
    layout_manager_->InvalidateLayoutCache();
  }
  BaseView::SetHeight(height);
  MarkNeedsLayout();
}

void BaseListView::SetPaddings(float padding_left, float padding_top,
                               float padding_right, float padding_bottom) {
  bool should_invalidate_layout_cache = false;
  if (layout_manager_) {
    if (layout_manager_->CanScrollVertically()) {
      should_invalidate_layout_cache =
          !lynx::base::FloatsEqual(Width() - PaddingLeft() - PaddingRight(),
                                   Width() - padding_left - padding_right);
    } else {
      should_invalidate_layout_cache =
          !lynx::base::FloatsEqual(Height() - PaddingTop() - PaddingBottom(),
                                   Height() - padding_top - padding_bottom);
    }
  }
  if (should_invalidate_layout_cache) {
    layout_manager_->InvalidateLayoutCache();
  }
  BaseView::SetPaddings(padding_left, padding_top, padding_right,
                        padding_bottom);
}

void BaseListView::SetAdapter(ListAdapter* adapter) {
  adapter_ = adapter;
  if (adapter_) {
    adapter_->SetObserver(this);
    recycler_ = std::make_unique<ListRecycler>(adapter_);
    adapter_helper_ = std::make_unique<ListAdapterHelper>(this);
    list_state_.structure_changed = true;
    ProcessDataSetCompletelyChanged(false);
    MarkNeedsLayout();
  } else {
    ResetListScroller();
  }
}

void BaseListView::NotifyLowMemory() {
  if (recycler_) {
    // Delete prefetch cache
    recycler_->MarkKnownViewsInvalid();
  }
}

void BaseListView::SetLayoutManager(
    std::unique_ptr<ListLayoutManager> layout_manager) {
  // Reset to idle
  SetScrollStatus(Scrollable::ScrollStatus::kIdle);
  if (layout_manager_ && recycler_) {
    layout_manager_->RemoveAndRecycleAllViews(*recycler_);
    recycler_->Clear();
  }

  ResetListScroller();

  layout_manager_ = std::move(layout_manager);
  layout_manager_->SetListView(this);
  layout_manager_->SetChildrenHelper(children_helper_.get());
  callback_manager_->SetLayoutManager(layout_manager_.get());
  if (recycler_) {
    recycler_->SetCacheMaxLimit(layout_span_count_);
  }

  DidUpdateAttributes();
}

void BaseListView::SetAttribute(const char* attr_c, const clay::Value& value) {
  auto kw = GetKeywordID(attr_c);
  if (kw == KeywordID::kListType) {
    UpdateLayoutManager(attribute_utils::GetCString(value), layout_span_count_);
  } else if (kw == KeywordID::kColumnCount) {
    UpdateLayoutManager(layout_manager_type_,
                        attribute_utils::GetNum(value, 1));
  } else if (kw == KeywordID::kLowerThreshold) {
    lower_threshold_pixel_ = attribute_utils::GetNum(value);
  } else if (kw == KeywordID::kUpperThreshold) {
    upper_threshold_pixel_ = attribute_utils::GetNum(value);
  } else if (kw == KeywordID::kLowerThresholdItemCount) {
    lower_threshold_item_count_ = attribute_utils::GetNum(value);
  } else if (kw == KeywordID::kUpperThresholdItemCount) {
    upper_threshold_item_count_ = attribute_utils::GetNum(value);
  } else if (kw == KeywordID::kScrollEventThrottle) {
    double throttle = 0.0;
    if (!attribute_utils::TryGetNum(value, throttle)) {
      return;
    }
    callback_manager_->SetScrollEventThrottle(
        fml::TimeDelta::FromMilliseconds(static_cast<int>(throttle)));
  } else if (kw == KeywordID::kNeedsVisibleCells) {
    callback_manager_->SetNeedsVisibleCells(attribute_utils::GetBool(value));
  } else if (kw == KeywordID::kEnableNestedScroll) {
    bool enable_nested_scroll = attribute_utils::GetBool(value);
    SetEnableNestedScroll(enable_nested_scroll);
  } else if (kw == KeywordID::kEnableScroll) {
    bool enabled = attribute_utils::GetBool(value);
    SetScrollEnabled(enabled);
  } else if (kw == KeywordID::kScrollMonitorTag) {
    std::string tag;
    if (attribute_utils::TryGetString(value, tag)) {
      SetScrollMonitorTag(tag);
    }
  } else if (kw == KeywordID::kUpdateAnimation) {
    bool update_animation = attribute_utils::GetCString(value) ==
                            attr_value::kListUpdateAnimationDefault;
    if (update_animation_enabled_ != update_animation) {
      update_animation_enabled_ = update_animation;
      if (update_animation) {
        updater_ = std::make_unique<ListAdapterUpdater>(this);
      }
    }
  } else if (kw == KeywordID::kFocusSmoothScroll) {
    focus_smooth_scroll_ =
        attribute_utils::GetBool(value, true);  // default enable
  } else if (kw == KeywordID::kAlignFocus) {
    std::string val = attribute_utils::GetCString(value);
    if (val == "middle") {
      align_focus_ = AlignTo::kMiddle;
    } else if (val == "start") {
      align_focus_ = AlignTo::kStart;
    } else if (val == "end") {
      align_focus_ = AlignTo::kEnd;
    } else if (val == "none") {
      align_focus_ = AlignTo::kNone;
    } else {
      FML_DLOG(ERROR) << "Unrecognized align-focus value: " << val;
    }
  } else if (kw == KeywordID::kSticky) {
    SetStickyEnabled(attribute_utils::GetBool(value));
  } else if (kw == KeywordID::kScrollWithoutFocus) {
    scroll_without_focus_ = attribute_utils::GetBool(value, false);
  } else if (kw == KeywordID::kScrollDirection ||
             kw == KeywordID::kScrollOrientation) {
    std::string val = attribute_utils::GetCString(value);
    auto scroll_orientation = ScrollDirection::kVertical;
    if (val == "horizontal") {
      scroll_orientation = ScrollDirection::kHorizontal;
    } else if (val == "vertical") {
      scroll_orientation = ScrollDirection::kVertical;
    } else {
      return;
    }
    UpdateScrollOrientation(scroll_orientation);
  } else if (kw == KeywordID::kVerticalOrientation) {
    auto orientation_vertical = attribute_utils::GetBool(value, true);
    auto scroll_orientation = orientation_vertical
                                  ? ScrollDirection::kVertical
                                  : ScrollDirection::kHorizontal;
    UpdateScrollOrientation(scroll_orientation);
  } else if (kw == KeywordID::kListCrossAxisGap) {
    float gap = attribute_utils::GetDouble(value, 0);
    cross_axis_gap_ = gap;
    layout_manager_->SetCrossAxisGap(gap);
  } else if (kw == KeywordID::kListMainAxisGap) {
    float gap = attribute_utils::GetDouble(value, 0);
    main_axis_gap_ = gap;
    layout_manager_->SetMainAxisGap(gap);
  } else if (kw == KeywordID::kBounce || kw == KeywordID::kBounces) {
    auto enable_bounce = attribute_utils::GetBool(value);
    SetOverscrollEnabled(enable_bounce);
  } else if (kw == KeywordID::kStickyOffset) {
    auto offset = attribute_utils::GetDouble(value);
    layout_manager_->SetStickyOffset(offset);
  } else if (kw == KeywordID::kInitialScrollIndex) {
    auto index = attribute_utils::GetInt(value);
    layout_manager_->ScrollToPosition(index);
  } else {
    BaseView::SetAttribute(attr_c, value);
  }
}

void BaseListView::UpdateScrollOrientation(ScrollDirection scroll_orientation) {
  // TODO(liuguoliang): Use `scroll_direction_` instead
  this->scroll_orientation_ = scroll_orientation;
  layout_manager_->SetOrientation(scroll_orientation);
  SetScrollDirection(scroll_orientation);
  ResetGestureRecognizers();
}

void BaseListView::AddEventCallback(const char* event_c) {
  NestedScrollable::AddEventCallback(event_c);
  std::string event(event_c);
  callback_manager_->AddEventCallback(event);
}

void BaseListView::UpdateLayoutManager(const std::string& type,
                                       int span_count) {
  if (span_count != layout_span_count_ && type == attr_value::kListTypeSingle) {
    // linear layout will not affected by span_count change.
    layout_span_count_ = span_count;
    // TODO(hanhaoshen): this for list demo, when lynx set span_count for list,
    // they manually set width of item , in that case relayout is required.
    MarkNeedsLayout();
    return;
  }
  if (type == layout_manager_type_ && span_count == layout_span_count_) {
    return;
  }
  LIST_LOG << "UpdateLayoutManager layout_type: " << type
           << " span_count: " << span_count;
  layout_manager_type_ = type;
  layout_span_count_ = span_count;
  SetLayoutManager(ListLayoutManager::Create(type, span_count));
  layout_manager_->SetCrossAxisGap(cross_axis_gap_);
  layout_manager_->SetMainAxisGap(main_axis_gap_);
  // update with last set orientation
  if (scroll_orientation_ != layout_manager_->GetOrientation()) {
    layout_manager_->SetOrientation(scroll_orientation_);
    SetScrollDirection(layout_manager_->GetOrientation());
    ResetGestureRecognizers();
  }
  // Cache one line at most
  recycler_->SetCacheMaxLimit(span_count);

  MarkNeedsLayout();
}

void BaseListView::AddChild(BaseView* child, int index) {
  FML_DCHECK(std::find(detached_children_.begin(), detached_children_.end(),
                       child) == detached_children_.end());
  // Make all list items repaint boundaries to improve scrolling performance
  child->render_object()->SetRepaintBoundary(true);

  BaseView* parent = child->parent_;
  if (parent) {
    parent->BaseView::RemoveChild(child);
  }

  // Note(Xietong): Don't set the parent
  detached_children_.insert(detached_children_.end(), child);
}

void BaseListView::RemoveChild(BaseView* child) {
  std::vector<BaseView*>::iterator iter(
      std::find(detached_children_.begin(), detached_children_.end(), child));
  if (iter != detached_children_.end()) {
    FML_DCHECK(child->parent_ == nullptr);
    detached_children_.erase(iter);
  }
  // Make sure we have removed this child from render tree.
  // Because this child might still exist in render tree.
  BaseView::RemoveChild(child);
}

void BaseListView::OnDestroy() {
  page_view_->GetGapWorker()->UnregisterTaskCollector(this);
  if (adapter_) {
    adapter_->SetObserver(nullptr);
  }

  if (!pending_tasks_.empty()) {
    auto tasks = std::move(pending_tasks_);
    for (auto task : tasks) {
      task();
    }
  }
  callback_manager_ = nullptr;
}

void BaseListView::DestroyAllChildren() {
  // Remove all children normally means cleanup the view. We will add the
  // detached children list to the actual list so that the detached children
  // will be removed as well.
  children_.insert(children_.end(), detached_children_.begin(),
                   detached_children_.end());
  detached_children_.clear();
  BaseView::DestroyAllChildren();
}

float BaseListView::GetScrollbarScrollOffset() {
  // TODO(Xietong): whatif there is pending ops?
  list_state_.item_count = adapter_->GetItemCount();
  return layout_manager_->GetScrollOffset(list_state_);
}

float BaseListView::GetTotalScrollLength() {
  // TODO(Xietong): whatif there is pending ops?
  list_state_.item_count = adapter_->GetItemCount();
  return layout_manager_->GetTotalLength(list_state_);
}

void BaseListView::OnCreateItem(ListItemViewHolder* item, int position) {
  // When a view is created, it is not attached yet.
  BaseView* view = HandleCreateView(item);

  // `ListView` already calls `SetView()` during `HandleCreateView()`.
  if (!item->GetView()) {
    item->SetView(view);
  }
}

void BaseListView::OnAddItem(ListItemViewHolder* item, int index) {
  last_add_item_position_ = item->GetPosition();
  if (item->HasAnyOfFlags(ListItemViewHolder::kFlagScrapped)) {
    FML_DCHECK(item->GetViewAttached());
    item->RemoveFlags(ListItemViewHolder::kFlagScrapped);
    return;
  }

  LIST_LOG << "Attach view(id): " << item->GetView()->id()
           << " to list with index: " << index;

  // The below AddChild/RemoveChild will mark needs layout, which is unnecessary
  // and may cause focus lost issue.
  BaseView::LayoutIgnoreHelper helper(this);

  // When adding an item, it means the view is detached and will be attached.
  FML_DCHECK(!item->GetViewAttached());
  RemoveChild(item->GetView());
  BaseView::AddChild(item->GetView(), index);
  FML_DCHECK(item->GetViewAttached());

  if (item->GetPosition() == focus_node_pos_) {
    FML_DCHECK(!focus_node_attached_);
    focus_node_attached_ = true;
  }

  // In some cases, there may be item that is scrapped with the same view, and
  // to ensure that scrapped item do not affect the attached view, we try to
  // remove it if it exists.
  recycler_->RemoveScrappedItemByView(item->GetView());
}

void BaseListView::OnRemoveItem(ListItemViewHolder* item) {
  // Move view to detached vector.
  FML_DCHECK(item->GetViewAttached());

  // The below AddChild/RemoveChild will mark needs layout, which is unnecessary
  // and may cause focus lost issue.
  BaseView::LayoutIgnoreHelper helper(this);

  LIST_LOG << "Detached view_id: " << item->GetView()->id();
  BaseView::RemoveChild(item->GetView());
  AddChild(item->GetView(), 0 /* not used */);

  if (item->GetPosition() == focus_node_pos_) {
    focus_node_attached_ = false;
  }
}

void BaseListView::OnDestroyItem(ListItemViewHolder* item) {
  FML_DCHECK(!item->GetViewAttached());
  RemoveChild(item->GetView());
  HandleDestroyView(item->GetView(), item);

  if (item->GetPosition() == focus_node_pos_) {
    ClearFocusNodeInfo();
  }
}

void BaseListView::OnBindListItem(ListItemViewHolder* item, int prev_position,
                                  int position, bool newly_created) {
  if (prev_position != ListItemViewHolder::kNoPosition &&
      prev_position != position && prev_position == focus_node_pos_) {
    // The previous focus node's position is updated to different position,
    // which means the item is recycled and re-used for other position. Cleanup
    // the focus cache.
    ClearFocusNodeInfo();
  }
}

void BaseListView::OffsetChildren(float x, float y) {
  children_helper_->ForEach([x, y](ListItemViewHolder* child) {
    child->LayoutWithOffset({x, y});
    return false;
  });
}

void BaseListView::DispatchLayout() {
  TRACE_EVENT("clay", "List::OnLayout");
  BaseView::LayoutIgnoreHelper helper(this);
  DCHECK_RET0(adapter_);

  LIST_LOG << "------ Before OnLayout ------";
  PrintChildren();
  adapter_->PrintViewHolders();
  LIST_LOG << "Children Helper:" << children_helper_->ToString();

  if (updater_ && updater_->IsRunning()) {
    updater_->EndUpdate();
  }

  ProcessAdapterUpdates();

  LIST_LOG << "After ProcessAdapterUpdates.";
  PrintChildren();
  adapter_->PrintViewHolders();
  LIST_LOG << "Children Helper:" << children_helper_->ToString();

  list_state_.item_count = adapter_->GetItemCount();

  SaveFocusNodeInfo();

  // Use `OffsetChildren` to copy the original view location to item's
  // layout_location_.
  OffsetChildren(0.f, 0.f);

  layout_manager_->ScrapFloatingItems(recycler_.get());

  layout_manager_->OnLayoutChildren(*recycler_, list_state_);

  if (sticky_enabled_) {
    layout_manager_->LayoutSticky(recycler_.get(), adapter_);
  }

  if (update_animation_enabled_ && updater_->hasPendingUpdateAnimation()) {
    children_helper_->ForEach([this](ListItemViewHolder* view_holder) -> bool {
      updater_->SaveItemEndAttribute(view_holder);
      view_holder->FlushLayout();
      return false;
    });
    updater_->PerformUpdate();
  } else {
    FlushChildrenLayout();
  }

  list_state_.structure_changed = false;

  layout_manager_->OnLayoutCompleted(*recycler_, list_state_);
  layout_manager_->UpdateItemsVisibility();

  RestoreFocusNodeInfo();

  data_set_has_changed_after_layout_ = false;
  dispatch_items_changed_event_ = false;

  if (delegate_) {
    delegate_->OnListViewDidLayout();
  }
  OnLayoutComplete();
  PrintChildren();
  adapter_->PrintViewHolders();
  LIST_LOG << "Children Helper:" << children_helper_->ToString();
  LIST_LOG << "------ After OnLayout ------";
}

void BaseListView::OnLayout(LayoutContext* context) { DispatchLayout(); }

ListScroller* BaseListView::GetListScroller() {
  if (!scroller_) {
    scroller_ = std::make_unique<ListScroller>(this);
  }
  return scroller_.get();
}

void BaseListView::ResetListScroller() {
  if (scroller_) {
    if (scroller_->Scrolling()) {
      scroller_->StopScroll();
    }
    scroller_ = nullptr;
  }
}

void BaseListView::StopAnimation() {
  NestedScrollable::StopAnimation();
  if (scroller_ && scroller_->Scrolling()) {
    scroller_->StopScroll();
  }
}

FloatPoint BaseListView::DoScroll(FloatPoint delta, bool is_user_input,
                                  bool ignore_repaint) {
  if (!IsScrollEnabled()) {
    return delta;
  }
  unconsumed_x_ = 0.f;
  unconsumed_y_ = 0.f;
  OnScrollBy({-delta.x(), -delta.y()});
  return {unconsumed_x_, unconsumed_y_};
}

bool BaseListView::OnScrollBy(const FloatSize& distance) {
  DCHECK_RET1(adapter_, false);
  // Move the round operation to RenderBox.
  const FloatSize& converted_distance = distance;
  int64_t start = lynx::base::CurrentSystemTimeMilliseconds();
  recycler_->SetListPerfFlag(false);
  // Early out. And this is necessary for sticky to work correctly.
  if (!((converted_distance.width() != 0 &&
         layout_manager_->CanScrollHorizontally()) ||
        (converted_distance.height() != 0 &&
         layout_manager_->CanScrollVertically()))) {
    return false;
  }

  LIST_LOG << "------ Before Scroll ------";
  PrintChildren();
  adapter_->PrintViewHolders();
  LIST_LOG << "Children Helper:" << children_helper_->ToString();
  if (adapter_helper_->HasPendingUpdates()) {
    DispatchLayout();
  }
  // Use `OffsetChildren` to copy the original view location to item's
  // layout_location_.
  OffsetChildren(0.f, 0.f);

  layout_manager_->ScrapFloatingItems(recycler_.get());

  float consumed_x = 0.f, consumed_y = 0.f;
  if (converted_distance.width() != 0 &&
      layout_manager_->CanScrollHorizontally()) {
    consumed_x = layout_manager_->ScrollHorizontallyBy(
        -converted_distance.width(), *recycler_, list_state_);
    unconsumed_x_ = -converted_distance.width() - consumed_x;
  } else if (converted_distance.height() != 0 &&
             layout_manager_->CanScrollVertically()) {
    consumed_y = layout_manager_->ScrollVerticallyBy(
        -converted_distance.height(), *recycler_, list_state_);
    unconsumed_y_ = -converted_distance.height() - consumed_y;
  }

  // ListView only can scroll in one direction
  scrolled_horizontal_ += consumed_x;
  scrolled_vertical_ += consumed_y;
  // Update current scroll offset for prefetch
  current_scroll_height_ += consumed_y;
  current_scroll_width_ += consumed_x;
  // Should not add HasEvent(event_attr::kEventScroll) judgment here, it's
  // always false, because the event is handled by the callback_manager.
  // And it's also handle the kEventScrollToUpper and kEventScrollToLower.
  callback_manager_->NotifyScrolled(FloatPoint(consumed_x, consumed_y),
                                    TotalScrollOffset(),
                                    CalculateBorderStatus());

  // Flush the layout_location_ to the actual view position.
  FlushChildrenLayout();

  if (sticky_enabled_) {
    layout_manager_->LayoutSticky(recycler_.get(), adapter_);
  }

  layout_manager_->UpdateItemsVisibility();

  recycler_->RecycleScrappedItems(this);
  if (recycler_->GetListPerfFlag()) {
    int64_t end = lynx::base::CurrentSystemTimeMilliseconds();
    auto record = FrameTimingCollector::PerfMap();
    record.emplace(Perf::kListLayoutNewItem, end - start);
    if (page_view()->GetFrameTimingCollector()) {
      page_view()->GetFrameTimingCollector()->InsertForceRecord(record);
    }
  }
  recycler_->SetListPerfFlag(false);

  PrintChildren();
  adapter_->PrintViewHolders();
  LIST_LOG << "Children Helper:" << children_helper_->ToString();
  LIST_LOG << "------ End Scroll ------";

  if (consumed_x != 0.f || consumed_y != 0.f) {
    // TODO(liuguoliang): Can we remove this?
    Invalidate();
    DidScroll();

#ifdef ENABLE_ACCESSIBILITY
    // We need to rebuild semantics tree, because some detached views may be
    // attached again. But the semanticsNodes attached to those new attaced
    // views are already detached.
    MarkRebuildSemanticsTree();
#endif

    return true;
  }

  return false;
}

FloatPoint BaseListView::TotalScrollOffset() {
  return GetRenderScroll()->ScrollOffset() +
         FloatPoint(scrolled_horizontal_, scrolled_vertical_);
}

void BaseListView::RegisterPrefetch() {
  if (sAllowListPrefetch && recycler_ && recycler_->GetCacheMaxLimit() > 0) {
    page_view_->GetGapWorker()->RegisterTaskCollector(this,
                                                      gap_task_collector_);
  }
}

void BaseListView::UnregisterPrefetch() {
  if (sAllowListPrefetch) {
    current_scroll_height_ = 0;
    current_scroll_width_ = 0;
    last_add_item_position_when_prefetch_start_ = -1;
    page_view_->GetGapWorker()->UnregisterTaskCollector(this);
  }
}

void BaseListView::StartPrefetch(int32_t width, int32_t height) {
  if ((width == 0 && height == 0) || last_add_item_position_ == -1) {
    return;
  }
  if (last_add_item_position_when_prefetch_start_ == last_add_item_position_) {
    // One long scroll can be broken down into many smaller scrolls. We do this
    // to avoid post too many prefetch requests.
    return;
  }
  if (!gap_task_bundle_) {
    gap_task_bundle_ = fml::MakeRefCounted<GapTaskBundle>(GetWeakPtr());
  }
  // Clean old tasks because position has changed.
  gap_task_bundle_->Clear();

  // Record latest add item position when prefetch start.
  last_add_item_position_when_prefetch_start_ = last_add_item_position_;

  TRACE_EVENT("clay", "BuildPrefetchTaskList");
  prefetch_registry_.ClearPrefetchPositions();
  prefetch_registry_.CollectPrefetchPostionFromView(this, -width, -height);
  for (const auto& item_info : prefetch_registry_.prefetch_item_infos_) {
    int distance_to_item = item_info.second;
    auto type = adapter_->GetItemViewType(item_info.first);
    auto task = std::make_unique<ListPrefetchTask>(
        GetWeakPtr(), item_info.first,
        adapter_->GetAverageBindTime(type).ToNanoseconds(), distance_to_item,
        true);
    gap_task_bundle_->AddTask(std::move(task));
  }
  // This step attempts to prioritize different subtasks by distance. But in
  // the current scenario, the distance of each subtask is the same because
  // they are placed in the same line.
  gap_task_bundle_->sort();
  page_view_->GetGapWorker()->SubmitTask(gap_task_bundle_);
}

bool BaseListView::CanDragScrollOnX() const {
  return layout_manager_->CanScrollHorizontally();
}

bool BaseListView::CanDragScrollOnY() const {
  return layout_manager_->CanScrollVertically();
}

bool BaseListView::IsMoveOnMainAxis(FocusManager::Direction direction) const {
  if (direction == FocusManager::Direction::kLeft ||
      direction == FocusManager::Direction::kRight) {
    return layout_manager_->CanScrollHorizontally();
  }
  if (direction == FocusManager::Direction::kUp ||
      direction == FocusManager::Direction::kDown) {
    return layout_manager_->CanScrollVertically();
  }
  return false;
}

FocusManager::TraversalResult BaseListView::OnTraversalOnScopeInternal(
    FocusManager::Direction direction,
    FocusManager::TraversalType traversal_type) {
  // to pass the test case add two condition
  if (page_view()->GetFrameTimingCollector() &&
      page_view()->GetFrameTimingCollector()->GetReceivedFocusTime() == 0.0) {
    traversal_perf_start_ = lynx::base::CurrentSystemTimeMilliseconds();
  }
  FocusManager::TraversalResult result =
      FocusNode::OnTraversalOnScope(direction, traversal_type);
  if (result.succeed) {
    pre_focus_node_pos_ = ListItemViewHolder::kNoPosition;
    // Reset to idle
    SetScrollStatus(Scrollable::ScrollStatus::kIdle);
    return result;
  }

  if (children_helper_->GetChildCount() == 0) {
    return result;
  }

  const bool to_end = direction == FocusManager::Direction::kDown ||
                      direction == FocusManager::Direction::kRight;
  const bool move_on_main_axis = IsMoveOnMainAxis(direction);

  if (layout_manager_type_ == attr_value::kListTypeSingle &&
      !move_on_main_axis) {
    return result;
  }
  // If the direction is not scrollable or already reach the start/end, the
  // event will be handled by parent.
  if (!to_end && !layout_manager_->HasSpaceToStart(list_state_)) {
    return result;
  } else if (to_end && !layout_manager_->HasSpaceToEnd(list_state_)) {
    return result;
  }
  pre_focus_node_pos_ = ListItemViewHolder::kNoPosition;
  ProcessAdapterUpdates();

  // `OnFocusSearchFailed()` is very similar to `OnLayoutChildren()` which may
  // trigger the Lynx issue. See `FlushChildrenLayout()` for more information.
  // So we cache children's location and flush it back after
  // `OnFocusSearchFailed()`.
  OffsetChildren(0.f, 0.f);
  layout_manager_->OnFocusSearchFailed(to_end, *recycler_, list_state_);
  FlushChildrenLayout();

  // After calling `OnFocusSearchFailed()`, more items are laid out even out of
  // the visible area of the list view. We will try to find a focusable node
  // again.
  result = FocusNode::OnTraversalOnScope(direction, traversal_type);
  // Reset to idle
  SetScrollStatus(Scrollable::ScrollStatus::kIdle);
  if (!scroll_without_focus_ || result.succeed) {
    return result;
  }

  if (!move_on_main_axis) {
    // cross-axis-moving-caused load more shouldn't do scroll on main axis.
    FML_DCHECK(layout_manager_type_ != attr_value::kListTypeSingle);
    return result;
  }

  int target_pos = ListItemViewHolder::kNoPosition;
  if (to_end) {
    ListItemViewHolder* last = layout_manager_->GetLastInBoxChild(list_state_);
    FML_DCHECK(last != nullptr);
    target_pos = std::min(last->GetPosition() + 1,
                          static_cast<int>(list_state_.item_count));
  } else {
    ListItemViewHolder* first =
        layout_manager_->GetFirstInBoxChild(list_state_);
    FML_DCHECK(first != nullptr);
    target_pos = std::max(first->GetPosition() - 1, 0);
  }
  ScrollToPosition(false, target_pos, 0, AlignTo::kNone, "", nullptr);
  result.succeed = true;
  // Call `UpdateFocusRing` to set focus ring back to list when previous
  // focus node be removed
  UpdateFocusRing();
  return result;
}

FocusManager::TraversalResult BaseListView::OnTraversalOnScope(
    FocusManager::Direction direction,
    FocusManager::TraversalType traversal_type) {
  auto result = OnTraversalOnScopeInternal(direction, traversal_type);
  if (result.succeed && page_view() && page_view()->GetFrameTimingCollector()) {
    page_view()->GetFrameTimingCollector()->SetReceivedFocusTime(
        traversal_perf_start_, direction);
  }
  traversal_perf_start_ = 0;
  return result;
}

void BaseListView::OnFlingAnimationStart() { page_view()->OnFlingStart(); }

void BaseListView::OnFlingAnimationEnd() { page_view()->OnFlingEnd(); }

void BaseListView::OnItemRangeInserted(int position_start, int item_count) {
  // Save insert operation to `adapter_helper_`, coalescence and consume
  // in next layout pass.
  adapter_helper_->OnItemRangeInserted(position_start, item_count);
  list_state_.item_count = adapter_->GetItemCount();
  MarkNeedsLayout();
}

void BaseListView::OnItemRangeRemoved(int position_start, int item_count) {
  // Save remove operation to `adapter_helper_`, coalescence and consume
  // in next layout pass.
  adapter_helper_->OnItemRangeRemoved(position_start, item_count);
  list_state_.item_count = adapter_->GetItemCount();
  MarkNeedsLayout();
}

void BaseListView::OnItemRangeChanged(
    int position_start, int item_count,
    std::unique_ptr<ListAdapter::Payload> payload) {
  // Save reload operation to `adapter_helper_`, coalescence and consume
  // in next layout pass.
  adapter_helper_->OnItemRangeChanged(position_start, item_count,
                                      std::move(payload));
  MarkNeedsLayout();
}

void BaseListView::OnItemMoved(int from_position, int to_position) {
  // Save move operation to `adapter_helper_`, coalescence and consume
  // in next layout pass.
  adapter_helper_->OnItemMoved(from_position, to_position);
  MarkNeedsLayout();
}

void BaseListView::OnChanged() {
  list_state_.structure_changed = true;
  ProcessDataSetCompletelyChanged(true);
  MarkNeedsLayout();
}

// Mark: - Consuming ListAdapter data update begin.
void BaseListView::OffsetPositionsForInsert(int position_start,
                                            int item_count) {
  children_helper_->ForEachWithSticky(
      [this, position_start, item_count](ListItemViewHolder* view_holder) {
        if (view_holder->GetPosition() >= position_start &&
            !view_holder->IsRemoved()) {
          // Visible item after `position_start` will be moved `item_count`
          // step(s).
          view_holder->OffsetPosition(item_count);
          list_state_.structure_changed = true;
          if (update_animation_enabled_) {
            // save start origin
            updater_->SaveItemStartAttribute(view_holder);
          }
        }
        return false;
      });
  recycler_->OffsetPositionsForInsert(position_start, item_count);
  layout_manager_->OnItemsAdded(this, position_start, item_count);
  if (update_animation_enabled_) {
    for (int i = position_start; i < position_start + item_count; ++i) {
      updater_->AddInsertItemPosition(i);
    }
  }
}

void BaseListView::OffsetPositionsForRemove(int position_start,
                                            int item_count) {
  const int position_end = position_start + item_count;
  children_helper_->ForEachWithSticky(
      [this, position_start, item_count,
       position_end](ListItemViewHolder* view_holder) {
        if (view_holder->GetPosition() >= position_end) {
          view_holder->OffsetPosition(-item_count, true);
          list_state_.structure_changed = true;

          if (update_animation_enabled_ && !view_holder->IsRemoved()) {
            updater_->SaveItemStartAttribute(view_holder);
          }
        } else if (view_holder->GetPosition() >= position_start) {
          view_holder->FlagRemovedAndOffsetPosition(position_start - 1,
                                                    -item_count, true);
          list_state_.structure_changed = true;
        } else {
          layout_manager_->OnRemoveItem(view_holder);
        }
        return false;
      });
  recycler_->OffsetPositionsForRemove(position_start, item_count);
  layout_manager_->OnItemsRemoved(this, position_start, item_count);

  // Current focused node gets deleted.
  if (pre_focus_node_pos_ <= position_end &&
      pre_focus_node_pos_ >= position_start) {
    pre_focus_node_deleted_ = true;
  }
}

void BaseListView::MarkViewHoldersChanged(
    int position_start, int item_count,
    std::unique_ptr<ListAdapter::Payload> payload) {
  const int position_end = position_start + item_count;

  std::optional<int> update_to_position = std::nullopt;
  if (payload && payload->is_lynx_payload) {
    auto* lynx_list_payload = static_cast<LynxListPayload*>(payload.get());
    update_to_position = lynx_list_payload->update_to_position;
  }

  children_helper_->ForEachWithSticky([position_start, position_end, &payload](
                                          ListItemViewHolder* view_holder) {
    if (view_holder->GetPosition() >= position_start &&
        view_holder->GetPosition() < position_end) {
      // Visible Item between [position_start, position_end) will be flagged
      // as update and store `payload`.
      view_holder->AddFlags(ListItemViewHolder::kFlagUpdate);
      view_holder->AddChangePayload(std::move(payload));
    }
    return false;
  });
  recycler_->MarkViewHoldersChanged(position_start, item_count);
  if (update_to_position && *update_to_position != position_start) {
    // Update operation from Lynx will have `from` and `to`, so we call move
    // instead of update.
    layout_manager_->OnItemsMoved(this, position_start, *update_to_position,
                                  item_count);
  } else {
    layout_manager_->OnItemsUpdated(this, position_start, item_count);
  }
}

void BaseListView::OffsetPositionsForMove(int from, int to) {
  int start, end, in_between_offset;
  if (from < to) {
    start = from;
    end = to;
    in_between_offset = -1;
  } else {
    start = to;
    end = from;
    in_between_offset = 1;
  }

  children_helper_->ForEachWithSticky(
      [this, from, to, start, end,
       in_between_offset](ListItemViewHolder* view_holder) {
        const int position = view_holder->GetPosition();
        if (position < start || position > end) {
          return false;
        }
        if (position == from) {
          view_holder->OffsetPosition(to - from, false);
        } else {
          view_holder->OffsetPosition(in_between_offset, false);
        }
        if (update_animation_enabled_) {
          updater_->SaveItemStartAttribute(view_holder);
        }
        list_state_.structure_changed = true;
        return false;
      });
  recycler_->OffsetPositionsForMove(from, to);
  layout_manager_->OnItemsMoved(this, from, to, 1);
}

bool BaseListView::IsItemFullSpan(int position) const {
  return adapter_->IsItemFullSpan(position);
}

void BaseListView::FlushChildrenLayout() {
  children_helper_->ForEach([](ListItemViewHolder* child) {
    child->FlushLayout();
    return false;
  });
}

bool BaseListView::OnScrollToVisible() {
  FloatRect focused_view_rect = GetFocusManager()->GetFocusedNodeRect();
  if (!focused_view_rect.IsEmpty()) {
    MakeViewInBox(focused_view_rect, align_focus_);
  }
  return true;
}

void BaseListView::MakeViewInBox(const FloatRect& focus_view_rect,
                                 AlignTo align_to) {
  if (focus_smooth_scroll_) {
    auto res = layout_manager_->GetChildPositionByRect(focus_view_rect);
    if (res) {
      auto& pos_and_rect = *res;
      ScrollToPosition(true, pos_and_rect.first, 0, align_to, "",
                       pos_and_rect.second, nullptr);
    } else {
      FML_DCHECK(false) << "position not found with the specified rect";
    }
  } else {
    // When user traverse focus node fast and `focus_view_rect` is totally
    // invisible, stop animation firstly and scroll without animation.
    GetListScroller()->StopScroll();
    // TODO(Chenfeng Pan): implement waterfall layout manager.
    FloatSize to_scroll =
        layout_manager_->ScrollToRect(focus_view_rect, align_to);
    to_scroll.SetHeight(-to_scroll.height());
    to_scroll.SetWidth(-to_scroll.width());
    OnScrollBy(to_scroll);
  }
}

void BaseListView::SaveFocusNodeInfo() {
  auto* focused = static_cast<BaseView*>(focus_manager_->GetLeafFocusedNode());
  if (focused) {
    if (pre_focus_node_deleted_) {
      LIST_LOG << "Focused node was removed, set focus to list";
      focused->ClearFocus();
      UpdateFocusRing();
      return;
    }
    focus_node_cache_ = focused;
    children_helper_->ForEach([this](ListItemViewHolder* child) {
      if (child->GetView()->IsDescendant(focus_node_cache_)) {
        focus_node_pos_ = child->GetPosition();
        return true;
      }
      return false;
    });
    focus_node_attached_ = true;

    focused->AddLifecycleListener(this);
  }
}

void BaseListView::RestoreFocusNodeInfo() {
  if (focus_node_cache_ != nullptr && focus_node_attached_ &&
      focus_node_cache_->attach_to_tree_) {
    focus_node_cache_->RequestFocus();
  } else if (focus_node_pos_ != ListItemViewHolder::kNoPosition) {
    LIST_LOG << "Focused node was removed, set focus to list";
    UpdateFocusRing();
  }

  ClearFocusNodeInfo();
}

void BaseListView::OnFocusNodeDestructed(FocusNode* node) {
  if (node == focus_node_cache_) {
    focus_node_cache_ = nullptr;
  }
}

void BaseListView::ClearFocusNodeInfo() {
  pre_focus_node_pos_ = focus_node_pos_;
  pre_focus_node_deleted_ = false;
  if (focus_node_cache_) {
    focus_node_cache_->RemoveLifecycleListener(this);
    focus_node_cache_ = nullptr;
  }
  focus_node_pos_ = ListItemViewHolder::kNoPosition;
  focus_node_attached_ = false;
}

void BaseListView::OnVisibilityChanged(ListItemViewHolder* item, bool visible) {
  if (visible) {
    callback_manager_->OnItemAppear(item);
  } else {
    callback_manager_->OnItemDisappear(item);
  }
}

void BaseListView::ProcessDataSetCompletelyChanged(bool dispatch_item_changed) {
  dispatch_items_changed_event_ |= dispatch_item_changed;
  data_set_has_changed_after_layout_ = true;
  MarkKnownViewsInvalid();
}

void BaseListView::MarkKnownViewsInvalid() {
  children_helper_->ForEach([](ListItemViewHolder* child) {
    child->AddFlags(static_cast<ListItemViewHolder::Flag>(
        ListItemViewHolder::kFlagInvalid | ListItemViewHolder::kFlagUpdate));
    return false;
  });
  recycler_->MarkKnownViewsInvalid();
}

void BaseListView::ProcessAdapterUpdates() {
  if (data_set_has_changed_after_layout_) {
    adapter_helper_->Reset();

    if (dispatch_items_changed_event_) {
      layout_manager_->OnItemsChanged(this);
    }
  }

  adapter_helper_->ConsumeUpdates();
}

ScrollEventCallbackManager::BorderStatus BaseListView::CalculateBorderStatus() {
  ScrollEventCallbackManager::BorderStatus border_status;
  if (lower_threshold_item_count_ || upper_threshold_item_count_) {
    int first_child_position = std::numeric_limits<int>::max();
    int last_child_position = std::numeric_limits<int>::min();
    children_helper_->ForEach([&first_child_position, &last_child_position](
                                  ListItemViewHolder* child) {
      int position = child->GetPosition();
      first_child_position = std::min(first_child_position, position);
      last_child_position = std::max(last_child_position, position);
      return false;
    });
    if (upper_threshold_item_count_ &&
        first_child_position < *upper_threshold_item_count_) {
      border_status.SetUpper(true);
    }
    if (lower_threshold_item_count_ &&
        last_child_position >
            adapter_->GetItemCount() - *lower_threshold_item_count_ - 1) {
      border_status.SetLower(true);
    }
  }

  bool first_item_visible = children_helper_->FindChildByPosition(0) != nullptr;
  bool last_item_visible = children_helper_->FindChildByPosition(
                               adapter_->GetItemCount() - 1) != nullptr;
  if (first_item_visible || last_item_visible) {
    // the min and max Y-offset of visible cells.
    float top_bound = std::numeric_limits<float>::max();
    float bottom_bound = std::numeric_limits<float>::lowest();
    ListOrientationHelper* orientation_helper =
        layout_manager_->GetOrientationHelper();
    children_helper_->ForEach([&top_bound, &bottom_bound,
                               orientation_helper](ListItemViewHolder* child) {
      top_bound =
          std::min(top_bound, orientation_helper->GetDecoratedStart(child));
      bottom_bound =
          std::max(bottom_bound, orientation_helper->GetDecoratedEnd(child));
      return false;
    });

    int start_of_visible_area = orientation_helper->GetStartAfterPadding();
    int end_of_visible_area = orientation_helper->GetEndAfterPadding();
    if (first_item_visible) {
      if (top_bound == start_of_visible_area) {
        scrolled_vertical_ = 0;
        scrolled_horizontal_ = 0;
      }
      // check if the top border threshold-px reached
      if (top_bound > start_of_visible_area - upper_threshold_pixel_) {
        border_status.SetUpper(true);
      }
    }
    if (last_item_visible) {
      // check if the bottom border threshold-px reached
      if (bottom_bound < end_of_visible_area + lower_threshold_pixel_) {
        border_status.SetLower(true);
      }
    }
  }

  return border_status;
}

void BaseListView::SetScrollState(ScrollState state) {
  if (state == scroll_state_) {
    return;
  }
  LIST_LOG << "Setting scroll from " << static_cast<int>(scroll_state_)
           << " to " << static_cast<int>(state);
  scroll_state_ = state;
  if (state != ScrollState::kSettling) {
    StopAnimation();
  }
  callback_manager_->OnScrollStateChange(state);
}

void BaseListView::OnScrollStatusChange(ScrollStatus old_status) {
  NestedScrollable::OnScrollStatusChange(old_status);

  if (status_ == Scrollable::ScrollStatus::kFling) {
    OnFlingAnimationStart();
  } else if (old_status == Scrollable::ScrollStatus::kFling) {
    OnFlingAnimationEnd();
  }

  bool need_prefetch = status_ == Scrollable::ScrollStatus::kFling ||
                       status_ == Scrollable::ScrollStatus::kDragging ||
                       status_ == Scrollable::ScrollStatus::kAnimating;
  bool old_need_prefetch = old_status == Scrollable::ScrollStatus::kFling ||
                           old_status == Scrollable::ScrollStatus::kDragging ||
                           old_status == Scrollable::ScrollStatus::kAnimating;
  if (need_prefetch && !old_need_prefetch) {
    RegisterPrefetch();
  } else if (!need_prefetch && old_need_prefetch) {
    UnregisterPrefetch();
  }

  switch (status_) {
    case Scrollable::ScrollStatus::kFling:
    case Scrollable::ScrollStatus::kBounce:
      SetScrollState(ScrollState::kSettling);
      break;
    case Scrollable::ScrollStatus::kDragging:
      SetScrollState(ScrollState::kDragging);
      break;
    case Scrollable::ScrollStatus::kIdle:
      SetScrollState(ScrollState::kIdle);
      break;
    default:
      break;
  }

  if (layout_manager_) {
    layout_manager_->OnScrollStateChange(status_);
  }
}

void BaseListView::ScrollToPosition(
    bool smooth, int position, float offset, AlignTo align_to,
    const std::string& id, const std::optional<FloatRect> target_rect,
    const std::function<void(uint32_t, const std::string&)>& callback) {
  if (position < 0 || position >= adapter_->GetItemCount()) {
    if (callback) {
      callback(static_cast<uint32_t>(LynxUIMethodResult::kParamInvalid), "");
    }
    return;
  }

  SetScrollStatus(Scrollable::ScrollStatus::kIdle);
  GetListScroller()->ScrollToPosition(smooth, position, offset, align_to, id,
                                      target_rect, callback);
}

void BaseListView::ScrollToPosition(
    bool smooth, int position, float offset, AlignTo align_to,
    const std::string& id,
    const std::function<void(uint32_t, const std::string&)>& callback) {
  ScrollToPosition(smooth, position, offset, align_to, id, std::nullopt,
                   callback);
}

void BaseListView::SetStickyEnabled(bool enabled) {
  if (sticky_enabled_ != enabled) {
    sticky_enabled_ = enabled;
    MarkNeedsLayout();
  }
}

void BaseListView::OnListItemSizeChanged() {
  if (ShouldIgnoreLayoutRequest()) {
    return;
  }
  if (layout_manager_) {
    layout_manager_->InvalidateLayoutCache();
  }
  MarkNeedsLayout();
}

#ifdef ENABLE_ACCESSIBILITY
int32_t BaseListView::GetSemanticsActions() const {
  int32_t actions = BaseView::GetSemanticsActions();
  const ScrollableDirection direction = GetScrollableDirection();
  if (CanDragScrollOnY()) {
    if ((direction & ScrollableDirection::kUpwards) !=
        ScrollableDirection::kNone) {
      actions |=
          static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollUp);
    }
    if ((direction & ScrollableDirection::kDownwards) !=
        ScrollableDirection::kNone) {
      actions |=
          static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollDown);
    }
  } else if (CanDragScrollOnX()) {
    if ((direction & ScrollableDirection::kLeftwards) !=
        ScrollableDirection::kNone) {
      actions |=
          static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollLeft);
    }
    if ((direction & ScrollableDirection::kRightwards) !=
        ScrollableDirection::kNone) {
      actions |=
          static_cast<int32_t>(SemanticsNode::SemanticsAction::kScrollRight);
    }
  } else {
    FML_DLOG(ERROR) << "BaseListView cannot draw scroll both on X and Y axis";
  }
  return actions;
}

int32_t BaseListView::GetSemanticsFlags() const {
  int32_t flags = BaseView::GetSemanticsFlags();
  flags |=
      static_cast<int32_t>(SemanticsNode::SemanticsFlag::kHasImplicitScrolling);
  return flags;
}

#endif

bool BaseListView::OnScrollToMiddle(BaseView* target_view) {
  if (!target_view) {
    FML_DLOG(ERROR) << "OnScrollToMiddle but view is nullptr";
    return false;
  }
  FloatRect rect = target_view->GetBounds();
  MakeViewInBox(rect, AlignTo::kMiddle);
  return true;
}

ScrollableDirection BaseListView::GetScrollableDirection() const {
  ScrollableDirection result = ScrollableDirection::kNone;
  if (CanDragScrollOnX()) {
    if (layout_manager_->HasSpaceToStart(list_state_)) {
      result |= ScrollableDirection::kLeftwards;
    }
    if (layout_manager_->HasSpaceToEnd(list_state_)) {
      result |= ScrollableDirection::kRightwards;
    }
  }
  if (CanDragScrollOnY()) {
    if (layout_manager_->HasSpaceToStart(list_state_)) {
      result |= ScrollableDirection::kUpwards;
    }
    if (layout_manager_->HasSpaceToEnd(list_state_)) {
      result |= ScrollableDirection::kDownwards;
    }
  }
  return result;
}

void BaseListView::PrintChildren() {
#if (DEBUG_LIST)
  auto print_child = [](std::string& str, int i, BaseView* child,
                        bool visible) {
    std::stringstream ss;
    ss << "[" << i << "] " << child->GetName() << " view_id:" << child->id();
    if (visible) {
      ss << " frame:(" << child->Left() << "," << child->Top() << ","
         << child->Width() << "," << child->Height() << ")";
    }
    str.append(ss.str());
  };
  std::string attached_str;
  for (size_t i = 0; i < child_count(); ++i) {
    attached_str += "\n";
    print_child(attached_str, i, children_[i], true);
  }
  LIST_LOG << "list attached children:" << attached_str;
  std::string detached_str;
  for (size_t i = 0; i < detached_children_.size(); ++i) {
    detached_str += "\n";
    print_child(detached_str, i, detached_children_[i], false);
  }
  LIST_LOG << "list detached children:" << detached_str;
#endif
}

std::vector<float> BaseListView::GestureScrollBy(float delta_x, float delta_y) {
  FloatPoint unconsumed_offset = DoScroll({delta_x, delta_y}, false);
  std::vector<float> res(4, 0);
  bool is_horizontal = (scroll_orientation_ == ScrollDirection::kHorizontal);
  res[0] = is_horizontal ? unconsumed_offset.x() : 0;
  res[1] = is_horizontal ? 0 : unconsumed_offset.y();
  res[2] = is_horizontal ? (delta_x - unconsumed_offset.x()) : delta_x;
  res[3] = is_horizontal ? delta_y : (delta_y - unconsumed_offset.y());
  return res;
}

}  // namespace clay
