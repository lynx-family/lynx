// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_BASE_LIST_VIEW_H_
#define CLAY_UI_COMPONENT_LIST_BASE_LIST_VIEW_H_

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/fml/memory/ref_ptr.h"
#include "clay/gfx/animation/fling_animator.h"
#include "clay/gfx/scroll_direction.h"
#include "clay/ui/common/gap_task.h"
#include "clay/ui/common/gap_worker.h"
#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/list_adapter.h"
#include "clay/ui/component/list/list_adapter_helper.h"
#include "clay/ui/component/list/list_common/layout_types.h"
#include "clay/ui/component/list/list_item_view_holder.h"
#include "clay/ui/component/list/list_scroller.h"
#include "clay/ui/component/nested_scroll/nested_scrollable.h"
#include "clay/ui/component/scrollable.h"
#include "clay/ui/component/view_callback/list_event_callback_manager.h"
#include "clay/ui/gesture/drag_gesture_recognizer.h"
#include "third_party/googletest/googletest/include/gtest/gtest_prod.h"  // nogncheck

namespace clay {

class ListChildrenHelper;
class ListItemViewHolder;
class ListLayoutManager;
class ListRecycler;
class ListAdapterUpdater;
class GapWorker;

void SetAllowListPrefetch(bool allow);

class LayoutPrefetchRegistry {
 public:
  void AddPosition(int layout_position, int pixel_distance);
  void CollectPrefetchPostionFromView(BaseListView* list_view,
                                      int prefetch_delta_x,
                                      int prefetch_delta_y);
  bool LastPrefetchIncludePosition(int position);
  void ClearPrefetchPositions();

 private:
  LayoutPrefetchRegistry() = default;

  friend class BaseListView;
  std::unordered_map<int, int> prefetch_item_infos_;
};

class BaseListView : public WithTypeInfo<BaseListView, NestedScrollable>,
                     public ListAdapter::Observer,
                     public ListAdapterHelper::Consumer,
                     public ListItemViewHolder::VisibilityObserver,
                     public FocusNode::LifecycleListener {
 public:
  class Delegate {
   public:
    virtual void OnListViewDidLayout() = 0;
  };

  using ScrollState = ListEventCallbackManager::ScrollState;

  BaseListView(int32_t id, std::string tag, PageView* page_view);
  BaseListView(int32_t id, int32_t callback_id, std::string tag,
               PageView* page_view);
  virtual ~BaseListView();

  int GetCallbackId() override { return callback_id_; }

  bool IsLayoutRootCandidate() const override { return true; }

  // Post a task that will only run when the list is not scrolling. Run the task
  // immediately if there is no scrolling, otherwise run it after scrolling.
  void PostLowPriorityTask(std::function<void()> task);

  void SetWidth(float width) override;
  void SetHeight(float height) override;

  void SetAdapter(ListAdapter* adapter);
  void SetLayoutManager(std::unique_ptr<ListLayoutManager> layout_manager);

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void AddEventCallback(const char* event_c) override;
  void UpdateLayoutManager(const std::string& type, int span_count);
  void NotifyLowMemory() override;

  ////////////////// Override BaseView Start //////////////
  // List View uses different model to manage its subview. Calling `AddChild`
  // will add the child to a detached children list.
  // A detach child means either it is just created or it is recycled.
  // Child in detached children list won't be rendered.
  // It is the layout manager to determine when to attach a child view. Then the
  // child will be moved from the detached list to the actual children list.
  void AddChild(BaseView* child, int index) override;
  // Remove the child from the detached list. Then the child may be deleted or
  // added to the actual children list.
  void RemoveChild(BaseView* child) override;
  void DestroyAllChildren() override;

  // NOTE(Xietong): cannot override `GetScrollOffset()` because
  // `GetScrollOffset()` is used by hittest. Overriding `GetScrollOffset()` will
  // affect hittest.
  float GetScrollbarScrollOffset();
  float GetTotalScrollLength();

  ////////////////// Override BaseView End ////////////////

  /////////// Called by ListLayoutManager Start ///////////
  // Called when an item is created.
  // To align with Lynx's behaviour, `OnCreateItem` won't be called during
  // adapter's `CreateListItem`. Instead, it is delayed to `BindListItem` so
  // that we know the actual position of the item.
  void OnCreateItem(ListItemViewHolder* item, int position);
  // An item is added to the list by the list manager. The corresponding view
  // should be attached.
  void OnAddItem(ListItemViewHolder* item, int index);
  // An item is removed to the list by the list manager. The corresponding view
  // should be detached.
  void OnRemoveItem(ListItemViewHolder* item);
  // An item is cleanup by the recycler. The corresponding view should be
  // destroyed.
  void OnDestroyItem(ListItemViewHolder* item);
  // Note(Xietong): Different from OnCreate/Add/Remove/DestroyItem, which
  // BaseListView really does manipulate the item views. This method is used to
  // update the focus logic. The actual binding logic is handled by list
  // adapter.
  void OnBindListItem(ListItemViewHolder* item, int prev_position, int position,
                      bool newly_created);

  void OffsetChildren(float x, float y);
  /////////// Called by ListLayoutManager End /////////////

  void OnLayout(LayoutContext* context) override;
  void DispatchLayout();

  bool OnScrollBy(const FloatSize& distance);

  // Return scroll offset including overscroll
  FloatPoint TotalScrollOffset();

  // Override FocusNode
  FocusManager::TraversalResult OnTraversalOnScope(
      FocusManager::Direction direction,
      FocusManager::TraversalType traversal_type) override;

  // Override ListAdapter::Observer
  void OnItemRangeInserted(int position_start, int item_count) override;
  void OnItemRangeRemoved(int position_start, int item_count) override;
  void OnItemRangeChanged(
      int position_start, int item_count,
      std::unique_ptr<ListAdapter::Payload> payload) override;
  void OnItemMoved(int from_position, int to_position) override;
  void OnChanged() override;

  // Override ListAdapterHelper::Consumer
  void OffsetPositionsForInsert(int position_start, int item_count) override;
  void OffsetPositionsForRemove(int position_start, int item_count) override;
  void MarkViewHoldersChanged(
      int position_start, int item_count,
      std::unique_ptr<ListAdapter::Payload> payload) override;
  void OffsetPositionsForMove(int from, int to) override;

  bool IsItemFullSpan(int position) const;

  // Override ListItemViewHolder::VisibilityObserver
  // Dispatch the notification to callback manager.
  void OnVisibilityChanged(ListItemViewHolder* item, bool visible) override;

  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

  /**
   * @brief UI method, scroll list to specified `position` item with `offset`
   * and `align_to`. And even item's child specified by `id` is supported.
   *
   * @param smooth whether enable animation or not.
   * @param position item's index in list.
   * @param offset negative means scrolling to end with abs(offset_),
   * positive has opposite effect.
   * @param align_to how target item or item's child align with list.
   * @param id item's child, can be omitted.
   * @param callback
   */
  void ScrollToPosition(
      bool smooth, int position, float offset, AlignTo align_to,
      const std::string& id,
      const std::function<void(uint32_t, const std::string&)>& callback);

  ListLayoutManager* GetLayoutManager() const { return layout_manager_.get(); }

  void SetStickyEnabled(bool enabled);
  void OnFocusNodeDestructed(FocusNode*) override;

  void StopAnimation() override;
  FloatPoint DoScroll(FloatPoint delta, bool is_user_input = true,
                      bool ignore_repaint = false) override;
  bool CanDragScrollOnX() const override;
  bool CanDragScrollOnY() const override;
  void NotifyScrollAnimationStart() {
    SetScrollStatus(Scrollable::ScrollStatus::kAnimating);
  }
  void NotifyScrollAnimationEnd() {
    SetScrollStatus(Scrollable::ScrollStatus::kIdle);
  }
  void PrintChildren();

#ifdef ENABLE_ACCESSIBILITY
  int32_t GetSemanticsActions() const override;
  int32_t GetSemanticsFlags() const override;
  int32_t GetA11yScrollChildren() const override;
#endif

  bool OnScrollToMiddle(BaseView* target_view) override;

  ScrollableDirection GetScrollableDirection() const override;

 protected:
  // Create a view and added to the end of children list.
  // Assume item's position has been set.
  virtual BaseView* HandleCreateView(ListItemViewHolder* item) = 0;
  virtual void HandleDestroyView(BaseView* to_destroy,
                                 ListItemViewHolder* item) = 0;
  void StartPrefetch(int32_t width, int32_t height);
  void RegisterPrefetch();
  void UnregisterPrefetch();
  void UpdateScrollOrientation(ScrollDirection scroll_orientation);
  /**
   * Maintain scroll state. Make sure every time scroll state change will
   * call to this, it should handle stop scroll cleanup and send notify to
   * `layout_manager_`/`callback_manager_`
   */
  // TODO(hanhaoshen) make sure SetScrollState be called when status change
  // TODO(liuguoliang): merge with ScrollStatus in Scrollable
  void SetScrollState(ScrollState state);

  void OnScrollStatusChange(ScrollStatus old_status) override;

  virtual void OnLayoutComplete() {}

  /**
   * Consumes adapter updates and calculates which type of animations we want to
   * run. Called in onMeasure and dispatchLayout.
   * <p> This method may process only the pre-layout state of updates or all of
   * them.
   */
  virtual void ProcessAdapterUpdates();

  // attribute set from client
  std::string layout_manager_type_ = attr_value::kListTypeSingle;
  int layout_span_count_ = 1;

  ScrollDirection scroll_orientation_ = ScrollDirection::kVertical;

  // view id that used as callback arguments. This should be the id of the
  // wrapper view. With callback_id_, Lynx will think the callback is sent from
  // the wrapper (which is known by Lynx) instead of the actual list view.
  int32_t callback_id_ = -1;

 private:
  void OnDestroy() override;
  void OnFlingAnimationStart();
  void OnFlingAnimationEnd();

  void ScrollToPosition(
      bool smooth, int position, float offset, AlignTo align_to,
      const std::string& id, const std::optional<FloatRect> target_rect,
      const std::function<void(uint32_t, const std::string&)>& callback);
  // This is to workaround an issue of Lynx:
  // The location of the item view supposes to be determined by the list view.
  // During the layout process, however, Lynx may set a wrong location for the
  // item view.
  // The solution is to maintain the latest location set by the list view to a
  // cache during the layout process and flush the cache value to the actual
  // location after the layout process.
  void FlushChildrenLayout();

  /**
   * @brief Try to scroll the list view so that the view (could be an item view
   * or a descendant of an item view) is completely visible (in box). If the
   * view is larger than the frame box of the list view, the view will be
   * partially visible with one border aligning with the list view.
   *
   * @param focus_view_rect view's rect relative to list
   * @param align_to same as in `ScrollToPosition`
   */
  void MakeViewInBox(const FloatRect& focus_view_rect, AlignTo align_to);
  bool OnScrollToVisible() override;

  // Methods used to maintain the focus node in the list view.
  // During the layout process of list view, item views will be detached and may
  // be attached again. The re-attached item may or may not correspond to the
  // same position.
  // When a view is detached the focus info will be cleared so the focus will
  // lost even it is attached again in the same layout pass.
  //
  // For list view, if the re-attached view corresponds to the same position, we
  // should recover the focus info after the view is attached.
  // The current implementation is to save the focus node info before layout. If
  // there is any update during the layout claims that the focus node should be
  // cleared, then we cleanup the saved info. After the layout process, we check
  // if the cached info is still valid, and restore it if so.
  void SaveFocusNodeInfo();
  void RestoreFocusNodeInfo();
  void ClearFocusNodeInfo();

  bool IsMoveOnMainAxis(FocusManager::Direction direction) const;

  /**
   * Processes the fact that, as far as we can tell, the data set has completely
   * changed.
   *
   * <ul>
   *   <li>Once layout occurs, all attached items should be discarded or
   *       animated.
   *   <li>Attached items are labeled as invalid.
   *   <li>Because items may still be prefetched between a "data set completely
   *       changed" event and a layout event, all cached items are discarded.
   * </ul>
   *
   * @param dispatch_item_changed Whether to call `onItemsChanged` during
   *                              measure/layout.
   */
  void ProcessDataSetCompletelyChanged(bool dispatch_item_changed);
  /**
   * Mark all known views as invalid. Used in response to a, "the whole world
   * might have changed" data change event.
   */
  void MarkKnownViewsInvalid();

  /**
   * Calculate border status from current layout status.
   */
  ScrollEventCallbackManager::BorderStatus CalculateBorderStatus();

  // Returns List Scroller. List scroller is created lazily.
  ListScroller* GetListScroller();
  void ResetListScroller();

  void FlushTasks();

  FocusManager::TraversalResult OnTraversalOnScopeInternal(
      FocusManager::Direction direction,
      FocusManager::TraversalType traversal_type);

  class ListPrefetchTask : public GapTask {
   public:
    void Run() override;
    using GapTask::GapTask;
  };

  FRIEND_TEST(ListLayoutManagerLinearTest, Scroll);
  FRIEND_TEST(ListLayoutManagerLinearTest, ScrollHorizontally);
  FRIEND_TEST(ListLayoutManagerLinearTest, ScrollDiffHeight);
  FRIEND_TEST(ListLayoutManagerLinearTest, RemoveData);
  FRIEND_TEST(ListLayoutManagerLinearTest, ScrollToPositionImmediately);
  FRIEND_TEST(ListLayoutManagerLinearTest, DISABLED_ScrollToPositionSmooth);
  FRIEND_TEST(ListLayoutManagerLinearTest,
              DISABLED_ScrollToPositionSmoothDataChange);
  FRIEND_TEST(BaseListView, Fling);
  FRIEND_TEST(ListLayoutManagerGridTest, Scroll);
  FRIEND_TEST(ListLayoutManagerStaggeredGridTest, Scroll);
  FRIEND_TEST(ListLayoutManagerStaggeredGridTest, Fling);
  FRIEND_TEST(FocusListViewTest, AppearDisappearEvent);
  FRIEND_TEST(FocusListViewTest, EventFindFocus);
  FRIEND_TEST(FocusListViewTest, EventThrottling);

  friend class ListScroller;
  friend class LayoutPrefetchRegistry;

  bool data_set_has_changed_after_layout_ = false;

  ListViewState list_state_;
  ScrollState scroll_state_ = ScrollState::kIdle;
  // Defines the behavior when focus moving between list items, where the
  // focused item should be positioned.
  AlignTo align_focus_ = AlignTo::kNone;

  std::unique_ptr<ListChildrenHelper> children_helper_;
  ListAdapter* adapter_ = nullptr;
  std::unique_ptr<ListAdapterHelper> adapter_helper_;
  std::unique_ptr<ListLayoutManager> layout_manager_;
  std::unique_ptr<ListRecycler> recycler_;

  std::vector<BaseView*> detached_children_;

  float scrolled_vertical_ = 0.f;
  float scrolled_horizontal_ = 0.f;
  float lower_threshold_pixel_ = 0.f;
  float upper_threshold_pixel_ = 0.f;
  std::optional<int> lower_threshold_item_count_;
  std::optional<int> upper_threshold_item_count_;
  std::unique_ptr<ListEventCallbackManager> callback_manager_;

  // The cached information of the focus node inside the list view.
  BaseView* focus_node_cache_ = nullptr;
  int focus_node_pos_ = ListItemViewHolder::kNoPosition;
  bool focus_node_attached_ = false;
  // Keep track the previous focused node,to check whether got deleted during
  // update
  int pre_focus_node_pos_ = ListItemViewHolder::kNoPosition;
  bool pre_focus_node_deleted_ = false;

  // Enable smooth scroll when focus change inside list.
  bool focus_smooth_scroll_ = true;
  // Whether scroll to next item even if the item is not focusable.
  bool scroll_without_focus_ = false;

  bool dispatch_items_changed_event_ = false;

  std::unique_ptr<ListScroller> scroller_;

  std::vector<std::function<void()>> pending_tasks_;
  std::unique_ptr<fml::Timer> task_timer_;

  Delegate* delegate_ = nullptr;

  bool sticky_enabled_ = false;
  bool update_animation_enabled_ = false;
  std::unique_ptr<ListAdapterUpdater> updater_;

  int64_t traversal_perf_start_ = 0;
  float unconsumed_x_ = 0.f;
  float unconsumed_y_ = 0.f;

  float cross_axis_gap_ = 0;
  float main_axis_gap_ = 0;

  int32_t last_add_item_position_ = -1;
  int32_t last_add_item_position_when_prefetch_start_ = -1;
  int32_t current_scroll_width_ = 0;
  int32_t current_scroll_height_ = 0;
  LayoutPrefetchRegistry prefetch_registry_;
  GapTaskCollector gap_task_collector_;

  // Owned by the ListView and shared with gap worker.
  fml::RefPtr<GapTaskBundle> gap_task_bundle_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_BASE_LIST_VIEW_H_
