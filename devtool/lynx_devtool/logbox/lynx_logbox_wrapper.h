// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef LYNX_DEVTOOL_LYNX_DEVTOOL_LOGBOX_LYNX_LOGBOX_WRAPPER_H_
#define LYNX_DEVTOOL_LYNX_DEVTOOL_LOGBOX_LYNX_LOGBOX_WRAPPER_H_

#include <functional>
#include <memory>

#include "devtool/base_devtool/logbox/logbox_base.h"

namespace lynx {
class LynxViewBase;
}  // namespace lynx

namespace lynx {
namespace devtool {
class LynxLogBoxWrapper
    : public LogBoxBase,
      public LogBoxResourceProvider,
      public std::enable_shared_from_this<LynxLogBoxWrapper> {
 public:
  LynxLogBoxWrapper() = delete;
  LynxLogBoxWrapper& operator=(const LynxLogBoxWrapper&) = delete;

  LynxLogBoxWrapper(
      lynx::LynxViewBase* view,
      std::function<std::unordered_map<std::string, std::string>()>&&
          get_js_source);

  // override for LogBoxBase
  void ShowErrorMessage(
      int level, int32_t error_code, const std::string& message,
      const std::string& fix_suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only) override;
  void OnLoadTemplate() override;

  // override for LogBoxResourceProvider
  void* GetNativeWindow() override;
  std::string GetTemplateUrl() override;
  const std::unordered_map<std::string, std::string>& GetAllJsSource() override;

 private:
  [[maybe_unused]] lynx::LynxViewBase* lynx_view_ = nullptr;
  bool is_js_sources_dirty_ = true;
  std::function<std::unordered_map<std::string, std::string>()> get_js_source_;
  std::shared_ptr<LogBoxProxy> proxy_;
  std::unordered_map<std::string, std::string> js_sources_;
};
}  // namespace devtool
}  // namespace lynx
#endif  // LYNX_DEVTOOL_LYNX_DEVTOOL_LOGBOX_LYNX_LOGBOX_WRAPPER_H_
