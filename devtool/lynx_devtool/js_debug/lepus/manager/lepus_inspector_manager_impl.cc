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
    Context* context, const std::shared_ptr<InspectorLepusObserver>& observer,
    const std::string& context_name) {
  // Do not support debugging lazy components of non-LepusNG.
  if (!context->IsLepusNGContext() &&
      (!observer->ShouldFetchDebugInfo() ||
       context_name != devtool::kLepusDefaultContextName)) {
    return;
  }

  observer_wp_ = observer;
  inspector_name_ = GenerateInspectorName(context_name);

  inspector_client_ =
      devtool::LepusInspectorClientProvider::GetInspectorClient();

  inspector_client_->InitInspector(context, inspector_name_);
  inspector_client_->ConnectSession();

  observer->OnInspectorInited(devtool::kKeyEngineLepus, inspector_name_,
                              inspector_client_);
}

void LepusInspectorManagerImpl::SetDebugInfo(const std::string& debug_info_url,
                                             const std::string& file_name) {
  auto sp = observer_wp_.lock();
  if (sp == nullptr || inspector_client_ == nullptr) {
    return;
  }

  static int cur_debug_info_id = 0;
  std::string debug_info;
  int debug_info_id = -1;
  auto it = debug_info_url_to_id_.find(debug_info_url);
  if (it == debug_info_url_to_id_.end()) {
    debug_info_id = cur_debug_info_id++;
    debug_info_url_to_id_[debug_info_url] = debug_info_id;
    if (sp->ShouldFetchDebugInfo()) {
      // Get the content of debug-info.json by the specified URL.
      debug_info = sp->GetDebugInfo(debug_info_url);
    }
  } else {
    debug_info_id = it->second;
  }
  inspector_client_->SetDebugInfo(file_name, debug_info, debug_info_id,
                                  debug_info_url);
  sp->PrepareForScriptEval(inspector_name_);
  // Associate and record the file name with the corresponding debug-info.json
  // URL.
  sp->SetDebugInfoUrl(debug_info_url, file_name);
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
  // default context: inspector_name_ is "Main"
  // other context: inspector_name_ is "Main:${context name}"
  return name == devtool::kLepusDefaultContextName
             ? devtool::kTargetLepus
             : devtool::kTargetLepusPrefix + name;
}

}  // namespace lepus
}  // namespace lynx
