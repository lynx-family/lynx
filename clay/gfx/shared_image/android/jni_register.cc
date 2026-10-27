// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "clay/gfx/shared_image/android/jni_register.h"

#include "clay/fml/logging.h"
#include "clay/gfx/shared_image/android/scoped_java_surface.h"
#include "clay/gfx/shared_image/android/surface_texture.h"
#include "clay/gfx/shared_image/android/surface_texture_listener.h"

namespace clay {

bool RegisterSharedImageJNI(JNIEnv *env) {
  bool result = SurfaceTexture::RegisterNatives(env);
  FML_CHECK(result);
  result = SurfaceTextureListener::RegisterNatives(env);
  FML_CHECK(result);
  result = ScopedJavaSurface::RegisterNatives(env);
  FML_CHECK(result);
  return result;
}

}  // namespace clay
