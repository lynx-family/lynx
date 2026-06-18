// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <windows.h>
#include <winuser.h>

#include <utility>

#include "base/include/debug/lynx_error.h"
#include "core/base/threading/task_runner_manufactor.h"
#include "devtool/base_devtool/logbox/logbox_base.h"
#include "platform/embedder/logbox_embedder.h"
#include "platform/embedder/lynx_devtool/devtool_env_embedder.h"

namespace lynx {
namespace embedder {
namespace {

void NormalizeJsSources(std::unordered_map<std::string, std::string>& sources) {
  auto entry = sources.extract("core.js");
  if (!entry.empty()) {
    entry.key() = "lynx_core.js";
    sources.insert(std::move(entry));
  }
  entry = sources.extract("/app-service.js");
  if (!entry.empty()) {
    entry.key() = "app-service.js";
    sources.insert(std::move(entry));
  }
}

lynx::base::LynxErrorLevel GetLogBoxSupportedErrorLevel(int level) {
  auto error_level = static_cast<lynx::base::LynxErrorLevel>(level);
  return error_level == lynx::base::LynxErrorLevel::Warn
             ? lynx::base::LynxErrorLevel::Warn
             : lynx::base::LynxErrorLevel::Error;
}

class EmbedderLogBoxWin;

class EmbedderLogBoxWinProvider : public devtool::LogBoxResourceProvider {
 public:
  explicit EmbedderLogBoxWinProvider(EmbedderLogBoxWin* owner)
      : owner_(owner) {}

  void* GetNativeWindow() override;
  std::string GetTemplateUrl() override;
  const std::unordered_map<std::string, std::string>& GetAllJsSource() override;

 private:
  EmbedderLogBoxWin* owner_ = nullptr;
};

class EmbedderLogBoxWin : public LogBoxEmbedder {
 public:
  EmbedderLogBoxWin(GetNativeWindowCallback get_native_window,
                    GetAllJsSourceCallback get_all_js_source)
      : get_native_window_(std::move(get_native_window)),
        get_all_js_source_(std::move(get_all_js_source)),
        provider_(std::make_shared<EmbedderLogBoxWinProvider>(this)) {}

  void OnLoadTemplate(const std::string& url) override {
    template_url_ = url;
    is_js_sources_dirty_ = true;
  }

  void OnErrorOccurred(
      int level, int32_t error_code, const std::string& message,
      const std::string& fix_suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only) override {
    lynx::base::LynxError error(error_code, message, fix_suggestion,
                                GetLogBoxSupportedErrorLevel(level),
                                is_logbox_only);
    error.custom_info_ = custom_info;
    if (!proxy_) {
      proxy_ = std::make_shared<devtool::LogBoxProxy>(provider_);
    }

    lynx::fml::TaskRunner::RunNowOrPostTask(
        lynx::base::UIThread::GetRunner(),
        [weak = std::weak_ptr<devtool::LogBoxProxy>(proxy_),
         error = std::move(error)]() mutable {
          auto proxy = weak.lock();
          if (proxy) {
            proxy->ShowErrorMessage(std::move(error));
          }
        });
  }

  void* GetNativeWindow() {
    if (!get_native_window_) {
      return nullptr;
    }
    HWND lynx_window = static_cast<HWND>(get_native_window_());
    return lynx_window ? static_cast<void*>(GetAncestor(lynx_window, GA_ROOT))
                       : nullptr;
  }

  std::string GetTemplateUrl() const { return template_url_; }

  const std::unordered_map<std::string, std::string>& GetAllJsSource() {
    if (is_js_sources_dirty_) {
      js_sources_.clear();
      if (get_all_js_source_) {
        js_sources_ = get_all_js_source_();
        NormalizeJsSources(js_sources_);
      }
      is_js_sources_dirty_ = false;
    }
    return js_sources_;
  }

 private:
  GetNativeWindowCallback get_native_window_;
  GetAllJsSourceCallback get_all_js_source_;
  bool is_js_sources_dirty_ = true;
  std::string template_url_;
  std::unordered_map<std::string, std::string> js_sources_;
  std::shared_ptr<EmbedderLogBoxWinProvider> provider_;
  std::shared_ptr<devtool::LogBoxProxy> proxy_;
};

void* EmbedderLogBoxWinProvider::GetNativeWindow() {
  return owner_ ? owner_->GetNativeWindow() : nullptr;
}

std::string EmbedderLogBoxWinProvider::GetTemplateUrl() {
  return owner_ ? owner_->GetTemplateUrl() : "";
}

const std::unordered_map<std::string, std::string>&
EmbedderLogBoxWinProvider::GetAllJsSource() {
  static const std::unordered_map<std::string, std::string> empty_sources;
  return owner_ ? owner_->GetAllJsSource() : empty_sources;
}

}  // namespace

std::unique_ptr<LogBoxEmbedder> CreateEmbedderLogBoxWin(
    LogBoxEmbedder::GetNativeWindowCallback get_native_window,
    LogBoxEmbedder::GetAllJsSourceCallback get_all_js_source) {
  auto& devtool_env = DevToolEnvEmbedder::GetInstance();
  if (!devtool_env.IsLynxDebugEnabled() || !devtool_env.IsLogBoxEnabled()) {
    return nullptr;
  }
  if (!get_native_window || !get_all_js_source) {
    return nullptr;
  }
  return std::make_unique<EmbedderLogBoxWin>(std::move(get_native_window),
                                             std::move(get_all_js_source));
}

// Register the LogBox creator function on static initialization
struct LogBoxRegistrar {
  LogBoxRegistrar() {
    LogBoxEmbedder::RegisterLogBoxCreator(&CreateEmbedderLogBoxWin);
  }
};

static LogBoxRegistrar g_logbox_registrar;

}  // namespace embedder
}  // namespace lynx
