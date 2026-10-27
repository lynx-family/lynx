// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android/android_hardwarebuffer_utils.h"

#include <dlfcn.h>
#include <sys/system_properties.h>

#include "clay/fml/logging.h"

namespace clay {
AHardwareBufferUtils::AHardwareBufferUtils() {
  char sdk_version_string[PROP_VALUE_MAX];
  if (__system_property_get("ro.build.version.sdk", sdk_version_string)) {
    int api_level = atoi(sdk_version_string);
    if (api_level < 26) {
      FML_LOG(ERROR) << "Sdk version < 26, AHardwareBuffer is not supported.";
      return;
    }
  }
  const char* lib = "libandroid.so";
  void* handle = dlopen(lib, RTLD_NOW | RTLD_NODELETE | RTLD_LOCAL);
  if (!handle) {
    FML_DLOG(ERROR) << "Failed to open libandroid.so for AHardwareBuffer!";
    return;
  }
  AHardwareBuffer_describe_ = reinterpret_cast<AHardwareBuffer_describe_type>(
      dlsym(handle, "AHardwareBuffer_describe"));
  if (!AHardwareBuffer_describe_) {
    FML_DLOG(ERROR) << "Failed to find AHardwareBuffer_describe!";
    return;
  }
  AHardwareBuffer_acquire_ = reinterpret_cast<AHardwareBuffer_acquire_type>(
      dlsym(handle, "AHardwareBuffer_acquire"));
  if (!AHardwareBuffer_acquire_) {
    FML_DLOG(ERROR) << "Failed to find AHardwareBuffer_acquire!";
    return;
  }
  AHardwareBuffer_release_ = reinterpret_cast<AHardwareBuffer_release_type>(
      dlsym(handle, "AHardwareBuffer_release"));
  if (!AHardwareBuffer_release_) {
    FML_DLOG(ERROR) << "Failed to find AHardwareBuffer_release!";
    return;
  }
  AHardwareBuffer_allocate_ = reinterpret_cast<AHardwareBuffer_allocate_type>(
      dlsym(handle, "AHardwareBuffer_allocate"));
  if (!AHardwareBuffer_allocate_) {
    FML_DLOG(ERROR) << "Failed to find AHardwareBuffer_allocate!";
    return;
  }
  AHardwareBuffer_lock_ = reinterpret_cast<AHardwareBuffer_lock_type>(
      dlsym(handle, "AHardwareBuffer_lock"));
  if (!AHardwareBuffer_lock_) {
    FML_DLOG(ERROR) << "Failed to find AHardwareBuffer_lock!";
    return;
  }
  AHardwareBuffer_unlock_ = reinterpret_cast<AHardwareBuffer_unlock_type>(
      dlsym(handle, "AHardwareBuffer_unlock"));
  if (!AHardwareBuffer_unlock_) {
    FML_DLOG(ERROR) << "Failed to find AHardwareBuffer_unlock!";
    return;
  }
  handle = dlopen("libEGL.so", RTLD_NOW | RTLD_NODELETE | RTLD_LOCAL);
  if (!handle) {
    FML_DLOG(ERROR) << "Failed to open libEGL.so for AHardwareBuffer!";
    return;
  }
  eglGetNativeClientBufferANDROID_ =
      reinterpret_cast<eglGetNativeClientBufferANDROID_type>(
          dlsym(handle, "eglGetNativeClientBufferANDROID"));
  if (!eglGetNativeClientBufferANDROID_) {
    FML_DLOG(ERROR) << "Failed to find eglGetNativeClientBufferANDROID!";
    return;
  }
  is_support_available_ = true;
}

// static
AHardwareBufferUtils& AHardwareBufferUtils::GetInstance() {
  static AHardwareBufferUtils instance;
  return instance;
}

void AHardwareBufferUtils::Describe(const AHardwareBuffer* buffer,
                                    AHardwareBuffer_Desc* outDesc) {
  FML_DCHECK(AHardwareBuffer_describe_);
  AHardwareBuffer_describe_(buffer, outDesc);
}

void AHardwareBufferUtils::Acquire(AHardwareBuffer* buffer) {
  FML_DCHECK(AHardwareBuffer_acquire_);
  AHardwareBuffer_acquire_(buffer);
}

void AHardwareBufferUtils::Release(AHardwareBuffer* buffer) {
  FML_DCHECK(AHardwareBuffer_release_);
  AHardwareBuffer_release_(buffer);
}

int AHardwareBufferUtils::Allocate(const AHardwareBuffer_Desc* desc,
                                   AHardwareBuffer** outBuffer) {
  FML_DCHECK(AHardwareBuffer_allocate_);
  return AHardwareBuffer_allocate_(desc, outBuffer);
}

int AHardwareBufferUtils::Lock(AHardwareBuffer* buffer, uint64_t usage,
                               int32_t fence, const ARect* rect,
                               void** out_virtual_address) {
  FML_DCHECK(AHardwareBuffer_lock_);
  return AHardwareBuffer_lock_(buffer, usage, fence, rect, out_virtual_address);
}

int AHardwareBufferUtils::Unlock(AHardwareBuffer* buffer, int32_t* fence) {
  FML_DCHECK(AHardwareBuffer_unlock_);
  return AHardwareBuffer_unlock_(buffer, fence);
}

EGLClientBuffer AHardwareBufferUtils::GetNativeClientBuffer(
    const AHardwareBuffer* buffer) {
  return eglGetNativeClientBufferANDROID_(buffer);
}

bool AHardwareBufferUtils::IsSupportAvailable() {
  return is_support_available_;
}
}  // namespace clay
