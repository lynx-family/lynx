// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_LIST_LIST_WRAPPER_H_
#define CLAY_UI_COMPONENT_LIST_LIST_WRAPPER_H_

#include <string>
#include <vector>

#include "clay/ui/component/list/base_list_view.h"
#include "clay/ui/component/scrollbar/scrollbar_wrapper.h"

namespace clay {

class ListWrapper : public WithTypeInfo<ListWrapper, ScrollbarWrapper>,
                    public BaseListView::Delegate,
                    public Scrollable::Listener {
 public:
  ListWrapper(BaseListView* inner_view, int32_t id, std::string tag,
              PageView* page_view);

  ListWrapper(int32_t id, PageView* page_view);
  ~ListWrapper() override;

  bool IsLayoutRootCandidate() const override { return true; }
  void OnLayout(LayoutContext* context) override;

  void SetAttribute(const char* attr_c, const clay::Value& value) override;
  void DidUpdateAttributes() override;

#define UI_METHOD_LIST_DECLARATION(V) \
  V(scrollToPosition)                 \
  V(getVisibleItemsPositions)         \
  V(getVisibleCells)
  UI_METHOD_LIST_DECLARATION(UI_METHOD_DEF);
#undef UI_METHOD_LIST_DECLARATION

  FocusBehavior GetFocusBehavior() const override {
    return view_->GetFocusBehavior();
  }

  BaseListView* GetListView() const {
    return static_cast<BaseListView*>(view_);
  }

 private:
  void OnDestroy() override;

  void UpdateScrollbarIfNeeded();

  // Override ScrollbarWrapper
  void WillUpdateScrollbar() override;
  float GetScrollbarScrollOffset() override;
  float GetTotalLength() override;

  // Override Scrollable::Listener
  void OnScrollableScrolled() override;

  // Override BaseListView::Delegate
  void OnListViewDidLayout() override;

  // Override ScrollbarView::Delegate
  void OnScrollbarScrolled(float old_position, float new_position,
                           bool by_interaction, bool smooth) override;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_LIST_LIST_WRAPPER_H_
