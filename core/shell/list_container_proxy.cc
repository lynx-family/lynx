// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <jni.h>
#include "core/shell/list_container_proxy.h"
#include "platform/android/lynx_android/src/main/jni/gen/ListContainerProxy_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/ListContainerProxy_register_jni.h"
#include "core/shell/lynx_shell.h"

namespace lynx {
namespace jni {

bool RegisterJNIForListContainerProxy(JNIEnv* env) {
  return RegisterNativesImpl(env);
}
}  // namespace jni


namespace shell { 
void ListContainerProxy::ScrollByListContainer(int32_t tag, float offset_x, float offset_y,
                              float original_x, float original_y) {

  if (auto engine_actor = engine_actor_.lock()) {
    engine_actor->Act(
      [tag, offset_x, offset_y, original_x, original_y](auto& engine) {
        engine->ScrollByListContainer(tag, offset_x, offset_y, original_x,
                                      original_y);
      });

  }
}

void ListContainerProxy::ScrollToPosition(int32_t tag, int index, float offset, int align,
                         bool smooth) {
  if (auto engine_actor = engine_actor_.lock()) {
    engine_actor->Act(
      [tag, index, offset, align, smooth](auto& engine) {
        engine->ScrollToPosition(tag, index, offset, align, smooth);
      });
  }   
}

void ListContainerProxy::ScrollStopped(int32_t tag) {   
  if (auto engine_actor = engine_actor_.lock()) {
    engine_actor->Act(
      [tag](auto& engine) {
      engine->ScrollStopped(tag); 
      });
  }   
}

}  // namespace shell
}  // namespace lynx


// ———————————— JNI method start ————————————————

jlong Create(JNIEnv* env, jobject jcaller,
    jlong ptr){

   auto shell = reinterpret_cast<lynx::shell::LynxShell*>(ptr);
   auto container_proxy = new lynx::shell::ListContainerProxy(shell->GetEngineActor());
    return reinterpret_cast<jlong>(container_proxy);

  }

 void ScrollByListContainer(JNIEnv* env, jobject jcaller, jlong ptr,
                           jlong lifecycle, jint sign, jfloat dx, jfloat dy,
                           jfloat originalX, jfloat originalY) {

  reinterpret_cast<lynx::shell::ListContainerProxy*>(ptr) ->ScrollByListContainer(sign, dx, dy, originalX, originalY);                        
}

void ScrollToPosition(JNIEnv* env, jobject jcaller, jlong ptr, jlong lifecycle,
                      jint sign, jint position, jfloat offset, jint align,
                      jboolean smooth) {
  reinterpret_cast<lynx::shell::ListContainerProxy*>(ptr) ->ScrollToPosition(sign, position, offset, align, smooth);                        
}

void ScrollStopped(JNIEnv* env, jobject jcaller, jlong ptr, jlong lifecycle,
                   jint sign) {
  reinterpret_cast<lynx::shell::ListContainerProxy*>(ptr) ->ScrollStopped(sign);                        
}   


void Destroy(JNIEnv* env, jobject jcaller,
    jlong ptr){
 lynx::shell::ListContainerProxy* obj = reinterpret_cast<lynx::shell::ListContainerProxy*>(ptr);
    delete obj;
    };

// --------   JNI method End ————————————
