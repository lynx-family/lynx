// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/renderer/data/android/platform_data_android.h"

#include "base/include/platform/android/jni_utils.h"
#include "core/renderer/dom/android/lynx_view_data_manager_android.h"

namespace lynx {
namespace tasm {

void PlatformDataAndroid::EnsureConvertData() {
  if (value_converted_from_platform_data_.IsEmpty() &&
      template_data_.Get() != nullptr) {
    JNIEnv* env = base::android::AttachCurrentThread();
    value_converted_from_platform_data_ =
        LynxViewDataManagerAndroid::GetJsThreadDataFromTemplateData(
            env, template_data_.Get());
  }
}

PlatformDataAndroid::~PlatformDataAndroid() {
  JNIEnv* env = base::android::AttachCurrentThread();
  LynxViewDataManagerAndroid::ConsumeTemplateDataActions(env,
                                                         template_data_.Get());
}

}  // namespace tasm
}  // namespace lynx
