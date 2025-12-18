// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/lynx_devtool/logbox/lynx_logbox_wrapper.h"

#include "core/base/threading/task_runner_manufactor.h"
#include "core/platform/clay/shell/public/lynx_view_base.h"

#if defined(_WIN32)
#include "core/platform/windows/lynx/public/lynx_view.h"
#endif

using LynxError = lynx::base::LynxError;
using LynxErrorLevel = lynx::base::LynxErrorLevel;

namespace lynx {
namespace devtool {

LynxLogBoxWrapper::LynxLogBoxWrapper(
    lynx::LynxViewBase* view,
    std::function<std::unordered_map<std::string, std::string>()>&&
        get_js_source)
    : lynx_view_(view), get_js_source_(std::move(get_js_source)) {}

void LynxLogBoxWrapper::ShowErrorMessage(
    int level, int32_t error_code, const std::string& message,
    const std::string& fix_suggestion,
    const std::unordered_map<std::string, std::string>& custom_info,
    bool is_logbox_only) {
  LynxError errorWrap(error_code, message, fix_suggestion,
                      LynxErrorLevel::Error, is_logbox_only);
  errorWrap.custom_info_ = custom_info;
  if (!proxy_) {
    proxy_ = std::make_shared<LogBoxProxy>(shared_from_this());
  }
#if !defined(DISABLE_LOGBOX_BASE)
  lynx::fml::TaskRunner::RunNowOrPostTask(
      lynx::base::UIThread::GetRunner(),
      [weak = std::weak_ptr<LogBoxProxy>(proxy_),
       error = std::move(errorWrap)]() mutable {
        auto sp_proxy = weak.lock();
        if (!sp_proxy) {
          return;
        }
        sp_proxy->ShowErrorMessage(std::move(error));
      });
#endif
}

void LynxLogBoxWrapper::OnLoadTemplate() { is_js_sources_dirty_ = true; }

void* LynxLogBoxWrapper::GetNativeWindow() {
#if defined(_WIN32)
  lynx::LynxView* lynx_view = reinterpret_cast<lynx::LynxView*>(lynx_view_);
  HWND lynx_window = lynx_view ? lynx_view->GetNativeWindow() : nullptr;
  return lynx_window
             ? reinterpret_cast<void*>(GetAncestor(lynx_window, GA_ROOT))
             : nullptr;
#else
  return nullptr;
#endif
};

std::string LynxLogBoxWrapper::GetTemplateUrl() {
#if !defined(DISABLE_LOGBOX_BASE)
  return lynx_view_ ? lynx_view_->GetTemplateUrl() : "";
#else
  return std::string();
#endif
}

const std::unordered_map<std::string, std::string>&
LynxLogBoxWrapper::GetAllJsSource() {
  if (is_js_sources_dirty_ && get_js_source_) {
    js_sources_ = get_js_source_();
    is_js_sources_dirty_ = false;
    auto entry = js_sources_.extract("core.js");
    if (!entry.empty()) {
      entry.key() = "lynx_core.js";
      js_sources_.insert(std::move(entry));
    }
    entry = js_sources_.extract("/app-service.js");
    if (!entry.empty()) {
      entry.key() = "app-service.js";
      js_sources_.insert(std::move(entry));
    }
  }
  return js_sources_;
}
}  // namespace devtool
}  // namespace lynx
