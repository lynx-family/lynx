// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_TASM_PLATFORM_INVOKER_H_
#define CORE_SHELL_TASM_PLATFORM_INVOKER_H_

#include <memory>
#include <string>

#include "core/renderer/page_config.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace shell {

// Invoke platform directly from the TASM thread.
class TasmPlatformInvoker {
 public:
  TasmPlatformInvoker() = default;
  virtual ~TasmPlatformInvoker() = default;

  virtual void OnPageConfigDecoded(
      const std::shared_ptr<tasm::PageConfig>& config) = 0;
  virtual lepus::Value TriggerLepusMethod(const std::string& method_name,
                                          const lepus::Value& args) = 0;
  virtual void TriggerLepusMethodAsync(const std::string& method_name,
                                       const lepus::Value& args) = 0;
  virtual std::string TranslateResourceForTheme(
      const std::string& res_id, const std::string& theme_key) = 0;
  virtual void GetI18nResource(const std::string& channel,
                               const std::string& fallback_url) = 0;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_TASM_PLATFORM_INVOKER_H_
