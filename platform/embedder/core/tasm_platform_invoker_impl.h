// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_CORE_TASM_PLATFORM_INVOKER_IMPL_H_
#define PLATFORM_EMBEDDER_CORE_TASM_PLATFORM_INVOKER_IMPL_H_

#include <memory>
#include <string>

#include "base/include/value/base_value.h"
#include "core/shell/tasm_platform_invoker.h"
#include "platform/embedder/core/lynx_template_renderer.h"

namespace lynx {
namespace embedder {

class TasmPlatformInvokerImpl : public shell::TasmPlatformInvoker {
 public:
  explicit TasmPlatformInvokerImpl(
      std::weak_ptr<LynxTemplateRenderer::WeakFlag> weak_renderer)
      : weak_renderer_(weak_renderer) {}
  ~TasmPlatformInvokerImpl() override = default;

  void OnPageConfigDecoded(
      const std::shared_ptr<tasm::PageConfig>& config) override;

  std::string TranslateResourceForTheme(const std::string& res_id,
                                        const std::string& theme_key) override;

  lepus::Value TriggerLepusMethod(const std::string& method_name,
                                  const lepus::Value& args) override;

  void TriggerLepusMethodAsync(const std::string& method_name,
                               const lepus::Value& args) override;

  void GetI18nResource(const std::string& channel,
                       const std::string& fallback_url) override;

  void OnRunPipelineFinished() override{};

 private:
  std::weak_ptr<LynxTemplateRenderer::WeakFlag> weak_renderer_;
};

}  // namespace embedder
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CORE_TASM_PLATFORM_INVOKER_IMPL_H_
