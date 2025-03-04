// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/base/android/lynx_white_board_android.h"

#include <memory>

#include "core/base/android/android_jni.h"
#include "core/build/gen/LynxWhiteBoard_jni.h"
#include "core/shared_data/lynx_white_board.h"

jlong Create(JNIEnv* env, jobject jcaller) {
  auto* white_board = new lynx::tasm::WhiteBoard();
  auto* sp_white_board =
      new std::shared_ptr<lynx::tasm::WhiteBoard>(white_board);
  return reinterpret_cast<int64_t>(sp_white_board);
}

void Destroy(JNIEnv* env, jobject jcaller, jlong ptr) {
  std::shared_ptr<lynx::tasm::WhiteBoard>* white_board =
      reinterpret_cast<std::shared_ptr<lynx::tasm::WhiteBoard>*>(ptr);
  delete white_board;
}

namespace lynx {
namespace base {

void LynxWhiteBoardAndroid::RegisterJni(JNIEnv* env) {
  RegisterNativesImpl(env);
}
}  // namespace base
}  // namespace lynx
