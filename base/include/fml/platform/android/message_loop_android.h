// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_PLATFORM_ANDROID_MESSAGE_LOOP_ANDROID_H_
#define BASE_INCLUDE_FML_PLATFORM_ANDROID_MESSAGE_LOOP_ANDROID_H_

#include <android/looper.h>
#include <jni.h>

#include <atomic>

#include "base/include/fml/macros.h"
#include "base/include/fml/message_loop_impl.h"
#include "base/include/fml/unique_fd.h"

namespace lynx {
namespace fml {

struct UniqueLooperTraits {
  static ALooper* InvalidValue() { return nullptr; }
  static bool IsValid(ALooper* value) { return value != nullptr; }
  static void Free(ALooper* value) { ::ALooper_release(value); }
};

/// Android implementation of \p MessageLoopImpl.
///
/// This implemenation wraps usage of Android's \p looper.
/// \see https://developer.android.com/ndk/reference/group/looper
class MessageLoopAndroid : public MessageLoopImpl {
 public:
  static bool Register(JNIEnv* env);

 private:
  fml::UniqueObject<ALooper*, UniqueLooperTraits> looper_;
  fml::UniqueFD timer_fd_;
  bool running_;

  void Run() override;

  void Terminate() override;

  void OnEventFired();

  FML_FRIEND_MAKE_REF_COUNTED(MessageLoopAndroid);
  FML_FRIEND_REF_COUNTED_THREAD_SAFE(MessageLoopAndroid);
  BASE_DISALLOW_COPY_AND_ASSIGN(MessageLoopAndroid);

 protected:
  void WakeUp(fml::TimePoint time_point) override;

  MessageLoopAndroid();

  ~MessageLoopAndroid() override;
};

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::MessageLoopAndroid;
}  // namespace fml

#endif  // BASE_INCLUDE_FML_PLATFORM_ANDROID_MESSAGE_LOOP_ANDROID_H_
