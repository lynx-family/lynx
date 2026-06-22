// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_COVER_VIEW_H_
#define CLAY_UI_COMPONENT_COVER_VIEW_H_

#include "clay/ui/component/overlay_view.h"

namespace clay {

class CoverView : public WithTypeInfo<CoverView, BaseView> {
 public:
  CoverView(int id, PageView* page_view);
  void SetBound(float left, float top, float width, float height) override;

  bool IsLayoutRootCandidate() const override { return true; }
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_COVER_VIEW_H_
