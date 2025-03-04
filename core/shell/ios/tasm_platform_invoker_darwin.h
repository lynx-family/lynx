// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_IOS_TASM_PLATFORM_INVOKER_DARWIN_H_
#define CORE_SHELL_IOS_TASM_PLATFORM_INVOKER_DARWIN_H_

#import <Foundation/Foundation.h>

#include <memory>
#include <string>

#include "core/renderer/page_config.h"
#include "core/runtime/vm/lepus/lepus_value.h"
#include "core/shell/tasm_platform_invoker.h"
#import "darwin/common/lynx/TemplateRenderCallbackProtocol.h"

namespace lynx {
namespace shell {

class TasmPlatformInvokerDarwin : public TasmPlatformInvoker {
 public:
  explicit TasmPlatformInvokerDarwin(id<TemplateRenderCallbackProtocol> render)
      : _render(render) {}
  ~TasmPlatformInvokerDarwin() override = default;

  TasmPlatformInvokerDarwin(const TasmPlatformInvokerDarwin&) = delete;
  TasmPlatformInvokerDarwin& operator=(const TasmPlatformInvokerDarwin&) =
      delete;

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

 private:
  __weak id<TemplateRenderCallbackProtocol> _render;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_IOS_TASM_PLATFORM_INVOKER_DARWIN_H_
