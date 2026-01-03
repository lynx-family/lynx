// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_ANDROID_DEVTOOL_LIFECYCLE_ANDROID_H_
#define CORE_RENDERER_UTILS_ANDROID_DEVTOOL_LIFECYCLE_ANDROID_H_

#include "core/renderer/utils/devtool_lifecycle.h"

namespace lynx {
namespace tasm {

class DevToolLifecycleAndroid : public DevToolLifecycle::Delegate {
 public:
  DevToolLifecycleAndroid() = default;
  ~DevToolLifecycleAndroid() override = default;

  void SyncStateToPlatform(DevToolState state) override;
};

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_ANDROID_DEVTOOL_LIFECYCLE_ANDROID_H_
