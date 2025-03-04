// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_LYNX_TEMPLATE_RENDERER_ANDROID_H_
#define CORE_SHELL_ANDROID_LYNX_TEMPLATE_RENDERER_ANDROID_H_

#include <jni.h>

namespace lynx {
namespace shell {

class LynxTemplateRendererAndroid {
 public:
  LynxTemplateRendererAndroid() = delete;
  static void RegisterJni(JNIEnv* env);
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_LYNX_TEMPLATE_RENDERER_ANDROID_H_
