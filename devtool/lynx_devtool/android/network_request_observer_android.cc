// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <jni.h>

#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/include/platform/android/jni_convert_helper.h"
#include "core/base/android/jni_helper.h"
#include "devtool/lynx_devtool/lynx_devtool_ng.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/LynxNetworkRequestObserverDelegate_jni.h"
#include "platform/android/lynx_devtool/src/main/jni/gen/LynxNetworkRequestObserverDelegate_register_jni.h"

namespace {

std::shared_ptr<lynx::devtool::NetworkRequestObserver> GetNetworkObserver(
    jlong lynx_devtool_ng_ptr) {
  if (lynx_devtool_ng_ptr == 0) {
    return nullptr;
  }
  auto lynx_devtool =
      *reinterpret_cast<std::shared_ptr<lynx::devtool::LynxDevToolNG>*>(
          lynx_devtool_ng_ptr);
  return lynx_devtool != nullptr
             ? lynx_devtool->GetNetworkRequestObserver().lock()
             : nullptr;
}

std::map<std::string, std::string> ConvertHeaders(JNIEnv* env,
                                                  jobject headers) {
  auto jni_headers = lynx::base::android::JNIConvertHelper::
      ConvertJavaStringHashMapToSTLStringMap(env, headers);
  if (jni_headers == nullptr) {
    return {};
  }
  return {std::make_move_iterator(jni_headers->begin()),
          std::make_move_iterator(jni_headers->end())};
}

}  // namespace

jboolean IsEnabled(JNIEnv* env, jobject jcaller, jlong lynxDevToolNGPtr) {
  auto observer = GetNetworkObserver(lynxDevToolNGPtr);
  return observer != nullptr && observer->IsEnabled();
}

jstring RequestWillBeSent(JNIEnv* env, jobject jcaller, jlong lynxDevToolNGPtr,
                          jstring url, jstring method, jobject headers,
                          jbyteArray body) {
  auto observer = GetNetworkObserver(lynxDevToolNGPtr);
  if (observer == nullptr || !observer->IsEnabled()) {
    // Returned to Java; JNI cleans up the local reference on native return.
    // NOLINTNEXTLINE(lynx_custom/new_java_ref)
    return env->NewStringUTF("");
  }
  lynx::devtool::NetworkRequestInfo request;
  request.url =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, url);
  request.method =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, method);
  request.headers = ConvertHeaders(env, headers);
  request.body =
      lynx::base::android::JNIConvertHelper::ConvertJavaBinary(env, body);
  const std::string request_id =
      observer->RequestWillBeSent(std::move(request));
  // request_id is generated internally and contains ASCII characters only.
  // Returned to Java; JNI cleans up the local reference on native return.
  // NOLINTNEXTLINE(lynx_custom/new_java_ref)
  return env->NewStringUTF(request_id.c_str());
}

void ResponseReceived(JNIEnv* env, jobject jcaller, jlong lynxDevToolNGPtr,
                      jstring requestId, jstring url, jint status,
                      jstring statusText, jobject headers) {
  auto observer = GetNetworkObserver(lynxDevToolNGPtr);
  if (observer == nullptr || !observer->IsEnabled()) {
    return;
  }
  lynx::devtool::NetworkResponseInfo response;
  response.url =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, url);
  response.status = status;
  response.status_text =
      lynx::base::android::JNIConvertHelper::ConvertToString(env, statusText);
  response.headers = ConvertHeaders(env, headers);
  observer->ResponseReceived(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, requestId),
      std::move(response));
}

void DataReceived(JNIEnv* env, jobject jcaller, jlong lynxDevToolNGPtr,
                  jstring requestId, jbyteArray data) {
  auto observer = GetNetworkObserver(lynxDevToolNGPtr);
  if (observer == nullptr || !observer->IsEnabled()) {
    return;
  }
  observer->DataReceived(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, requestId),
      lynx::base::android::JNIConvertHelper::ConvertJavaBinary(env, data));
}

void LoadingFinished(JNIEnv* env, jobject jcaller, jlong lynxDevToolNGPtr,
                     jstring requestId) {
  auto observer = GetNetworkObserver(lynxDevToolNGPtr);
  if (observer == nullptr || !observer->IsEnabled()) {
    return;
  }
  observer->LoadingFinished(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, requestId));
}

void LoadingFailed(JNIEnv* env, jobject jcaller, jlong lynxDevToolNGPtr,
                   jstring requestId, jstring errorText, jboolean canceled) {
  auto observer = GetNetworkObserver(lynxDevToolNGPtr);
  if (observer == nullptr || !observer->IsEnabled()) {
    return;
  }
  observer->LoadingFailed(
      lynx::base::android::JNIConvertHelper::ConvertToString(env, requestId),
      lynx::base::android::JNIConvertHelper::ConvertToString(env, errorText),
      static_cast<bool>(canceled));
}

namespace lynx {
namespace devtool {
namespace jni {

bool RegisterJNIForLynxNetworkRequestObserverDelegate(JNIEnv* env) {
  return RegisterNativesImpl(env);
}

}  // namespace jni
}  // namespace devtool
}  // namespace lynx
