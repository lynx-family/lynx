// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include <jni.h>
#include "core/shell/list_container_proxy.h"
#include "platform/android/lynx_android/src/main/jni/gen/ListContainerProxy_jni.h"
#include "platform/android/lynx_android/src/main/jni/gen/ListContainerProxy_register_jni.h"
#include "core/shell/lynx_shell.h"
#include "core/shell/list_engine_proxy.h"

namespace lynx {
    namespace jni {

        bool RegisterJNIForListContainerProxy(JNIEnv *env) {
            return RegisterNativesImpl(env);
        }
    }  // namespace jni
    namespace shell {
        void ListContainerProxy::ScrollByListContainer(int32_t tag, float offset_x, float offset_y,
                                                       float original_x, float original_y) {
            if (list_engine_proxy_) {
                list_engine_proxy_->ScrollByListContainer(tag, offset_x, offset_y, original_x,
                                                          original_y);
            }
        }

        void ListContainerProxy::ScrollToPosition(int32_t tag, int index, float offset, int align,
                                                  bool smooth) {
            if (list_engine_proxy_) {
                list_engine_proxy_->ScrollToPosition(tag, index, offset, align, smooth);
            }
        }

        void ListContainerProxy::ScrollStopped(int32_t tag) {
            if (list_engine_proxy_) {
                list_engine_proxy_->ScrollStopped(tag);
            }
        }

    }    // namespace shell 

}  // namespace lynx


// ———————————— JNI method start ————————————————

jlong Create(JNIEnv *env, jobject jcaller,
             jlong ptr) {

    auto shell = reinterpret_cast<lynx::shell::LynxShell *>(ptr);
    auto container_proxy = new lynx::shell::ListContainerProxy(shell->GetListEngineProxy());
    return reinterpret_cast<jlong>(container_proxy);

}

void ScrollByListContainer(JNIEnv *env, jobject jcaller, jlong ptr,
                           jlong lifecycle, jint sign, jfloat dx, jfloat dy,
                           jfloat originalX, jfloat originalY) {
    auto container_proxy = reinterpret_cast<lynx::shell::ListContainerProxy *>(ptr);
    container_proxy->ScrollByListContainer(sign, dx, dy, originalX, originalY);

}

void ScrollToPosition(JNIEnv *env, jobject jcaller, jlong ptr, jlong lifecycle,
                      jint sign, jint position, jfloat offset, jint align,
                      jboolean smooth) {
    auto container_proxy = reinterpret_cast<lynx::shell::ListContainerProxy *>(ptr);
    container_proxy->ScrollToPosition(sign, position, offset, align, smooth)
}

void ScrollStopped(JNIEnv *env, jobject jcaller, jlong ptr, jlong lifecycle,
                   jint sign) {
    auto container_proxy = reinterpret_cast<lynx::shell::ListContainerProxy *>(ptr);
    container_proxy->ScrollStopped(sign);
    
}


void Destroy(JNIEnv *env, jobject jcaller,
             jlong ptr) {
    lynx::shell::ListContainerProxy *obj = reinterpret_cast<lynx::shell::ListContainerProxy *>(ptr);
    delete obj;
};

// --------   JNI method End ————————————
