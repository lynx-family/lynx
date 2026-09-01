// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_COVER_VIEW_PLATFORM_SERVICE_H_
#define CLAY_UI_PLATFORM_COVER_VIEW_PLATFORM_SERVICE_H_

#include <memory>

#include "clay/common/service/service.h"

namespace clay {

class CoverViewPlatformPlugin : public ActorObject<Owner::kPlatform> {
 public:
  virtual ~CoverViewPlatformPlugin() = default;

  virtual void Initialize(int id) = 0;
  virtual void SetPreferredSize(int width, int height) {}
  virtual void OnAttachToTree() = 0;
  virtual void OnDetachFromTree() = 0;
  virtual void OnViewDestroy() = 0;
};

class CoverViewPlatformService
    : public Service<CoverViewPlatformService, Owner::kPlatform,
                     ServiceFlags::kManualRegister> {
 public:
  virtual std::unique_ptr<CoverViewPlatformPlugin>
  CreateCoverViewPlatformPlugin() = 0;
};

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_COVER_VIEW_PLATFORM_SERVICE_H_
