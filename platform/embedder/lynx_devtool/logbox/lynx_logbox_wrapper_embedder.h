// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_LYNX_DEVTOOL_LOGBOX_LYNX_LOGBOX_WRAPPER_EMBEDDER_H_
#define PLATFORM_EMBEDDER_LYNX_DEVTOOL_LOGBOX_LYNX_LOGBOX_WRAPPER_EMBEDDER_H_

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "platform/embedder/core/lynx_template_renderer.h"
#include "platform/embedder/lynx_devtool/logbox/logbox_resource_provider.h"
#include "platform/embedder/lynx_devtool/logbox/lynx_logbox_bridge.h"
#include "platform/embedder/public/capi/lynx_view_builder_capi.h"

namespace lynx {
namespace embedder {

class LynxLogBoxWrapperEmbedder final : public TemplateRendererClient,
                                        public LogBoxResourceProvider {
 public:
  LynxLogBoxWrapperEmbedder(NativeWindow native_window,
                            LynxTemplateRenderer* template_renderer);
  ~LynxLogBoxWrapperEmbedder() override = default;

  bool HasBridge() const;
  void OnHostViewAttached();
  void OnReload();
  void OnDestroy();

  void OnErrorOccurred(
      int level, int32_t error_code, const std::string& message,
      const std::string& fix_suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only) override;
  void OnLoadTemplate(const std::string& url,
                      const std::vector<uint8_t>& source,
                      const std::shared_ptr<tasm::TemplateData>& data) override;
  void OnReloadTemplate(
      const std::string& url, const std::vector<uint8_t>& source,
      const std::shared_ptr<tasm::TemplateData>& data) override;
  void OnLoadTemplateBundle(
      const std::string& url, const tasm::LynxTemplateBundle& template_bundle,
      const std::shared_ptr<tasm::TemplateData>& data) override;

  std::string GetEntryUrl() const override;
  void* GetHostView() const override;
  std::unordered_map<std::string, std::string> GetLogSources() const override;
  std::string GetLogSourceByFileName(std::string_view file_name) const override;

 private:
  NativeWindow native_window_;
  LynxTemplateRenderer* template_renderer_;
  std::string entry_url_;
  bool host_view_attached_ = false;
  std::unique_ptr<LynxLogBoxBridge> bridge_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_DEVTOOL_LOGBOX_LYNX_LOGBOX_WRAPPER_EMBEDDER_H_
