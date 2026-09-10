// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_COVER_VIEW_H_
#define CLAY_UI_COMPONENT_COVER_VIEW_H_

#include <memory>

#include "clay/ui/component/overlay_view.h"

namespace clay {

class CoverViewPlatformDelegate;

class CoverView : public WithTypeInfo<CoverView, BaseView> {
 public:
  CoverView(int id, PageView* page_view);
  ~CoverView() override;

  void SetAttribute(const char* attr, const clay::Value& value) override;
  void SetBound(float left, float top, float width, float height) override;
  bool HitTest(const PointerEvent& event, HitTestResult& result) override;
  void OnAttachToTree() override;
  void OnDetachFromTree() override;
  void OnDestroy() override;

  bool IsLayoutRootCandidate() const override { return true; }

 private:
  std::unique_ptr<CoverViewPlatformDelegate> platform_delegate_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_COVER_VIEW_H_
