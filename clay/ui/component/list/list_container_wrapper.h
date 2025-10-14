// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_CONTAINER_WRAPPER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_CONTAINER_WRAPPER_H_

#include <string>
#include <vector>

#include "clay/ui/component/list/list_container_view.h"
#include "clay/ui/component/scrollbar/scrollbar_wrapper.h"
namespace clay {

class ListContainerWrapper
    : public WithTypeInfo<ListContainerWrapper, ScrollbarWrapper>,
      public ListContainerView::Delegate,
      public Scrollable::Listener {
 public:
  ListContainerWrapper(int32_t id, PageView* page_view);

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void DidUpdateAttributes() override;

  void UpdateContentOffsetForListContainer(float content_size,
                                           float target_content_offset_x,
                                           float target_content_offset_y);
  void InsertListItemPaintingNode(BaseView* view);
  void RemoveListItemPaintingNode(BaseView* view);

  void UpdateScrollInfo(bool smooth, float estimated_offset, bool scrolling);

#define UI_METHOD_LIST_DECLARATION(V) \
  V(scrollToPosition)                 \
  V(getVisibleItemsPositions)         \
  V(getScrollInfo)                    \
  V(getVisibleCells)
  UI_METHOD_LIST_DECLARATION(UI_METHOD_DEF);
#undef UI_METHOD_LIST_DECLARATION

  void OnListViewDidLayout() override;

  ListContainerView* GetListContainerView() const {
    return static_cast<ListContainerView*>(view_);
  }

  void AddChild(BaseView* child, int index) override {}

 private:
  void OnDestroy() override;

  void UpdateScrollbarIfNeeded();

  // Override ScrollbarWrapper
  void WillUpdateScrollbar() override;
  float GetScrollbarScrollOffset() override;
  float GetTotalLength() override;

  // Override Scrollable::Listener
  void OnScrollableScrolled() override;

  // Override ListContainerView::Delegate
  void OnScrollToPosition(int position, float offset, int align,
                          bool smooth) override;
  void OnScrollStopped() override;
  void OnLayoutFinish(BaseView* view) override;
  void OnScrollByListContainer(float offset_x, float offset_y, float original_x,
                               float original_y) override;

  // Override ScrollbarView::Delegate
  void OnScrollbarScrolled(float old_position, float new_position,
                           bool by_interaction, bool smooth) override;

  void OnNodeReady() override;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_CONTAINER_WRAPPER_H_
