// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/fml/platform/android/jni_weak_ref.h"

#include "base/include/platform/android/jni_utils.h"
#include "clay/fml/logging.h"

namespace fml {
namespace jni {

JavaObjectWeakGlobalRef::JavaObjectWeakGlobalRef() : obj_(NULL) {}

JavaObjectWeakGlobalRef::JavaObjectWeakGlobalRef(
    const JavaObjectWeakGlobalRef& orig)
    : obj_(NULL) {
  Assign(orig);
}

JavaObjectWeakGlobalRef::JavaObjectWeakGlobalRef(JNIEnv* env, jobject obj)
    : obj_(env->NewWeakGlobalRef(obj)) {
  FML_DCHECK(obj_);
}

JavaObjectWeakGlobalRef::~JavaObjectWeakGlobalRef() { reset(); }

void JavaObjectWeakGlobalRef::operator=(const JavaObjectWeakGlobalRef& rhs) {
  Assign(rhs);
}

void JavaObjectWeakGlobalRef::reset() {
  if (obj_) {
    AttachCurrentThread()->DeleteWeakGlobalRef(obj_);
    obj_ = NULL;
  }
}

ScopedLocalJavaRef<jobject> JavaObjectWeakGlobalRef::get(JNIEnv* env) const {
  return GetRealObject(env, obj_);
}

ScopedLocalJavaRef<jobject> GetRealObject(JNIEnv* env, jweak obj) {
  jobject real = NULL;
  if (obj) {
    real = env->NewLocalRef(obj);
    if (!real) {
      FML_DLOG(ERROR) << "The real object has been deleted!";
    }
  }
  return ScopedLocalJavaRef<jobject>(env, real);
}

void JavaObjectWeakGlobalRef::Assign(const JavaObjectWeakGlobalRef& other) {
  if (&other == this) {
    return;
  }

  JNIEnv* env = AttachCurrentThread();
  if (obj_) {
    env->DeleteWeakGlobalRef(obj_);
  }

  obj_ = other.obj_ ? env->NewWeakGlobalRef(other.obj_) : NULL;
}

}  // namespace jni
}  // namespace fml
