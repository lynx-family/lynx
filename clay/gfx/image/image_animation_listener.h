// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_IMAGE_IMAGE_ANIMATION_LISTENER_H_
#define CLAY_GFX_IMAGE_IMAGE_ANIMATION_LISTENER_H_

namespace clay {

class ImageAnimationListener {
 public:
  virtual void OnStartPlay() = 0;
  virtual void OnCurrentLoopComplete() = 0;
  virtual void OnFinalLoopComplete() = 0;

 protected:
  virtual ~ImageAnimationListener() = default;
};

}  // namespace clay

#endif  // CLAY_GFX_IMAGE_IMAGE_ANIMATION_LISTENER_H_
