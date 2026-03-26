// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef PLATFORM_EMBEDDER_LYNX_VIEW_CLIENTS_H_
#define PLATFORM_EMBEDDER_LYNX_VIEW_CLIENTS_H_

#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "platform/embedder/core/lynx_template_renderer.h"
#include "platform/embedder/lynx_view_client_priv.h"

namespace lynx {
namespace embedder {
class LynxViewClients : public TemplateRendererClient {
 public:
  LynxViewClients() = default;
  virtual ~LynxViewClients() = default;

  void AddClient(lynx_view_client_t* client);
  void RemoveClient(lynx_view_client_t* client);

  void OnLoaded(const std::string& url) override;
  void OnRuntimeReady() override;
  void OnDataUpdated() override;
  void OnPageChanged(bool is_first_screen) override;
  void OnFirstLoadPerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing) override;
  void OnUpdatePerfReady(
      const std::unordered_map<int32_t, double>& perf,
      const std::unordered_map<int32_t, std::string>& perf_timing) override;
  void OnErrorOccurred(
      int level, int32_t error_code, const std::string& message,
      const std::string& fix_suggestion,
      const std::unordered_map<std::string, std::string>& custom_info,
      bool is_logbox_only) override;
  void OnThemeUpdatedByJs(
      const std::unordered_map<std::string, std::string>& theme) override;
  void OnLoadTemplate(const std::string& url,
                      const std::vector<uint8_t>& source,
                      const std::shared_ptr<tasm::TemplateData>& data) override;
  void OnReloadTemplate(
      const std::string& url, const std::vector<uint8_t>& source,
      const std::shared_ptr<tasm::TemplateData>& data) override;
  void OnLoadTemplateBundle(
      const std::string& url, const tasm::LynxTemplateBundle& template_bundle,
      const std::shared_ptr<tasm::TemplateData>& data) override;
  void OnLoadScriptAsync(const std::string& url,
                         const std::string& source) override;
  void OnPageConfigDecoded(
      const std::shared_ptr<tasm::PageConfig>& config) override;

  void OnTimingSetup(const lepus::Value& timing_info) override;
  void OnTimingUpdate(const lepus::Value& timing_info,
                      const lepus::Value& update_timing,
                      const std::string& update_flag) override;
  void OnPerformanceEvent(const lepus::Value& event_entry) override;
  void OnTemplateBundleReady(const tasm::LynxTemplateBundle& bundle) override;

  void OnFrameTiming(int64_t frame_start_time_in_ns,
                     int64_t frame_finish_time_in_ns);

 private:
  std::list<lynx_view_client_t*> clients_;
};
}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_LYNX_VIEW_CLIENTS_H_
