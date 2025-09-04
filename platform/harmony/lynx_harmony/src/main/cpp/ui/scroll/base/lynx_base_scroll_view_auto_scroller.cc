// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/harmony/lynx_harmony/src/main/cpp/ui/scroll/base/lynx_base_scroll_view_auto_scroller.h"

#include "base/include/fml/make_copyable.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/ui/base/node_manager.h"

namespace lynx {
namespace tasm {
namespace harmony {

LynxBaseScrollViewAutoScroller::LynxBaseScrollViewAutoScroller(
    LynxBaseScrollView* scroll_view)
    : scroll_view_(scroll_view) {}

void LynxBaseScrollViewAutoScroller::RequestVsync() {
  base::HarmonyVsyncManager::GetInstance().RequestVSync(
      fml::MakeCopyable([this](long long timestamp) mutable { this->Tick(); }));
}

void LynxBaseScrollViewAutoScroller::Tick() {
  float delta[2] = {rate_, rate_};
  scroll_view_->ScrollBy(delta);
  if (rate_ == 0) {
    return;
  }
  bool can_scroll = rate_ > 0 ? scroll_view_->CanScrollForwards()
                              : scroll_view_->CanScrollBackwards();
  if (can_scroll) {
    RequestVsync();
  } else {
    if (auto_stop_) {
      StopAutoScroll();
    } else {
      RequestVsync();
    }
  }
}

void LynxBaseScrollViewAutoScroller::StopAutoScroll() {
  rate_ = 0;
  scroll_view_->TryToUpdateScrollState(
      LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateIdle);
}

void LynxBaseScrollViewAutoScroller::StartAutoScroll(float rate,
                                                     bool auto_stop) {
  auto_stop_ = auto_stop;
  rate_ = rate;
  if (rate_ != 0) {
    scroll_view_->TryToUpdateScrollState(
        LynxBaseScrollViewScrollState::LynxBaseScrollViewScrollStateAnimating);
    RequestVsync();
  }
}

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx
