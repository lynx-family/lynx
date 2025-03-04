// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_DATA_ANDROID_PLATFORM_DATA_ANDROID_H_
#define CORE_RENDERER_DATA_ANDROID_PLATFORM_DATA_ANDROID_H_

#include <string>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/renderer/data/platform_data.h"
#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {
class PlatformDataAndroid : public PlatformData {
 public:
  PlatformDataAndroid(
      const base::android::ScopedGlobalJavaRef<jobject>& template_data)
      : template_data_(template_data) {}

  virtual ~PlatformDataAndroid() override;

 private:
  void EnsureConvertData() override;

  base::android::ScopedGlobalJavaRef<jobject> template_data_;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_DATA_ANDROID_PLATFORM_DATA_ANDROID_H_
