// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "base/include/platform/android/scoped_java_ref.h"

#include <vector>

#include "base/include/platform/android/jni_utils.h"
namespace lynx {
namespace base {
namespace android {

static constexpr int32_t kDefaultLocalFrameCapacity = 16;

JavaRef<jobject>::JavaRef() = default;

JavaRef<jobject>::JavaRef(JNIEnv* env, jobject obj) : obj_(obj) {}

// NOLINTNEXTLINE
JNIEnv* JavaRef<jobject>::ResetNewLocalRef(JNIEnv* env, jobject obj) {
  if (!env) {
    env = AttachCurrentThread();
  }
  if (obj) {
    obj = env->NewLocalRef(obj);  // NOLINT
  }

  if (obj_) env->DeleteLocalRef(obj_);
  obj_ = obj;

  return env;
}

void JavaRef<jobject>::ReleaseLocalRef(JNIEnv* env) {
  if (!obj_) {
    return;
  }
  if (!env) {
    env = AttachCurrentThread();
  }
  env->DeleteLocalRef(obj_);
  obj_ = nullptr;
}

void JavaRef<jobject>::ResetNewGlobalRef(JNIEnv* env, jobject obj) {  // NOLINT
  if (!env) {
    env = AttachCurrentThread();
  }

  if (obj) obj = env->NewGlobalRef(obj);  // NOLINT
  if (obj_) env->DeleteGlobalRef(obj_);   // NOLINT
  obj_ = obj;
}

void JavaRef<jobject>::ReleaseGlobalRef(JNIEnv* env) {
  if (obj_ == nullptr) {
    return;
  }
  if (!env) {
    env = AttachCurrentThread();
  }

  if (!env) {
    // Oppo Android 5.1, JNIEnv is null when destruct global JavaRef in thread
    return;
  }

  env->DeleteGlobalRef(obj_);  // NOLINT
  obj_ = nullptr;
}

// NOLINTNEXTLINE
void JavaRef<jobject>::ResetNewWeakGlobalRef(JNIEnv* env, jobject obj) {
  if (!env) {
    env = AttachCurrentThread();
  }
  if (obj) obj = env->NewWeakGlobalRef(obj);  // NOLINT
  if (obj_) env->DeleteWeakGlobalRef(obj_);   // NOLINT
  obj_ = obj;
}

void JavaRef<jobject>::ReleaseWeakGlobalRef(JNIEnv* env) {
  if (obj_ == nullptr) {
    return;
  }
  if (!env) {
    env = AttachCurrentThread();
  }
  env->DeleteWeakGlobalRef(obj_);  // NOLINT
  obj_ = nullptr;
}

ScopedJavaLocalFrame::ScopedJavaLocalFrame(JNIEnv* env) : env_(env) {
  env_->PushLocalFrame(kDefaultLocalFrameCapacity);
}

ScopedJavaLocalFrame::ScopedJavaLocalFrame(JNIEnv* env, int capacity)
    : env_(env) {
  env_->PushLocalFrame(capacity);
}

ScopedJavaLocalFrame::~ScopedJavaLocalFrame() { env_->PopLocalFrame(nullptr); }

}  // namespace android
}  // namespace base
}  // namespace lynx
