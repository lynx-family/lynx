// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/ui_delegate_darwin.h"
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/renderer/ui_wrapper/layout/ios/layout_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"

namespace lynx {
namespace tasm {

std::unique_ptr<PaintingCtxPlatformImpl> UIDelegateDarwin::CreatePaintingContext() {
  return std::make_unique<PaintingContextDarwin>(ui_owner_, enable_create_ui_async_);
}

std::unique_ptr<LayoutCtxPlatformImpl> UIDelegateDarwin::CreateLayoutContext() {
  return std::make_unique<LayoutContextDarwin>(shadow_node_owner_);
}

std::unique_ptr<PropBundleCreator> UIDelegateDarwin::CreatePropBundleCreator() {
  return std::make_unique<PropBundleCreatorDarwin>();
}

}  // namespace tasm
}  // namespace lynx
