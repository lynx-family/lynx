// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_SCROLL_VIEW_H_
#define CLAY_UI_COMPONENT_SCROLL_VIEW_H_

#include <memory>
#include <optional>
#include <string>

#include "clay/gfx/scroll_direction.h"
#include "clay/ui/component/bounce_view/bounce_view.h"
#include "clay/ui/component/nested_scroll/nested_scrollable.h"
#include "clay/ui/component/scroller.h"
#include "clay/ui/component/view_callback/scroll_event_callback_manager.h"
#include "clay/ui/rendering/render_scroll.h"

namespace clay {

class ScrollView : public WithTypeInfo<ScrollView, NestedScrollable>,
                   public Scroller::Delegate {
 public:
  class Delegate {
   public:
    virtual void OnScrollViewChildAdded() = 0;
    virtual void OnScrollViewChildRemoved() = 0;
    virtual void OnScrollViewChildUpdated() = 0;
  };
  ScrollView(int32_t id, PageView* page_view);
  ScrollView(int32_t id, ScrollDirection direction, PageView* page_view,
             std::unique_ptr<RenderScroll> render_scroll =
                 std::make_unique<RenderScroll>());
  ScrollView(int32_t id, int32_t callback_id, ScrollDirection direction,
             PageView* page_view,
             std::unique_ptr<RenderScroll> render_scroll =
                 std::make_unique<RenderScroll>());

  ~ScrollView() override;

  int GetCallbackId() override { return callback_id_; }
  void AddChild(BaseView* child, int index) override;
  void RemoveChild(BaseView* child) override;

  void OnLayoutUpdated() override;
  void OnLayout(LayoutContext* context) override;

  bool CanChildrenEscape() const override { return false; }

  FloatPoint GetScrollOffsetForPaint() const override {
    return GetRenderScroll()->GetPaintOffsetForScroll();
  }

  bool OnScrollToVisible() override;
  bool OnScrollToMiddle(BaseView* target_view) override;

  void DoScrollFromRaster(float scroll_offset, bool ignore_repaint);
  // Scroller::Delegate
  void OnScrollUpdate(float scroll_offset) override;
  void OnScrollAnimationStart(float start, float delta, int duration) override;
  void OnScrollAnimationEnd() override;

  void StopAnimation() override;

  NestedScrollable* NextScrollable() override;
  NestedScrollable* PreviousScrollable() override;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void SetDirection(int type) override;
  void AddEventCallback(const char* event) override;

  // content width/height in scroll_view include invisible part
  float ContentWidth() const override;
  float ContentHeight() const override;

  const FloatRect& OverflowRect() const;

  void OnScrollBegin();
  void OnScrollEnd();
  // bool OnFling(const FloatSize& distance);
  bool CanScroll();

  void OnScrollStatusChange(ScrollStatus old_status) override;

  void OnChildSizeChanged(BaseView* child) override;
  void OnContentSizeChanged(const FloatRect& old_rect,
                            const FloatRect& new_rect) override;

  // When offset and index are both set, the effect stack
  void ScrollTo(bool smooth, float offset, int index = -1);
  void ScrollWithDelta(bool smooth, float delta);
  void SetScrollToIndex(int index);
  void SetScrollToId(const std::string& id_str, bool smooth = true);
  void AutoScroll(bool start, float rate);
  void StartScrollInto(BaseView* node, std::string block,
                       std::string inline_mode, std::string behavior);
  void DidScroll() override;
  // Return scroll offset including overscroll
  FloatPoint TotalScrollOffset();

  void CorrectScrollOffset();

  void HandleSticky();
  void EnableSticky();
  Scroller* GetScroller() { return scroller_.get(); }

#ifdef ENABLE_ACCESSIBILITY
  int32_t GetSemanticsActions() const override;
  int32_t GetSemanticsFlags() const override;
  int32_t GetA11yScrollChildren() const override;
#endif

#ifndef NDEBUG
  std::string ToString() const override;
#endif

  FloatPoint DoScroll(FloatPoint delta, bool by_user_input = true,
                      bool ignore_repaint = false) override;

 protected:
  virtual void CalculateOverFlow();

  void ResetScroll() override { OnScrollUpdate(0); }

 private:
  static constexpr float kDefaultThreshold = 0.f;
  friend class ScrollWrapper;
  friend class ListContainerWrapper;

  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }
  void OnBounceStart(float start_offset);
  void NotifyContentSizeChanged(const FloatSize& new_size);
  void StartSmoothScroll();

  void ScrollChildViewToVisible(const FloatRect&) override;
  void ScrollChildViewToMiddle(const FloatRect& rect);
  void Invalidate() override;

  NestedScrollable* FindScrollableById(const std::string& id);

  bool CanInvokeScrollImmediately() const;

  void UpdateScrollOffsetForRTL();

  bool enable_content_size_changed_event_ = false;

  // all content include invisible child
  FloatSize content_size_;
  FloatSize old_content_size_;

  float upper_threshold_ = kDefaultThreshold;
  float lower_threshold_ = kDefaultThreshold;

  std::unique_ptr<ScrollEventCallbackManager> callback_manager_;

  // view id that used as callback arguments. This should be the id of the
  // wrapper view. With callback_id_, Lynx will think the callback is sent from
  // the wrapper (which is known by Lynx) instead of the actual list view.
  int32_t callback_id_ = -1;

  BaseView* parent_scroll_view_ = nullptr;

  ScrollEventCallbackManager::ScrollState last_callback_scroll_state_ =
      ScrollEventCallbackManager::ScrollState::kIdle;

  BounceView* bounce_view_ = nullptr;
  bool auto_scroll_ = false;
  int auto_scroll_rate_;
  std::optional<int> pending_scroll_offset_ = std::nullopt;
  int pending_scroll_index_ = -1;
  fml::WeakPtrFactory<ScrollView> weak_factory_;
  Delegate* delegate_ = nullptr;
  bool initial_scroll_index_set_ = false;
  bool initial_scroll_offset_set_ = false;
  bool enable_sticky_ = false;
  // TODO(liuguoliang): This can be moved to Scrollable and reused by other
  // scrollable components.
  std::unique_ptr<Scroller> scroller_;

  FloatPoint last_scroll_offset_{};

  std::string next_scrollable_;
  std::string pre_scrollable_;

  FRIEND_TEST(ScrollViewTest, NestedScrollOnPC);
  FRIEND_TEST(ScrollViewTest, NestedScrollGestureOnPC);
  FRIEND_TEST(ScrollViewTest, ScrollToId);
};

}  // namespace clay
#endif  // CLAY_UI_COMPONENT_SCROLL_VIEW_H_
