// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_HARMONY_TASM_PLATFORM_INVOKER_PROVIDER_HARMONY_H_
#define CORE_SHELL_HARMONY_TASM_PLATFORM_INVOKER_PROVIDER_HARMONY_H_

#include <memory>

#include "core/shell/tasm_platform_invoker_provider.h"
#include "platform/harmony/lynx_harmony/src/main/cpp/lynx_template_renderer.h"

namespace lynx {
namespace shell {

// Harmony implementation of TasmPlatformInvokerProvider.
// It creates a TasmPlatformInvokerHarmony with the provided WeakFlag
// captured from LynxTemplateRenderer.
class TasmPlatformInvokerProviderHarmony : public TasmPlatformInvokerProvider {
 public:
  explicit TasmPlatformInvokerProviderHarmony(
      const std::weak_ptr<harmony::LynxTemplateRenderer::WeakFlag>& flag);
  ~TasmPlatformInvokerProviderHarmony() override = default;

  std::unique_ptr<TasmPlatformInvoker> Create() override;

 private:
  std::weak_ptr<harmony::LynxTemplateRenderer::WeakFlag> weak_flag_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_HARMONY_TASM_PLATFORM_INVOKER_PROVIDER_HARMONY_H_
