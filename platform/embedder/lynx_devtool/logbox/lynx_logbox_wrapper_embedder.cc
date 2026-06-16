// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "platform/embedder/lynx_devtool/logbox/lynx_logbox_wrapper_embedder.h"

#include "platform/embedder/lynx_devtool/logbox/lynx_logbox_bridge.h"

namespace lynx {
namespace embedder {

LynxLogBoxWrapperEmbedder::LynxLogBoxWrapperEmbedder(
    NativeWindow native_window, LynxTemplateRenderer* template_renderer)
    : native_window_(native_window),
      template_renderer_(template_renderer),
      bridge_(LynxLogBoxBridge::Create(this)) {}

bool LynxLogBoxWrapperEmbedder::HasBridge() const { return bridge_ != nullptr; }

void LynxLogBoxWrapperEmbedder::OnHostViewAttached() {
  if (host_view_attached_) {
    return;
  }
  host_view_attached_ = true;
  if (bridge_) {
    bridge_->OnHostViewAttached();
  }
}

void LynxLogBoxWrapperEmbedder::OnReload() {
  if (bridge_) {
    bridge_->OnReload();
  }
}

void LynxLogBoxWrapperEmbedder::OnDestroy() {
  if (bridge_) {
    bridge_->OnDestroy();
  }
}

void LynxLogBoxWrapperEmbedder::OnErrorOccurred(
    int level, int32_t error_code, const std::string& message,
    const std::string& fix_suggestion,
    const std::unordered_map<std::string, std::string>& custom_info,
    bool is_logbox_only) {
  if (!bridge_) {
    return;
  }
  LogBoxErrorInfo error;
  error.level = level;
  error.error_code = error_code;
  error.message = message;
  error.fix_suggestion = fix_suggestion;
  error.custom_info = custom_info;
  error.is_logbox_only = is_logbox_only;
  bridge_->OnError(error);
}

void LynxLogBoxWrapperEmbedder::OnLoadTemplate(
    const std::string& url, const std::vector<uint8_t>&,
    const std::shared_ptr<tasm::TemplateData>&) {
  entry_url_ = url;
}

void LynxLogBoxWrapperEmbedder::OnReloadTemplate(
    const std::string& url, const std::vector<uint8_t>&,
    const std::shared_ptr<tasm::TemplateData>&) {
  entry_url_ = url;
}

void LynxLogBoxWrapperEmbedder::OnLoadTemplateBundle(
    const std::string& url, const tasm::LynxTemplateBundle&,
    const std::shared_ptr<tasm::TemplateData>&) {
  entry_url_ = url;
}

std::string LynxLogBoxWrapperEmbedder::GetEntryUrl() const {
  return entry_url_;
}

void* LynxLogBoxWrapperEmbedder::GetHostView() const { return native_window_; }

std::unordered_map<std::string, std::string>
LynxLogBoxWrapperEmbedder::GetLogSources() const {
  if (template_renderer_ == nullptr) {
    return {};
  }
  return template_renderer_->GetAllJsSource();
}

std::string LynxLogBoxWrapperEmbedder::GetLogSourceByFileName(
    std::string_view file_name) const {
  auto log_sources = GetLogSources();
  std::string value;
  size_t match_length = 0;
  for (const auto& [key, source] : log_sources) {
    if (file_name.size() < key.size()) {
      continue;
    }
    if (file_name.substr(file_name.size() - key.size()) == key &&
        key.size() > match_length) {
      match_length = key.size();
      value = source;
    }
  }
  return value;
}

}  // namespace embedder
}  // namespace lynx
