// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_COVER_VIEW_PLATFORM_DELEGATE_H_
#define CLAY_UI_COMPONENT_COVER_VIEW_PLATFORM_DELEGATE_H_

#include <memory>

#include "base/include/fml/memory/weak_ptr.h"
#include "clay/common/service/service.h"
#include "clay/ui/platform/overlay_service.h"

namespace clay {

class PageView;

class CoverViewPlatformDelegate final
    : public OverlayPlatformPlugin::OverlayListener {
 public:
  CoverViewPlatformDelegate(int id, PageView* page_view);
  ~CoverViewPlatformDelegate() override = default;

  void SetEventThrough(bool event_through);
  void SetSize(int width, int height);
  void OnAttachToTree();
  void OnDetachFromTree();
  void OnDestroy();

  void OnDialogBackPressed() override {}
  void OnViewOffsetUpdated(int offset_x, int offset_y) override {}

 private:
  fml::WeakPtr<CoverViewPlatformDelegate> GetWeakPtr() const {
    return weak_factory_.GetWeakPtr();
  }

  fml::WeakPtrFactory<CoverViewPlatformDelegate> weak_factory_;
  Puppet<Owner::kUI, std::unique_ptr<OverlayPlatformPlugin>> overlay_plugin_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_COVER_VIEW_PLATFORM_DELEGATE_H_
