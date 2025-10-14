// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_FML_PLATFORM_ANDROID_JNI_WEAK_REF_H_
#define CLAY_FML_PLATFORM_ANDROID_JNI_WEAK_REF_H_

#include <jni.h>

#include "base/include/platform/android/scoped_java_ref.h"

namespace fml {
namespace jni {

// Manages WeakGlobalRef lifecycle.
// This class is not thread-safe w.r.t. get() and reset(). Multiple threads may
// safely use get() concurrently, but if the user calls reset() (or of course,
// calls the destructor) they'll need to provide their own synchronization.
class JavaObjectWeakGlobalRef {
 public:
  JavaObjectWeakGlobalRef();

  JavaObjectWeakGlobalRef(const JavaObjectWeakGlobalRef& orig);

  JavaObjectWeakGlobalRef(JNIEnv* env, jobject obj);

  virtual ~JavaObjectWeakGlobalRef();

  void operator=(const JavaObjectWeakGlobalRef& rhs);

  ScopedLocalJavaRef<jobject> get(JNIEnv* env) const;

  bool is_empty() const { return obj_ == NULL; }

  void reset();

 private:
  void Assign(const JavaObjectWeakGlobalRef& rhs);

  jweak obj_;
};

// Get the real object stored in the weak reference returned as a
// ScopedLocalJavaRef.
ScopedLocalJavaRef<jobject> GetRealObject(JNIEnv* env, jweak obj);

}  // namespace jni
}  // namespace fml

#endif  // CLAY_FML_PLATFORM_ANDROID_JNI_WEAK_REF_H_
