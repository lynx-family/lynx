// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_SHADOW_BUNDLE_H_
#define CLAY_UI_SHADOW_BUNDLE_H_

namespace clay {

class BaseView;

class Bundle {
 public:
  Bundle() = default;
  virtual ~Bundle() = default;
  virtual void UpdateExtraData(BaseView* view) = 0;
};

}  // namespace clay

#endif  // CLAY_UI_SHADOW_BUNDLE_H_
