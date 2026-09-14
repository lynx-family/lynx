// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#define CLAY_UNIT_TESTS 1

#include <memory>

#include "clay/ui/component/component_constants.h"
#include "clay/ui/component/list/focus_list_adapter.h"
#include "clay/ui/component/list/focus_list_view.h"
#include "clay/ui/testing/ui_test.h"

namespace clay {
namespace {

class EventListView : public BaseFocusListView {
 public:
  explicit EventListView(PageView* page_view)
      : BaseFocusListView(-1, page_view) {
    SetCount(50);
    SetFocusableRule([](int) { return false; });
    SetAdapterFactory([](BaseFocusListView* view) {
      return std::make_unique<FocusListAdapter>(view);
    });
    SetColumnRow(0, 0);
    SetItemDefaultDimension(100.f);
    Init();
  }
};

}  // namespace

class ListEventCallbackManagerTest : public UITest {};

TEST_F_UI(ListEventCallbackManagerTest,
          ScrollEventIncludesVisibleCellsWhenRequested) {
  size_t attached_cells_count = 0;
  EXPECT_CALL(*this, OnCustomEvent(event_attr::kEventScroll, ::testing::_))
      .WillOnce(::testing::Invoke(
          [&attached_cells_count](std::string, const clay::Value::Map& params) {
            const auto& attached_cells = params.at("attachedCells");
            ASSERT_TRUE(attached_cells.IsArray());
            attached_cells_count = attached_cells.GetArray().size();
          }));

  auto list_view = std::make_unique<EventListView>(page_.get());
  list_view->SetAttribute("needs-visible-cells", clay::Value(true));
  list_view->SetAttribute("scroll-event-throttle", clay::Value(0));
  list_view->AddEventCallback(event_attr::kEventScroll);
  page_->AddChild(list_view.get());
  list_view->SetWidth(300);
  list_view->SetHeight(900);
  list_view->Layout();

  EXPECT_TRUE(list_view->OnScrollBy({0, -100}));
  EXPECT_GT(attached_cells_count, 0u);
}

}  // namespace clay
