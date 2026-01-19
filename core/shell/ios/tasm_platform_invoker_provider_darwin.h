// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_IOS_TASM_PLATFORM_INVOKER_PROVIDER_DARWIN_H_
#define CORE_SHELL_IOS_TASM_PLATFORM_INVOKER_PROVIDER_DARWIN_H_

#import <Foundation/Foundation.h>

#include <memory>

#include "core/shell/tasm_platform_invoker_provider.h"
#import "platform/darwin/common/lynx/TemplateRenderCallbackProtocol.h"

namespace lynx {
namespace shell {

// Darwin implementation of TasmPlatformInvokerProvider.
// It creates TasmPlatformInvokerDarwin instances bound to the
// given TemplateRenderCallbackProtocol render object.
class TasmPlatformInvokerProviderDarwin : public TasmPlatformInvokerProvider {
 public:
  explicit TasmPlatformInvokerProviderDarwin(
      id<TemplateRenderCallbackProtocol> render)
      : render_(render) {}
  ~TasmPlatformInvokerProviderDarwin() override = default;

  std::unique_ptr<TasmPlatformInvoker> Create() override;

 private:
  __weak id<TemplateRenderCallbackProtocol> render_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_IOS_TASM_PLATFORM_INVOKER_PROVIDER_DARWIN_H_
