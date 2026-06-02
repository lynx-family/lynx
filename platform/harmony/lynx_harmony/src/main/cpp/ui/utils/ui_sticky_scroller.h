// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_UI_STICKY_SCROLLER_H_
#define PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_UI_STICKY_SCROLLER_H_

namespace lynx {
namespace tasm {
namespace harmony {

class UIStickyScroller {
 public:
  virtual void EnableSticky() = 0;
  virtual void RefreshStickyChildren() = 0;
  virtual void AddStickyChildSign(int sign) = 0;
  virtual void RemoveStickyChildSign(int sign) = 0;
  virtual int GetSignFromImplUI() = 0;

 protected:
  ~UIStickyScroller() = default;
};

}  // namespace harmony
}  // namespace tasm
}  // namespace lynx

#endif  // PLATFORM_HARMONY_LYNX_HARMONY_SRC_MAIN_CPP_UI_UTILS_UI_STICKY_SCROLLER_H_
