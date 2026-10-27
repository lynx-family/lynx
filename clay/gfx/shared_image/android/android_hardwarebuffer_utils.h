// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_ANDROID_HARDWAREBUFFER_UTILS_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_ANDROID_HARDWAREBUFFER_UTILS_H_

#include <EGL/egl.h>
#include <android/hardware_buffer.h>

namespace clay {

typedef void (*AHardwareBuffer_describe_type)(const AHardwareBuffer*,
                                              AHardwareBuffer_Desc* desc);
typedef void (*AHardwareBuffer_acquire_type)(AHardwareBuffer*);
typedef void (*AHardwareBuffer_release_type)(AHardwareBuffer*);
typedef int (*AHardwareBuffer_allocate_type)(const AHardwareBuffer_Desc*,
                                             AHardwareBuffer**);
typedef int (*AHardwareBuffer_lock_type)(AHardwareBuffer* buffer,
                                         uint64_t usage, int32_t fence,
                                         const ARect* rect,
                                         void** outVirtualAddress);
typedef int (*AHardwareBuffer_unlock_type)(AHardwareBuffer* buffer,
                                           int32_t* fence);

typedef EGLClientBuffer (*eglGetNativeClientBufferANDROID_type)(
    const AHardwareBuffer*);

class AHardwareBufferUtils {
 public:
  static AHardwareBufferUtils& GetInstance();

  AHardwareBufferUtils(const AHardwareBufferUtils&) = delete;
  AHardwareBufferUtils& operator=(const AHardwareBufferUtils&) = delete;

  bool IsSupportAvailable();
  int Allocate(const AHardwareBuffer_Desc* desc, AHardwareBuffer** outBuffer);
  void Acquire(AHardwareBuffer* buffer);
  void Release(AHardwareBuffer* buffer);
  void Describe(const AHardwareBuffer* buffer, AHardwareBuffer_Desc* outDesc);
  int Lock(AHardwareBuffer* buffer, uint64_t usage, int32_t fence,
           const ARect* rect, void** out_virtual_address);
  int Unlock(AHardwareBuffer* buffer, int32_t* fence);

  EGLClientBuffer GetNativeClientBuffer(const AHardwareBuffer* buffer);

 private:
  AHardwareBufferUtils();

  AHardwareBuffer_describe_type AHardwareBuffer_describe_;
  AHardwareBuffer_acquire_type AHardwareBuffer_acquire_;
  AHardwareBuffer_release_type AHardwareBuffer_release_;
  AHardwareBuffer_allocate_type AHardwareBuffer_allocate_;
  AHardwareBuffer_lock_type AHardwareBuffer_lock_;
  AHardwareBuffer_unlock_type AHardwareBuffer_unlock_;
  eglGetNativeClientBufferANDROID_type eglGetNativeClientBufferANDROID_;

  bool is_support_available_ = false;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_ANDROID_HARDWAREBUFFER_UTILS_H_
