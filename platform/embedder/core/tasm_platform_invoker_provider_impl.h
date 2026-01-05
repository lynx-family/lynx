// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef PLATFORM_EMBEDDER_CORE_TASM_PLATFORM_INVOKER_PROVIDER_IMPL_H_
#define PLATFORM_EMBEDDER_CORE_TASM_PLATFORM_INVOKER_PROVIDER_IMPL_H_

#include <memory>

#include "core/shell/tasm_platform_invoker_provider.h"
#include "platform/embedder/core/lynx_template_renderer.h"

namespace lynx {
namespace shell {

// Embedder implementation of TasmPlatformInvokerProvider.
// It creates TasmPlatformInvokerImpl instances bound to the
// given LynxTemplateRenderer::WeakFlag.
class TasmPlatformInvokerProviderImpl : public TasmPlatformInvokerProvider {
 public:
  explicit TasmPlatformInvokerProviderImpl(
      const std::weak_ptr<embedder::LynxTemplateRenderer::WeakFlag>& flag);
  ~TasmPlatformInvokerProviderImpl() override = default;

  std::unique_ptr<TasmPlatformInvoker> Create() override;

 private:
  std::weak_ptr<embedder::LynxTemplateRenderer::WeakFlag> weak_flag_;
};

}  // namespace shell
}  // namespace lynx

#endif  // PLATFORM_EMBEDDER_CORE_TASM_PLATFORM_INVOKER_PROVIDER_IMPL_H_
