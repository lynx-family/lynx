// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "platform/embedder/core/tasm_platform_invoker_provider_impl.h"

#include "platform/embedder/core/tasm_platform_invoker_impl.h"

namespace lynx {
namespace shell {

TasmPlatformInvokerProviderImpl::TasmPlatformInvokerProviderImpl(
    const std::weak_ptr<embedder::LynxTemplateRenderer::WeakFlag>& flag)
    : weak_flag_(flag) {}

std::unique_ptr<TasmPlatformInvoker> TasmPlatformInvokerProviderImpl::Create() {
  return std::make_unique<embedder::TasmPlatformInvokerImpl>(weak_flag_);
}

}  // namespace shell
}  // namespace lynx
