// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/lynx_view_clients.h"

#include <memory>
#include <vector>

#include "core/runtime/lepus_context/json_parser.h"

namespace lynx {
namespace embedder {

void LynxViewClients::AddClient(lynx_view_client_t* client) {
  if (std::find(clients_.begin(), clients_.end(), client) == clients_.end()) {
    clients_.emplace_back(client);
  }
}

void LynxViewClients::RemoveClient(lynx_view_client_t* client) {
  clients_.erase(std::remove(clients_.begin(), clients_.end(), client),
                 clients_.end());
}

void LynxViewClients::OnLoaded(const std::string& url) {
  for (auto& client : clients_) {
    if (client->on_load_success) {
      client->on_load_success(client);
    }
  }
}

void LynxViewClients::OnRuntimeReady() {
  for (auto& client : clients_) {
    if (client->on_runtime_ready) {
      client->on_runtime_ready(client);
    }
  }
}

void LynxViewClients::OnDataUpdated() {
  for (auto& client : clients_) {
    if (client->on_data_updated) {
      client->on_data_updated(client);
    }
  }
}

void LynxViewClients::OnPageChanged(bool is_first_screen) {
  for (auto& client : clients_) {
    if (is_first_screen) {
      if (client->on_first_screen) {
        client->on_first_screen(client);
      }
    } else {
      if (client->on_page_updated) {
        client->on_page_updated(client);
      }
    }
  }
}

void LynxViewClients::OnFirstLoadPerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {}

void LynxViewClients::OnUpdatePerfReady(
    const std::unordered_map<int32_t, double>& perf,
    const std::unordered_map<int32_t, std::string>& perf_timing) {}

void LynxViewClients::OnErrorOccurred(
    int level, int32_t error_code, const std::string& message,
    const std::string& fix_suggestion,
    const std::unordered_map<std::string, std::string>& custom_info,
    bool is_logbox_only) {
  for (auto* client : clients_) {
    if (client->on_received_error) {
      client->on_received_error(client, error_code, message.c_str());
    }
  }
}

void LynxViewClients::OnThemeUpdatedByJs(
    const std::unordered_map<std::string, std::string>& theme) {}

void LynxViewClients::OnLoadTemplate(
    const std::string& url, const std::vector<uint8_t>& source,
    const std::shared_ptr<tasm::TemplateData>& data) {
  for (auto* client : clients_) {
    if (client->on_page_start) {
      client->on_page_start(client, url.c_str());
    }
  }
}

void LynxViewClients::OnReloadTemplate(
    const std::string& url, const std::vector<uint8_t>& source,
    const std::shared_ptr<tasm::TemplateData>& data) {}

void LynxViewClients::OnLoadTemplateBundle(
    const std::string& url, const tasm::LynxTemplateBundle& template_bundle,
    const std::shared_ptr<tasm::TemplateData>& data) {
  for (auto* client : clients_) {
    if (client->on_page_start) {
      client->on_page_start(client, url.c_str());
    }
  }
}

void LynxViewClients::OnLoadScriptAsync(const std::string& url,
                                        const std::string& source) {}

void LynxViewClients::OnPageConfigDecoded(
    const std::shared_ptr<tasm::PageConfig>& config) {}

void LynxViewClients::OnTimingSetup(const lepus::Value& timing_info) {
  if (clients_.empty()) {
    return;
  }
  std::string timing_info_str =
      lepus::lepusValueToString(timing_info, false, true);
  for (auto* client : clients_) {
    if (client->on_timing_setup) {
      client->on_timing_setup(client, timing_info_str.c_str());
    }
  }
}

void LynxViewClients::OnTimingUpdate(const lepus::Value& timing_info,
                                     const lepus::Value& update_timing,
                                     const std::string& update_flag) {
  if (clients_.empty()) {
    return;
  }
  std::string timing_info_str =
      lepus::lepusValueToString(timing_info, false, true);
  std::string update_timing_str =
      lepus::lepusValueToString(update_timing, false, true);
  for (auto* client : clients_) {
    if (client->on_timing_update) {
      client->on_timing_update(client, timing_info_str.c_str(),
                               update_timing_str.c_str(), update_flag.c_str());
    }
  }
}

void LynxViewClients::OnPerformanceEvent(const lepus::Value& event_entry) {}

void LynxViewClients::OnTemplateBundleReady(
    const tasm::LynxTemplateBundle& bundle) {}

}  // namespace embedder
}  // namespace lynx
