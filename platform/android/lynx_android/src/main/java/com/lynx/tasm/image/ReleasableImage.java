// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.image;

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;

/**
 * An interface for a Bitmap or Drawable that holds a resource and needs to be released.
 * This decouples the user of the Bitmap from the concrete implementation of
 * how the Bitmap is managed and released.
 */
public interface ReleasableImage {
  /**
   * @return The underlying Bitmap object. Can be null if released or invalid.
   */
  Bitmap getBitmap();

  /**
   * @return The underlying Drawable object for animated images. Can be null if released or invalid.
   */
  Drawable getDrawable();

  /**
   * Releases the underlying resource associated with this Bitmap.
   * After calling this method, the Bitmap should not be used anymore.
   */
  void release();
}
