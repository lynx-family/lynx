// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_SSR_CLIENT_SSR_CLIENT_UTILS_H_
#define CORE_SERVICES_SSR_CLIENT_SSR_CLIENT_UTILS_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/public/pipeline_option.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class TemplateAssembler;
class TemplateData;
class PageProxy;
class RadonPage;
}  // namespace tasm
namespace ssr {
class SsrDataUpdateManager;

class SSRRenderUtils {
 public:
  static bool DecodeSSRData(
      tasm::TemplateAssembler* tasm, std::vector<uint8_t> data,
      const std::shared_ptr<tasm::TemplateData>& template_data,
      tasm::PipelineOptions& pipeline_options, int32_t instance_id);
  static void ReconstructDom(
      const lepus::Value& ssr_out_data, tasm::PageProxy* proxy,
      tasm::RadonPage* page, const lepus::Value& injected_data,
      ssr::SsrDataUpdateManager* data_manager,
      std::function<void(lepus_value, lepus_value)> callback);
  static void ProcessSsrScriptIfNeeded(const lepus::Value& ssr_out_data,
                                       tasm::PageProxy* proxy,
                                       const lepus::Value& injected_data,
                                       SsrDataUpdateManager* data_manager);
};

}  // namespace ssr
}  // namespace lynx

#endif  // CORE_SERVICES_SSR_CLIENT_SSR_CLIENT_UTILS_H_
