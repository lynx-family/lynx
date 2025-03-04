// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/services/ssr/client/ssr_client_utils.h"

#include "core/renderer/template_assembler.h"

namespace lynx {
namespace ssr {
bool SSRRenderUtils::DecodeSSRData(
    tasm::TemplateAssembler* tasm, std::vector<uint8_t> data,
    const std::shared_ptr<tasm::TemplateData>& template_data,
    tasm::PipelineOptions& pipeline_options, int32_t instance_id) {
  LYNX_ERROR(error::E_SSR_DECODE, "SSR not implemented", "");
  return false;
}

void SSRRenderUtils::ReconstructDom(
    const lepus::Value& ssr_out_data, tasm::PageProxy* proxy,
    tasm::RadonPage* page, const lepus::Value& injected_data,
    ssr::SsrDataUpdateManager* data_manager,
    std::function<void(lepus_value, lepus_value)> callback) {
  LYNX_ERROR(error::E_SSR_DECODE, "SSR not implemented", "");
  return;
}

void SSRRenderUtils::ProcessSsrScriptIfNeeded(
    const lepus::Value& ssr_out_data, tasm::PageProxy* proxy,
    const lepus::Value& injected_data, SsrDataUpdateManager* data_manager) {
  LYNX_ERROR(error::E_SSR_DECODE, "SSR not implemented", "");
}

}  // namespace ssr
}  // namespace lynx
