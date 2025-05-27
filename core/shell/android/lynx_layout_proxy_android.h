// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_SHELL_ANDROID_LYNX_LAYOUT_PROXY_ANDROID_H_
#define CORE_SHELL_ANDROID_LYNX_LAYOUT_PROXY_ANDROID_H_

#include <memory>
#include <utility>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/renderer/ui_wrapper/layout/layout_context.h"
#include "core/shell/lynx_actor_specialization.h"
#include "core/shell/lynx_layout_proxy.h"

namespace lynx {
namespace shell {
class LynxLayoutProxyAndroid {
 public:
  explicit LynxLayoutProxyAndroid(
      std::shared_ptr<shell::LynxActor<tasm::LayoutContext>> actor)
      : layout_proxy_(std::make_unique<LynxLayoutProxy>(std::move(actor))) {}
  ~LynxLayoutProxyAndroid() = default;
  void RunOnLayoutThread(JNIEnv *env, jobject jcaller, jlong nativePtr,
                         jobject java_runnable);

 private:
  std::unique_ptr<LynxLayoutProxy> layout_proxy_;
};
}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_LYNX_LAYOUT_PROXY_ANDROID_H_
