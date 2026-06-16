// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <windows.h>

#include <memory>
#include <utility>

#include "base/include/debug/lynx_error.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "devtool/base_devtool/logbox/logbox_base.h"
#include "platform/embedder/lynx_devtool/logbox/logbox_resource_provider.h"
#include "platform/embedder/lynx_devtool/logbox/lynx_logbox_bridge.h"

namespace lynx {
namespace embedder {

namespace {

lynx::base::LynxErrorLevel ToLynxErrorLevel(int level) {
  return level == static_cast<int>(lynx::base::LynxErrorLevel::Warn)
             ? lynx::base::LynxErrorLevel::Warn
             : lynx::base::LynxErrorLevel::Error;
}

class WinLogBoxResourceProviderAdapter final
    : public lynx::devtool::LogBoxResourceProvider {
 public:
  explicit WinLogBoxResourceProviderAdapter(
      lynx::embedder::LogBoxResourceProvider* provider)
      : provider_(provider) {}

  void* GetNativeWindow() override {
    if (provider_ == nullptr) {
      return nullptr;
    }
    auto* host_view = static_cast<HWND>(provider_->GetHostView());
    if (host_view == nullptr) {
      return nullptr;
    }
    auto* root_window = GetAncestor(host_view, GA_ROOT);
    return root_window != nullptr ? root_window : host_view;
  }

  std::string GetTemplateUrl() override {
    return provider_ != nullptr ? provider_->GetEntryUrl() : std::string();
  }

  const std::unordered_map<std::string, std::string>& GetAllJsSource()
      override {
    source_cache_ = provider_ != nullptr
                        ? provider_->GetLogSources()
                        : std::unordered_map<std::string, std::string>();
    return source_cache_;
  }

 private:
  lynx::embedder::LogBoxResourceProvider* provider_ = nullptr;
  std::unordered_map<std::string, std::string> source_cache_;
};

class LynxLogBoxBridgeWin final : public LynxLogBoxBridge {
 public:
  explicit LynxLogBoxBridgeWin(LogBoxResourceProvider* provider)
      : provider_adapter_(
            std::make_shared<WinLogBoxResourceProviderAdapter>(provider)) {}

  void OnHostViewAttached() override {}

  void OnError(const LogBoxErrorInfo& error) override {
    if (proxy_ == nullptr) {
      proxy_ = std::make_shared<lynx::devtool::LogBoxProxy>(provider_adapter_);
    }

    lynx::base::LynxError wrapped_error(
        error.error_code, error.message, error.fix_suggestion,
        ToLynxErrorLevel(error.level), error.is_logbox_only);
    wrapped_error.custom_info_ = error.custom_info;

    lynx::fml::TaskRunner::RunNowOrPostTask(
        lynx::base::UIThread::GetRunner(),
        [weak_proxy = std::weak_ptr<lynx::devtool::LogBoxProxy>(proxy_),
         wrapped_error = std::move(wrapped_error)]() mutable {
          auto proxy = weak_proxy.lock();
          if (proxy == nullptr) {
            return;
          }
          proxy->ShowErrorMessage(std::move(wrapped_error));
        });
  }

  void OnReload() override { proxy_.reset(); }

  void OnDestroy() override { proxy_.reset(); }

 private:
  std::shared_ptr<WinLogBoxResourceProviderAdapter> provider_adapter_;
  std::shared_ptr<lynx::devtool::LogBoxProxy> proxy_;
};

}  // namespace

std::unique_ptr<LynxLogBoxBridge> LynxLogBoxBridge::Create(
    LogBoxResourceProvider* provider) {
  return std::make_unique<LynxLogBoxBridgeWin>(provider);
}

}  // namespace embedder
}  // namespace lynx
