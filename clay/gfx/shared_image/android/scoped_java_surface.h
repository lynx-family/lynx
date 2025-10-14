// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CLAY_GFX_SHARED_IMAGE_ANDROID_SCOPED_JAVA_SURFACE_H_
#define CLAY_GFX_SHARED_IMAGE_ANDROID_SCOPED_JAVA_SURFACE_H_

#include <jni.h>

#include <cstddef>

#include "base/include/fml/macros.h"
#include "base/include/platform/android/scoped_java_ref.h"

namespace clay {

class SurfaceTexture;

// A helper class for holding a scoped reference to a Java Surface instance.
// When going out of scope, Surface.release() is called on the Java object to
// make sure server-side references (esp. wrt graphics memory) are released.
class ScopedJavaSurface {
 public:
  ScopedJavaSurface();
  explicit ScopedJavaSurface(std::nullptr_t);

  // Wraps an existing Java Surface object in a ScopedJavaSurface.
  ScopedJavaSurface(const fml::jni::JavaRef<jobject>& surface,
                    bool auto_release);

  // Creates a Java Surface from a SurfaceTexture and wraps it in a
  // ScopedJavaSurface.
  explicit ScopedJavaSurface(const SurfaceTexture* surface_texture);

  // Move constructor. Take the surface from another ScopedJavaSurface object,
  // the latter no longer owns the surface afterwards.
  ScopedJavaSurface(ScopedJavaSurface&& rvalue);
  ScopedJavaSurface& operator=(ScopedJavaSurface&& rhs);

  ~ScopedJavaSurface();

  // Make a copy that does not retain ownership. Client is responsible for not
  // using the copy after this is destroyed.
  ScopedJavaSurface CopyRetainOwnership() const;

  // Checks whether the surface is an empty one.
  bool IsEmpty() const;

  // Checks whether this object references a valid surface.
  bool IsValid() const;

  const fml::jni::JavaRef<jobject>& j_surface() const { return j_surface_; }

  BASE_DISALLOW_COPY_AND_ASSIGN(ScopedJavaSurface);

 private:
  // Performs destructive move from |other| to this.
  void MoveFrom(ScopedJavaSurface& other);
  void ReleaseSurfaceIfNeeded();

  bool auto_release_ = true;

  fml::jni::ScopedGlobalJavaRef<jobject> j_surface_;
};

}  // namespace clay

#endif  // CLAY_GFX_SHARED_IMAGE_ANDROID_SCOPED_JAVA_SURFACE_H_
