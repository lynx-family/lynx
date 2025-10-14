// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/ui/platform/common/native_view_service_common.h"

#include <memory>

namespace clay {

std::unique_ptr<NativeViewPlugin>
NativeViewServiceCommon::CreateNativeViewPlugin(int id, NativeView* view_ptr) {
  return nullptr;
}

std::shared_ptr<NativeViewService> NativeViewService::Create() {
  return std::make_shared<NativeViewServiceCommon>();
}

}  // namespace clay
