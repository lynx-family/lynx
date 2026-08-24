// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_LYNX_ADAPTOR_FRAME_CHILD_RUNTIME_ANDROID_H_
#define CLAY_LYNX_ADAPTOR_FRAME_CHILD_RUNTIME_ANDROID_H_

#include <jni.h>

#include <memory>

namespace lynx {
namespace tasm {

class FrameChildRuntimeFactory;

std::shared_ptr<FrameChildRuntimeFactory>
CreateFrameChildRuntimeFactoryAndroid(JNIEnv* env, jobject factory);

}  // namespace tasm
}  // namespace lynx

#endif  // CLAY_LYNX_ADAPTOR_FRAME_CHILD_RUNTIME_ANDROID_H_
