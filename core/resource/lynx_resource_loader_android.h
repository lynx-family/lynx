// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RESOURCE_LYNX_RESOURCE_LOADER_ANDROID_H_
#define CORE_RESOURCE_LYNX_RESOURCE_LOADER_ANDROID_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/platform/android/scoped_java_ref.h"
#include "core/public/lynx_resource_loader.h"

namespace lynx {
namespace shell {

class LynxResourceLoaderAndroid : public pub::LynxResourceLoader {
 public:
  class ResponseHandler {
   public:
    ResponseHandler(
        base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback)
        : callback_(std::move(callback)) {}
    void HandleResponse(std::vector<uint8_t> data, void* bundle_ptr,
                        int32_t err_code, std::string err_msg);

   private:
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback_;
  };

  LynxResourceLoaderAndroid(JNIEnv* env, jobject jni_object)
      : jni_object_(env, jni_object) {}
  ~LynxResourceLoaderAndroid() override = default;

  static void RegisterJni(JNIEnv* env);

  void LoadResource(const pub::LynxResourceRequest& request,
                    bool request_in_current_thread,
                    base::MoveOnlyClosure<void, pub::LynxResourceResponse&>
                        callback) override;

  void SetEnableLynxResourceService(bool enable) override;

 private:
  base::android::ScopedWeakGlobalJavaRef<jobject> jni_object_;
};

}  // namespace shell
}  // namespace lynx

#endif  // CORE_RESOURCE_LYNX_RESOURCE_LOADER_ANDROID_H_
