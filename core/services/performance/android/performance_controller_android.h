// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_SERVICES_PERFORMANCE_ANDROID_PERFORMANCE_CONTROLLER_ANDROID_H_
#define CORE_SERVICES_PERFORMANCE_ANDROID_PERFORMANCE_CONTROLLER_ANDROID_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/services/performance/performance_controller.h"

namespace lynx {
namespace tasm {
namespace performance {

/// @class PerformanceControllerActorWrapper
/// @brief Shared reference wrapper for cross-language actor safety
/// This class provides thread-safe access to
/// lynx::shell::LynxActor<lynx::tasm::performance::PerformanceController>
/// instances across Java/C++ boundaries.
/// **Lifecycle Ownership**:
/// - Created/released exclusively by Java layer (PerformanceController.java)
/// - Native pointer stored as jlong in Java for deterministic cleanup
/// @see PerformanceControllerAndroid For native-side implementation.
/// @see PerformanceController.java For Java-side ownership management.
struct PerformanceControllerActorWrapper {
  PerformanceControllerActorWrapper() = default;
  ~PerformanceControllerActorWrapper() = default;
  PerformanceControllerActorWrapper(const PerformanceControllerActorWrapper&) =
      delete;
  PerformanceControllerActorWrapper& operator=(
      const PerformanceControllerActorWrapper&) = delete;
  PerformanceControllerActorWrapper(PerformanceControllerActorWrapper&&) =
      delete;
  PerformanceControllerActorWrapper& operator=(
      PerformanceControllerActorWrapper&&) = delete;

  std::shared_ptr<
      lynx::shell::LynxActor<lynx::tasm::performance::PerformanceController>>
      performance_controller_actor_;
};

class PerformanceControllerAndroid : public PerformanceController {
 public:
  explicit PerformanceControllerAndroid(JNIEnv* env,
                                        jobject jni_platform_performance)
      : jni_platform_performance_(env, jni_platform_performance){};

  ~PerformanceControllerAndroid() override = default;

  static void RegisterJNI(JNIEnv* env);

  void OnPerformanceEvent(
      const std::unique_ptr<lynx::pub::Value> entry) override;

  void SetActor(
      const std::shared_ptr<lynx::shell::LynxActor<PerformanceController>>&
          actor);

  PerformanceControllerAndroid(const PerformanceControllerAndroid&) = delete;
  PerformanceControllerAndroid& operator=(const PerformanceControllerAndroid&) =
      delete;
  PerformanceControllerAndroid(PerformanceControllerAndroid&&) = default;
  PerformanceControllerAndroid& operator=(PerformanceControllerAndroid&&) =
      default;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> jni_platform_performance_;
};

}  // namespace performance
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_SERVICES_PERFORMANCE_ANDROID_PERFORMANCE_CONTROLLER_ANDROID_H_
