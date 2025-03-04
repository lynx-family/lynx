// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/js_debug/lepus/manager/lepus_inspector_manager_impl.h"

#include "core/runtime/vm/lepus/context.h"
#include "devtool/js_inspect/lepus/lepus_inspector_client_provider.h"
#include "devtool/lynx_devtool/js_debug/inspector_const_extend.h"

namespace lynx {
namespace lepus {

void LepusInspectorManagerImpl::InitInspector(
    Context* context, const std::shared_ptr<InspectorLepusObserver>& observer) {
  const std::string& debug_info_url = context->GetDebugInfoURL();
  if (!observer->IsDebugEnabled()) {
    observer->SetDebugInfoUrl(debug_info_url);
    return;
  }

  // If reuse Lepus context, do not recreate inspector_client_, but need to
  // download debug-info.json.
  if (inspector_client_ == nullptr) {
    observer_wp_ = observer;
    inspector_name_ = GenerateInspectorName(context->name());

    inspector_client_ =
        devtool::LepusInspectorClientProvider::GetInspectorClient();

    inspector_client_->InitInspector(context, inspector_name_);
    inspector_client_->ConnectSession();
  }
  inspector_client_->SetDebugInfo(debug_info_url,
                                  observer->GetDebugInfo(debug_info_url));

  observer->OnInspectorInited(devtool::kKeyEngineLepus, inspector_name_,
                              inspector_client_);
}

void LepusInspectorManagerImpl::DestroyInspector() {
  auto sp = observer_wp_.lock();
  if (sp != nullptr) {
    sp->OnContextDestroyed(inspector_name_);
  }
  if (inspector_client_ != nullptr) {
    inspector_client_->DisconnectSession();
    inspector_client_->DestroyInspector();
  }
}

std::string LepusInspectorManagerImpl::GenerateInspectorName(
    const std::string& name) {
  // default entry or reused lepus context: inspector_name_ is "Main"
  // lazy component: inspector_name_ is "Main:${lazy component url}"
  return name == devtool::kLepusDefaultContextName
             ? devtool::kTargetLepus
             : devtool::kTargetLepusPrefix + name;
}

}  // namespace lepus
}  // namespace lynx
