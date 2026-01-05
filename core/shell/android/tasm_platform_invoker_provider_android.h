// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_TASM_PLATFORM_INVOKER_PROVIDER_ANDROID_H_
#define CORE_SHELL_ANDROID_TASM_PLATFORM_INVOKER_PROVIDER_ANDROID_H_

#include <memory>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/shell/tasm_platform_invoker_provider.h"

namespace lynx {
namespace shell {

// Android implementation of TasmPlatformInvokerProvider.
// It creates a Java com.lynx.tasm.TasmPlatformInvoker using the
// provided NativeFacade instance, and wraps it with
// TasmPlatformInvokerAndroid.
class TasmPlatformInvokerProviderAndroid : public TasmPlatformInvokerProvider {
 public:
  TasmPlatformInvokerProviderAndroid(JNIEnv* env, jobject native_facade);
  ~TasmPlatformInvokerProviderAndroid() override = default;

  std::unique_ptr<TasmPlatformInvoker> Create() override;

 private:
  // Global reference to the Java NativeFacade used to construct the
  // Java-side TasmPlatformInvoker.
  base::android::ScopedGlobalJavaRef<jobject> native_facade_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_TASM_PLATFORM_INVOKER_PROVIDER_ANDROID_H_
