// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/harmony/tasm_platform_invoker_provider_harmony.h"

#include "core/shell/harmony/tasm_platform_invoker_harmony.h"

namespace lynx {
namespace shell {

TasmPlatformInvokerProviderHarmony::TasmPlatformInvokerProviderHarmony(
    const std::weak_ptr<harmony::LynxTemplateRenderer::WeakFlag>& flag)
    : weak_flag_(flag) {}

std::unique_ptr<TasmPlatformInvoker>
TasmPlatformInvokerProviderHarmony::Create() {
  return std::make_unique<harmony::TasmPlatformInvokerHarmony>(weak_flag_);
}

}  // namespace shell
}  // namespace lynx
