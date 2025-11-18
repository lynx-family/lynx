// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>

#include "base/include/fml/thread.h"
#include "clay/ui/common/overlay_manager.h"
#include "clay/ui/component/overlay_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/component/scroll_view.h"
#include "clay/ui/component/view.h"
#include "clay/ui/rendering/render_object.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace clay {

TEST(OverlayViewTest, ShowAndHide) {
  auto thread = std::make_unique<fml::Thread>("ui");
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(0, nullptr, thread->GetTaskRunner());
  page_view->SetBound(0, 0, 1000, 1000);
  std::unique_ptr<OverlayView> overlay1 =
      std::make_unique<OverlayView>(1, page_view.get());
  std::unique_ptr<OverlayView> overlay2 =
      std::make_unique<OverlayView>(2, page_view.get());
  overlay1->SetBound(0, 0, 1000, 1000);
  overlay2->SetBound(0, 0, 1000, 1000);

  auto child1 = std::make_unique<View>(3, page_view.get());
  child1->SetBound(0, 0, 100, 100);
  overlay1->AddChild(child1.get());

  auto child2 = std::make_unique<View>(4, page_view.get());
  child2->SetBound(100, 100, 200, 200);
  overlay2->AddChild(child2.get());

  FloatPoint unused;
  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(10, 10), &unused),
            page_view.get());

  EXPECT_EQ(page_view->overlay_manager()->overlays_.size(), (size_t)0);

  page_view->AddChild(overlay1.get());
  page_view->AddChild(overlay2.get());

  page_view->Layout();

  EXPECT_EQ(page_view->overlay_manager()->overlays_.size(), (size_t)2);

  overlay2->Hide();

  EXPECT_EQ(page_view->overlay_manager()->overlays_.size(), (size_t)1);

  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(10, 10), &unused),
            child1.get());
  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(110, 10), &unused),
            page_view.get());

  overlay2->SetPassEventsThrough(false);
  overlay2->Show();

  EXPECT_EQ(page_view->overlay_manager()->overlays_.size(), (size_t)2);
  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(110, 10), &unused),
            overlay2.get());
  EXPECT_EQ(page_view->GetTopViewToAcceptEvent(FloatPoint(110, 110), &unused),
            child2.get());
}

TEST(OverlayViewTest, Level) {
  auto thread = std::make_unique<fml::Thread>("ui");
  std::unique_ptr<PageView> page_view =
      std::make_unique<PageView>(-1, nullptr, thread->GetTaskRunner());
  page_view->SetBound(0, 0, 1000, 1000);
  std::unique_ptr<OverlayView> overlay1 =
      std::make_unique<OverlayView>(-1, page_view.get());
  std::unique_ptr<OverlayView> overlay2 =
      std::make_unique<OverlayView>(-1, page_view.get());

  page_view->AddChild(overlay1.get());
  page_view->AddChild(overlay2.get());

  page_view->Layout();

  // The larger the level, the lower it will be displayed
  overlay1->Hide();
  overlay2->Hide();
  overlay1->SetLevel(1);
  overlay2->SetLevel(2);
  overlay1->Show();
  overlay2->Show();
  EXPECT_EQ(page_view->overlay_manager()->overlays_.back(), overlay1.get());
  overlay1->Hide();
  overlay2->Hide();
  overlay1->SetLevel(2);
  overlay2->SetLevel(1);
  overlay1->Show();
  overlay2->Show();
  EXPECT_EQ(page_view->overlay_manager()->overlays_.back(), overlay2.get());

  // For overlays with the same level, the newly added ones are at the front (to
  // user).
  overlay1->Hide();
  overlay2->Hide();
  overlay1->SetLevel(1);
  overlay2->SetLevel(1);
  overlay1->Show();
  overlay2->Show();
  EXPECT_EQ(page_view->overlay_manager()->overlays_.back(), overlay2.get());
  overlay1->Hide();
  overlay2->Hide();
  overlay1->SetLevel(1);
  overlay2->SetLevel(1);
  overlay2->Show();
  overlay1->Show();
  EXPECT_EQ(page_view->overlay_manager()->overlays_.back(), overlay1.get());
}

}  // namespace clay
