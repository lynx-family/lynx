// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_DEVTOOL_WRAPPER_ANDROID_DEVTOOL_POOL_ANDROID_H_
#define CORE_DEVTOOL_WRAPPER_ANDROID_DEVTOOL_POOL_ANDROID_H_

#include <jni.h>

#include <memory>
#include <mutex>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/devtool_wrapper/devtool_pool.h"

namespace lynx {
namespace devtool {

class DevToolPoolAndroid : public DevToolPool {
 public:
  DevToolPoolAndroid(JNIEnv* env, jobject jcaller);
  ~DevToolPoolAndroid() override = default;

  void CreateDevTool() override;
  void PopDevTool() override;

  std::shared_ptr<lepus::InspectorLepusObserver> OnMTSRuntimeCreated() override;

  void Destroy();

 private:
  // CreateDevTool() and OnMTSRuntimeCreated() are called from the NormalTask
  // thread, PopDevTool() is called from the TASM thread, and Destroy() may be
  // called from other thread. So we need to protect the weak_android_delegate_
  // with a mutex.
  std::mutex mutex_;
  base::android::ScopedWeakGlobalJavaRef<jobject> weak_android_delegate_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // CORE_DEVTOOL_WRAPPER_ANDROID_DEVTOOL_POOL_ANDROID_H_
