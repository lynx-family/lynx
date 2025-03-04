// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_IOS_LYNX_RUNTIME_FACADE_DARWIN_H_
#define CORE_SHELL_IOS_LYNX_RUNTIME_FACADE_DARWIN_H_

#import "LynxBackgroundRuntime.h"
#import "LynxError.h"
#import "LynxProviderRegistry.h"

#include <memory>
#include <string>
#include <vector>

#include "core/resource/external_resource/external_resource_loader.h"
#include "core/runtime/bindings/jsi/modules/ios/module_factory_darwin.h"
#include "core/shell/native_facade.h"
#include "core/shell/native_facade_empty_implementation.h"

@interface LynxBackgroundRuntime ()

- (void)onErrorOccurred:(LynxError*)error;

@end

namespace lynx {
namespace shell {
class NativeRuntimeFacadeDarwin : public NativeFacadeEmptyImpl {
 public:
  NativeRuntimeFacadeDarwin(LynxBackgroundRuntime* runtime) : _runtime(runtime) {}
  ~NativeRuntimeFacadeDarwin() override = default;
  void ReportError(const base::LynxError& error) override;

 private:
  __weak LynxBackgroundRuntime* _runtime;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_IOS_LYNX_RUNTIME_FACADE_DARWIN_H_
