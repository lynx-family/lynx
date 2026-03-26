// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_TASM_PLATFORM_INVOKER_ANDROID_H_
#define CORE_SHELL_ANDROID_TASM_PLATFORM_INVOKER_ANDROID_H_

#include <memory>
#include <string>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/base/android/java_only_map.h"
#include "core/public/ui_delegate.h"
#include "core/shell/tasm_platform_invoker.h"

namespace lynx {
namespace shell {

class TasmPlatformInvokerAndroid : public TasmPlatformInvoker {
 public:
  TasmPlatformInvokerAndroid(JNIEnv* env, jobject jni_object,
                             tasm::UIDelegate* ui_delegate)
      : jni_object_(env, jni_object),
        weak_ui_delegate_(ui_delegate->GetWeakFlag()) {}
  ~TasmPlatformInvokerAndroid() override = default;

  static base::android::JavaOnlyMap ConvertToJavaOnlyMap(
      const std::shared_ptr<tasm::PageConfig>& config);

  void OnPageConfigDecoded(
      const std::shared_ptr<tasm::PageConfig>& config) override;

  void OnRunPipelineFinished() override;

  std::string TranslateResourceForTheme(const std::string& res_id,
                                        const std::string& theme_key) override;

  lepus::Value TriggerLepusMethod(const std::string& method_name,
                                  const lepus::Value& args) override;

  void TriggerLepusMethodAsync(const std::string& method_name,
                               const lepus::Value& args) override;

  void GetI18nResource(const std::string& channel,
                       const std::string& fallback_url) override;

 private:
  base::android::ScopedGlobalJavaRef<jobject> jni_object_;
  std::weak_ptr<tasm::UIDelegate::WeakFlag> weak_ui_delegate_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_TASM_PLATFORM_INVOKER_ANDROID_H_
