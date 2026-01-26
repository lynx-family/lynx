// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/ui_wrapper/painting/ios/ui_delegate_darwin.h"
#include "core/renderer/ui_wrapper/common/ios/prop_bundle_darwin.h"
#include "core/renderer/ui_wrapper/layout/ios/layout_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/native_painting_context_darwin.h"
#include "core/renderer/ui_wrapper/painting/ios/painting_context_darwin.h"

#import <Lynx/LynxUIOwner+Private.h>

namespace lynx {
namespace tasm {

std::unique_ptr<PaintingCtxPlatformImpl> UIDelegateDarwin::CreatePaintingContext() {
  if (use_native_painting_context_) {
    return std::make_unique<NativePaintingCtxDarwin>([ui_owner_ tryGetContainerView], textra_);
  }
  return std::make_unique<PaintingContextDarwin>(ui_owner_, enable_create_ui_async_, textra_);
}

std::unique_ptr<LayoutCtxPlatformImpl> UIDelegateDarwin::CreateLayoutContext() {
  return std::make_unique<LayoutContextDarwin>(shadow_node_owner_);
}

std::unique_ptr<PropBundleCreator> UIDelegateDarwin::CreatePropBundleCreator() {
  return std::make_unique<PropBundleCreatorDarwin>();
}

void UIDelegateDarwin::OnLynxCreate(
    const std::shared_ptr<shell::ListEngineProxy> &list_engine_proxy,
    const std::shared_ptr<shell::LynxEngineProxy> &engine_proxy,
    const std::shared_ptr<shell::LynxRuntimeProxy> &runtime_proxy,
    const std::shared_ptr<shell::LynxLayoutProxy> &layout_proxy,
    const std::shared_ptr<shell::PerfControllerProxy> &perf_controller_proxy,
    const std::shared_ptr<pub::LynxResourceLoader> &resource_loader,
    const fml::RefPtr<fml::TaskRunner> &ui_task_runner,
    const fml::RefPtr<fml::TaskRunner> &layout_task_runner, int32_t instance_id,
    bool is_embedded_mode) {
  if (is_embedded_mode) {
    engine_proxy_ = std::move(engine_proxy);
    [[shadow_node_owner_ layoutTick] setLayoutBlock:^{
      engine_proxy_->TriggerLayout();
    }];
  } else {
    layout_proxy_ = std::move(layout_proxy);
    [[shadow_node_owner_ layoutTick] setLayoutBlock:^{
      layout_proxy_->TriggerLayout();
    }];
  }
}

}  // namespace tasm
}  // namespace lynx
