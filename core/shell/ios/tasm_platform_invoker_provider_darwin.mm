// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "core/shell/ios/tasm_platform_invoker_provider_darwin.h"

#import "core/shell/ios/tasm_platform_invoker_darwin.h"

namespace lynx {
namespace shell {

std::unique_ptr<TasmPlatformInvoker> TasmPlatformInvokerProviderDarwin::Create() {
  id<TemplateRenderCallbackProtocol> render = render_;
  if (!render) {
    return nullptr;
  }

  return std::make_unique<TasmPlatformInvokerDarwin>(render);
}

}  // namespace shell
}  // namespace lynx
