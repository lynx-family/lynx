// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lynx_resource_loader_android.h"

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/jni_helper.h"
#include "core/base/lynx_trace_categories.h"
#include "core/build/gen/LynxResourceLoader_jni.h"
#include "core/resource/lynx_resource_setting.h"

using lynx::base::android::AttachCurrentThread;
using lynx::base::android::JNIConvertHelper;
using lynx::base::android::ScopedLocalJavaRef;

// JNI method
void InvokeCallback(JNIEnv* env, jclass jcaller, jlong response_handler,
                    jbyteArray data, jlong bundle_ptr, jint err_code,
                    jstring err_msg) {
  auto* handler = reinterpret_cast<
      lynx::shell::LynxResourceLoaderAndroid::ResponseHandler*>(
      response_handler);
  handler->HandleResponse(
      JNIConvertHelper::ConvertJavaBinary(env, data),
      bundle_ptr != 0 ? reinterpret_cast<void*>(bundle_ptr) : nullptr, err_code,
      JNIConvertHelper::ConvertToString(env, err_msg));
  delete handler;
}

void ConfigLynxResourceSetting(JNIEnv* env, jobject jcaller) {
  lynx::piper::LynxResourceSetting::getInstance()->is_debug_resource_ = true;
}

namespace lynx {
namespace shell {

void LynxResourceLoaderAndroid::RegisterJni(JNIEnv* env) {
  (void)RegisterNativesImpl(env);
}

void LynxResourceLoaderAndroid::LoadResource(
    const pub::LynxResourceRequest& request, bool request_in_current_thread,
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, "LoadResource",
              [&request](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("url", request.url);
              });
  JNIEnv* env = AttachCurrentThread();
  ScopedLocalJavaRef<jobject> local_ref(jni_object_);
  if (local_ref.IsNull()) {
    pub::LynxResourceResponse response{
        .err_code = -1, .err_msg = std::string("Can not find local ref")};
    callback(response);
    return;
  }

  // Delete response_handler after callback
  auto* response_handler = new ResponseHandler(std::move(callback));
  auto j_url = JNIConvertHelper::ConvertToJNIStringUTF(env, request.url);
  Java_LynxResourceLoader_loadResource(
      env, local_ref.Get(), reinterpret_cast<jlong>(response_handler),
      j_url.Get(), static_cast<int>(request.type), request_in_current_thread);
}

void LynxResourceLoaderAndroid::ResponseHandler::HandleResponse(
    std::vector<uint8_t> data, void* bundle_ptr, int32_t err_code,
    std::string err_msg) {
  pub::LynxResourceResponse response{.data = std::move(data),
                                     .bundle = bundle_ptr,
                                     .err_code = err_code,
                                     .err_msg = std::move(err_msg)};
  callback_(response);
}

void LynxResourceLoaderAndroid::SetEnableLynxResourceService(bool enable) {
  JNIEnv* env = AttachCurrentThread();
  ScopedLocalJavaRef<jobject> local_ref(jni_object_);
  if (local_ref.IsNull()) {
    return;
  }
  Java_LynxResourceLoader_setEnableLynxResourceService(env, local_ref.Get(),
                                                       enable);
}

}  // namespace shell
}  // namespace lynx
