// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "clay/ui/component/native_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/testing/ui_test.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

namespace {

PointerEvent CreateTouchDownPointer(float x, float y) {
  PointerEvent event(PointerEvent::EventType::kDownEvent);
  event.device = PointerEvent::DeviceType::kTouch;
  event.position = {x, y};
  return event;
}

}  // namespace

TEST(NativeViewTest, TextureRegistry) {
  int view_id = 0;
  fml::Thread thread("mock_ui_platform");
  thread.GetTaskRunner()->PostSyncTask([&] {
    std::unique_ptr<PageView> page_view =
        std::make_unique<PageView>(view_id++, nullptr, thread.GetTaskRunner());
    std::unique_ptr<NativeView> root = std::make_unique<NativeView>(
        view_id++, "test-platform", page_view.get());
    EXPECT_EQ(root->IsNativeViewAvailable(), false);
  });
}

class NativeViewHitTestTest : public UITest {};

TEST_F_UI(NativeViewHitTestTest, IgnoreUnhandledTouchSequenceInHitTest) {
  auto* background_view = new View(1, page_.get());
  auto* native_view = new NativeView(2, "test-platform", page_.get());
  page_->AddChild(background_view);
  page_->AddChild(native_view);

  page_->SetWidth(100.f);
  page_->SetHeight(100.f);

  background_view->SetWidth(100.f);
  background_view->SetHeight(100.f);

  native_view->SetWidth(100.f);
  native_view->SetHeight(100.f);

  background_view->OnLayoutUpdated();
  native_view->OnLayoutUpdated();

  {
    HitTestResult result;
    page_->HitTest(CreateTouchDownPointer(50.f, 50.f), result);
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(static_cast<BaseView*>(result.front().get())->id(),
              native_view->id());
  }

  native_view->UpdateTouchDispatchState(false, /* action= */ 0);

  {
    HitTestResult result;
    page_->HitTest(CreateTouchDownPointer(50.f, 50.f), result);
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(static_cast<BaseView*>(result.front().get())->id(),
              background_view->id());
  }

  {
    FloatPoint relative_position;
    EXPECT_EQ(page_->GetTopViewToAcceptEvent(FloatPoint(50.f, 50.f),
                                             &relative_position),
              background_view);
  }

  native_view->UpdateTouchDispatchState(true, /* action= */ 1);

  {
    HitTestResult result;
    page_->HitTest(CreateTouchDownPointer(50.f, 50.f), result);
    ASSERT_FALSE(result.empty());
    EXPECT_EQ(static_cast<BaseView*>(result.front().get())->id(),
              native_view->id());
  }
}

}  // namespace clay
