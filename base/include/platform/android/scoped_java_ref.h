// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_PLATFORM_ANDROID_SCOPED_JAVA_REF_H_
#define BASE_INCLUDE_PLATFORM_ANDROID_SCOPED_JAVA_REF_H_

#include <jni.h>

#include <string>

#include "base/include/fml/macros.h"

namespace lynx {
namespace base {
namespace android {

// Forward declare the generic java reference template class.
template <typename T>
class JavaRef;

// Template specialization of JavaRef, which acts as the base class for all
// other JavaRef<> template types. This allows you to e.g. pass
// ScopedLocalJavaRef<jstring> into a function taking const JavaRef<jobject>&
template <>
class JavaRef<jobject> {
 public:
  JavaRef();

  JavaRef(JNIEnv* env, jobject obj);

  ~JavaRef() {}
#ifndef NDEBUG
  static int GlobalRefCount();

  void SetOwnerName(const std::string& str) { owner_name_ = str; }
  const std::string OwnerName() const { return owner_name_; }

 private:
  std::string owner_name_ = "initial";

 public:
#endif
  // NOLINTNEXTLINE
  JNIEnv* ResetNewLocalRef(JNIEnv* env, jobject obj);

  void ReleaseLocalRef(JNIEnv* env);

  void ResetNewGlobalRef(JNIEnv* env, jobject obj);  // NOLINT

  void ReleaseGlobalRef(JNIEnv* env);

  // NOLINTNEXTLINE
  void ResetNewWeakGlobalRef(JNIEnv* env, jobject obj);

  void ReleaseWeakGlobalRef(JNIEnv* env);

  jobject Get() const { return obj_; }

  bool IsNull() const { return obj_ == nullptr; }

  virtual bool IsLocal() const { return false; }
  virtual bool IsGlobal() const { return false; }
  virtual bool IsWeakGlobal() const { return false; }

 protected:
  jobject obj_ = nullptr;
};

// Generic base class for ScopedLocalJavaRef and ScopedGlobalJavaRef. Useful
// for allowing functions to accept a reference without having to mandate
// whether it is a local or global type.
template <typename T>
class JavaRef : public JavaRef<jobject> {
 public:
  T Get() const { return static_cast<T>(JavaRef<jobject>::Get()); }

 protected:
  JavaRef() {}
  ~JavaRef() {}

  JavaRef(JNIEnv* env, T obj) : JavaRef<jobject>(env, obj) {}

 private:
  BASE_DISALLOW_COPY_AND_ASSIGN(JavaRef);
};

/* Note: If you put ScopedLocalJavaRef in std::vector, be careful to hold or
 * save ScopedLocalJavaRef's obj_(raw pointer) directly for a long time. If
 * vector's reallocation happens, it will use ScopedLocalJavaRef's copy
 * constructor to construct a new one and destruct the old one. The raw pointer
 * you hold will become dangling pointer.
 */
// TODO: Add noexcept in move semantics function.
template <typename T>
class ScopedLocalJavaRef final : public JavaRef<T> {
 public:
  ScopedLocalJavaRef() : env_(nullptr) {}

  ScopedLocalJavaRef(JNIEnv* env, T obj) : JavaRef<T>(env, obj), env_(env) {}

  ScopedLocalJavaRef(const ScopedLocalJavaRef<T>& other) : env_(other.env_) {
    Reset(env_, other.Get());
  }

  template <typename U>
  explicit ScopedLocalJavaRef(const U& other) : env_(NULL) {
    this->Reset(other);
  }

  ScopedLocalJavaRef(ScopedLocalJavaRef<T>&& other) {
    JavaRef<T>::obj_ = other.obj_;
    other.obj_ = nullptr;
  }

  bool IsLocal() const override { return true; }

  void operator=(const ScopedLocalJavaRef<T>& other) {
    env_ = other.env_;
    Reset(env_, other.Get());
  }

  ~ScopedLocalJavaRef() { this->ReleaseLocalRef(env_); }

  // NOLINTNEXTLINE
  void Reset(JNIEnv* env, jobject obj) { this->ResetNewLocalRef(env, obj); }
  void Reset() { this->ReleaseLocalRef(env_); }

  template <typename U>
  void Reset(const ScopedLocalJavaRef<U>& other) {
    // We can copy over env_ here as |other| instance must be from the same
    // thread as |this| local ref. (See class comment for multi-threading
    // limitations, and alternatives).
    this->Reset(other.env_, other.Get());
  }

  template <typename U>
  void Reset(const U& other) {
    // If |env_| was not yet set (is still NULL) it will be attached to the
    // current thread in ResetNewLocalRef().
    this->Reset(env_, other.Get());
  }

 private:
  JNIEnv* env_ = nullptr;
};

/* Note: If you put ScopedGlobalJavaRef in std::vector, be careful to hold or
 * save ScopedGlobalJavaRef's obj_(raw pointer) directly for a long time. If
 * vector's reallocation happens, it will use ScopedGlobalJavaRef's copy
 * constructor to construct a new one and destruct the old one. The raw pointer
 * you hold will become dangling pointer.
 */
// TODO: Add noexcept in move semantics function.
template <typename T>
class ScopedGlobalJavaRef final : public JavaRef<T> {
 public:
  ScopedGlobalJavaRef() {}

  ScopedGlobalJavaRef(JNIEnv* env, T obj) { Reset(env, obj); }

  ScopedGlobalJavaRef(const ScopedGlobalJavaRef<T>& other) {
    Reset(nullptr, other.Get());
  }

  ScopedGlobalJavaRef(const ScopedLocalJavaRef<T>& other) {
    Reset(nullptr, other.Get());
  }

  template <typename U>
  explicit ScopedGlobalJavaRef(const U& other) {
    this->Reset(other);
  }

  ScopedGlobalJavaRef(ScopedGlobalJavaRef<T>&& other) {
    JavaRef<T>::obj_ = other.obj_;
    other.obj_ = nullptr;
  }

  ~ScopedGlobalJavaRef() { this->ReleaseGlobalRef(nullptr); }

  bool IsGlobal() const override { return true; }
  void operator=(const ScopedGlobalJavaRef<T>& other) {
    Reset(nullptr, other.Get());
  }

  void Reset(JNIEnv* env, const ScopedLocalJavaRef<T>& other) {
    this->ResetNewGlobalRef(env, other.Get());  // NOLINT
  }

  // NOLINTNEXTLINE
  void Reset(JNIEnv* env, jobject obj) { this->ResetNewGlobalRef(env, obj); }

  void Reset() { this->ReleaseGlobalRef(nullptr); }

  template <typename U>
  void Reset(const U& other) {
    this->Reset(NULL, other.Get());
  }
};

/* Note: If you put ScopedWeakGlobalJavaRef in std::vector, be careful to hold
 * or save ScopedWeakGlobalJavaRef's obj_(raw pointer) directly for a long time.
 * If vector's reallocation happens, it will use ScopedWeakGlobalJavaRef's copy
 * constructor to construct a new one and destruct the old one. The raw pointer
 * you hold will become dangling pointer.
 */
// TODO: Add noexcept in move semantics function.
template <typename T>
class ScopedWeakGlobalJavaRef final : public JavaRef<T> {
 public:
  ScopedWeakGlobalJavaRef() {}

  ScopedWeakGlobalJavaRef(JNIEnv* env, T obj) { Reset(env, obj); }

  ScopedWeakGlobalJavaRef(const ScopedGlobalJavaRef<T>& other) {
    Reset(nullptr, other.Get());
  }

  ScopedWeakGlobalJavaRef(const ScopedLocalJavaRef<T>& other) {
    Reset(nullptr, other.Get());
  }

  ScopedWeakGlobalJavaRef(const ScopedWeakGlobalJavaRef<T>& other) {
    Reset(nullptr, other.Get());
  }

  ~ScopedWeakGlobalJavaRef() { this->ReleaseWeakGlobalRef(nullptr); }

  bool IsWeakGlobal() const override { return true; }

  void operator=(const ScopedWeakGlobalJavaRef<T>& other) {
    Reset(nullptr, other.Get());
  }

  void Reset(JNIEnv* env, const ScopedLocalJavaRef<T>& other) {
    this->ResetNewWeakGlobalRef(env, other.Get());  // NOLINT
  }

  void Reset(JNIEnv* env, jobject obj) {
    this->ResetNewWeakGlobalRef(env, obj);  // NOLINT
  }
};

class ScopedJavaLocalFrame {
 public:
  explicit ScopedJavaLocalFrame(JNIEnv* env);
  ScopedJavaLocalFrame(JNIEnv* env, int capacity);
  ~ScopedJavaLocalFrame();

 private:
  // This class is only good for use on the thread it was created on so
  // it's safe to cache the non-threadsafe JNIEnv* inside this object.
  JNIEnv* env_;
  BASE_DISALLOW_COPY_AND_ASSIGN(ScopedJavaLocalFrame);
};

}  // namespace android
}  // namespace base
}  // namespace lynx

namespace fml {
namespace jni {
using lynx::base::android::JavaRef;
using lynx::base::android::ScopedGlobalJavaRef;
using lynx::base::android::ScopedJavaLocalFrame;
using lynx::base::android::ScopedLocalJavaRef;
}  // namespace jni
}  // namespace fml

#endif  // BASE_INCLUDE_PLATFORM_ANDROID_SCOPED_JAVA_REF_H_
