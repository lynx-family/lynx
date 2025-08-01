// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HARMONY_TASM_PLATFORM_INVOKER_HARMONY_H_
#define CORE_SHELL_HARMONY_TASM_PLATFORM_INVOKER_HARMONY_H_

#include <memory>
#include <string>

#include "base/include/value/base_value.h"
#include "core/shell/tasm_platform_invoker.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.h"

namespace lynx {
namespace harmony {

class TasmPlatformInvokerHarmony : public shell::TasmPlatformInvoker {
 public:
  explicit TasmPlatformInvokerHarmony(
      const std::weak_ptr<LynxTemplateRenderer::WeakFlag>& flag)
      : weak_flag_(flag) {}
  ~TasmPlatformInvokerHarmony() override = default;

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

  void SetUITaskRunner(const fml::RefPtr<fml::TaskRunner>& task_runner) {
    ui_task_runner_ = task_runner;
  }

  void OnRunPipelineFinished() override{};

 private:
  fml::RefPtr<fml::TaskRunner> ui_task_runner_;
  std::weak_ptr<LynxTemplateRenderer::WeakFlag> weak_flag_;
};

}  // namespace harmony
}  // namespace lynx

#endif  // CORE_SHELL_HARMONY_TASM_PLATFORM_INVOKER_HARMONY_H_
