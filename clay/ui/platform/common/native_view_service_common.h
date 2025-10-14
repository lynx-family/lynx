// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_UI_PLATFORM_COMMON_NATIVE_VIEW_SERVICE_COMMON_H_
#define CLAY_UI_PLATFORM_COMMON_NATIVE_VIEW_SERVICE_COMMON_H_

#include <memory>

#include "clay/ui/platform/native_view_service.h"

namespace clay {

class NativeViewServiceCommon final : public NativeViewService {
 public:
  std::unique_ptr<NativeViewPlugin> CreateNativeViewPlugin(
      int id, NativeView* view_ptr) override;
};

}  // namespace clay

#endif  // CLAY_UI_PLATFORM_COMMON_NATIVE_VIEW_SERVICE_COMMON_H_
