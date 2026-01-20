// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/resource/lynx_resource_loader_android.h"

#include "base/include/log/logging.h"
#include "base/trace/native/trace_event.h"
#include "core/base/android/jni_helper.h"
#include "core/resource/lynx_resource_setting.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxResourceLoader_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/LynxResourceLoader_register_jni.h"

namespace lynx {
namespace jni {
bool RegisterJNIForLynxResourceLoader(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni
}  // namespace lynx

#include "core/resource/trace/resource_trace_event_def.h"

using lynx::base::android::AttachCurrentThread;
using lynx::base::android::JNIConvertHelper;
using lynx::base::android::ScopedLocalJavaRef;

// JNI method
void InvokeCallback(JNIEnv* env, jclass jcaller, jlong response_handler,
                    jbyteArray data, jlong bundle_ptr, jobject bufferPtr,
                    jint err_code, jstring err_msg) {
  auto* handler = reinterpret_cast<
      lynx::shell::LynxResourceLoaderAndroid::ResponseHandler*>(
      response_handler);
  if (handler == nullptr) {
    return;
  }
  std::vector<uint8_t> vec;
  auto* buffer_ptr =
      bufferPtr != nullptr
          ? static_cast<uint8_t*>(env->GetDirectBufferAddress(bufferPtr))
          : nullptr;
  if (buffer_ptr != nullptr) {
    jlong capacity = env->GetDirectBufferCapacity(bufferPtr);
    vec = std::vector<uint8_t>(buffer_ptr, buffer_ptr + capacity);
  } else {
    vec = JNIConvertHelper::ConvertJavaBinary(env, data);
  }
  handler->HandleResponse(
      std::move(vec),
      bundle_ptr != 0 ? reinterpret_cast<void*>(bundle_ptr) : nullptr, err_code,
      JNIConvertHelper::ConvertToString(env, err_msg));
  delete handler;
}

void ConfigLynxResourceSetting(JNIEnv* env, jobject jcaller) {
  lynx::runtime::js::LynxResourceSetting::getInstance()->is_debug_resource_ =
      true;
}

namespace lynx {
namespace shell {

void LynxResourceLoaderAndroid::LoadResource(
    const pub::LynxResourceRequest& request,
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LOAD_RESOURCE,
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
      j_url.Get(), static_cast<int>(request.type),
      request.request_in_current_thread);
}

void LynxResourceLoaderAndroid::LoadBytecode(
    const pub::LynxResourceRequest& request,
    base::MoveOnlyClosure<void, pub::LynxResourceResponse&> callback) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, LOAD_BYTE_CODE,
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

  auto* response_handler = new ResponseHandler(std::move(callback));
  auto j_url = JNIConvertHelper::ConvertToJNIStringUTF(env, request.url);
  Java_LynxResourceLoader_loadBytecode(
      env, local_ref.Get(), reinterpret_cast<jlong>(response_handler),
      j_url.Get(), static_cast<int>(request.type));
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

}  // namespace shell
}  // namespace lynx
