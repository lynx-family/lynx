// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SHELL_ANDROID_LYNX_NODE_INFO_RESULT_ANDROID_H_
#define CORE_SHELL_ANDROID_LYNX_NODE_INFO_RESULT_ANDROID_H_

#include <vector>

#include "core/base/android/android_jni.h"
#include "core/renderer/template_assembler.h"

namespace lynx {
namespace shell {

class LynxNodeInfoResultAndroid {
 public:
  explicit LynxNodeInfoResultAndroid(
      const std::vector<tasm::TemplateAssembler::NodeInfo>& node_infos);

  inline jobject jni_object() { return jni_object_.Get(); }

 private:
  base::android::ScopedGlobalJavaRef<jobject> jni_object_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_SHELL_ANDROID_LYNX_NODE_INFO_RESULT_ANDROID_H_
