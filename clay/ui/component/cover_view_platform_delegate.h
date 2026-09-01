// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_COMPONENT_COVER_VIEW_PLATFORM_DELEGATE_H_
#define CLAY_UI_COMPONENT_COVER_VIEW_PLATFORM_DELEGATE_H_

#include <memory>

#include "clay/common/service/service.h"
#include "clay/ui/platform/cover_view_platform_service.h"

namespace clay {

class PageView;

class CoverViewPlatformDelegate final {
 public:
  CoverViewPlatformDelegate(int id, PageView* page_view);
  ~CoverViewPlatformDelegate() = default;

  void SetPreferredSize(int width, int height);
  void OnAttachToTree();
  void OnDetachFromTree();
  void OnDestroy();

 private:
  Puppet<Owner::kUI, std::unique_ptr<CoverViewPlatformPlugin>> platform_plugin_;
};

}  // namespace clay

#endif  // CLAY_UI_COMPONENT_COVER_VIEW_PLATFORM_DELEGATE_H_
