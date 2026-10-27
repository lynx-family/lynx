// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SKITY_SKITY_AUTO_CANVAS_SAVE_H_
#define CLAY_GFX_SKITY_SKITY_AUTO_CANVAS_SAVE_H_

#include "skity/render/canvas.hpp"

namespace clay {

class SkityAutoCanvasRestore {
 public:
  SkityAutoCanvasRestore(skity::Canvas* canvas, bool do_save)
      : canvas_(canvas), save_count_(0) {
    if (canvas_) {
      save_count_ = canvas->GetSaveCount();
      if (do_save) {
        canvas->Save();
      }
    }
  }

  ~SkityAutoCanvasRestore() {
    if (canvas_) {
      canvas_->RestoreToCount(save_count_);
    }
  }

  void restore() {
    if (canvas_) {
      canvas_->RestoreToCount(save_count_);
      canvas_ = nullptr;
    }
  }

 private:
  skity::Canvas* canvas_;
  int save_count_;
  SkityAutoCanvasRestore(SkityAutoCanvasRestore&&) = delete;
  SkityAutoCanvasRestore(const SkityAutoCanvasRestore&) = delete;
  SkityAutoCanvasRestore& operator=(SkityAutoCanvasRestore&&) = delete;
  SkityAutoCanvasRestore& operator=(const SkityAutoCanvasRestore&) = delete;
};

}  // namespace clay

#endif  // CLAY_GFX_SKITY_SKITY_AUTO_CANVAS_SAVE_H_
