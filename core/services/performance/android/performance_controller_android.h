// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_ANDROID_PERFORMANCE_CONTROLLER_ANDROID_H_
#define CORE_SERVICES_PERFORMANCE_ANDROID_PERFORMANCE_CONTROLLER_ANDROID_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/public/performance_controller_platform.h"
#include "core/services/performance/performance_controller.h"

namespace lynx {
namespace tasm {
namespace performance {

class PerformanceControllerAndroid
    : public PerformanceControllerPlatform<PerformanceController> {
 public:
  PerformanceControllerAndroid(JNIEnv* env, jobject jni_platform_performance)
      : jni_platform_performance_(env, jni_platform_performance){};

  ~PerformanceControllerAndroid() override;

  static void RegisterJNI(JNIEnv* env);

  void OnPerformanceEvent(const std::unique_ptr<pub::Value>& entry) override;

  void SetActor(const std::shared_ptr<shell::LynxActor<PerformanceController>>&
                    actor) override;

  const std::shared_ptr<shell::LynxActor<PerformanceController>> GetActor() {
    return actor_.lock();
  }

  PerformanceControllerAndroid(const PerformanceControllerAndroid&) = delete;
  PerformanceControllerAndroid& operator=(const PerformanceControllerAndroid&) =
      delete;
  PerformanceControllerAndroid(PerformanceControllerAndroid&&) = default;
  PerformanceControllerAndroid& operator=(PerformanceControllerAndroid&&) =
      default;

 private:
  base::android::ScopedGlobalJavaRef<jobject> jni_platform_performance_;
  std::weak_ptr<lynx::shell::LynxActor<PerformanceController>> actor_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_ANDROID_PERFORMANCE_CONTROLLER_ANDROID_H_
