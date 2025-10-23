// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <memory>
#include <utility>

#include "clay/ui/component/image_view.h"
#include "clay/ui/component/page_view.h"
#include "clay/ui/gesture/tap_gesture_recognizer.h"

#ifndef CLAY_UI_COMPONENT_TAPPABLE_IMAGE_VIEW_H_
#define CLAY_UI_COMPONENT_TAPPABLE_IMAGE_VIEW_H_

namespace clay {

// Currently only used for soft keyboard.
class TappableImageView : public ImageView {
 public:
  TappableImageView(int id, PageView* page_view) : ImageView(id, page_view) {}

  void SetTapUpCallback(OnTapUpCallback&& callback) {
    if (!tap_recognizer_) {
      tap_recognizer_.reset(
          new TapGestureRecognizer(page_view()->gesture_manager()));
    }
    tap_recognizer_->SetTapUpCallback(std::move(callback));
  }

  void HandleEvent(const PointerEvent& event) override {
    if (event.type == PointerEvent::EventType::kDownEvent) {
      tap_recognizer_->AddPointer(event);
    }
  }

 private:
  std::unique_ptr<TapGestureRecognizer> tap_recognizer_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_TAPPABLE_IMAGE_VIEW_H_
