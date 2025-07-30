// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.service.image;

import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import com.facebook.common.references.CloseableReference;
import com.facebook.imagepipeline.image.CloseableBitmap;
import com.facebook.imagepipeline.image.CloseableImage;
import com.lynx.tasm.image.ReleasableImage;

/**
 * An implementation of ReleasableBitmap that wraps a Fresco's CloseableReference.
 * It is responsible for holding the reference and releasing it when it's no longer needed.
 */
public class FrescoReleasableImage implements ReleasableImage {
  private final CloseableReference<CloseableImage> mImageReference;
  private final Bitmap mBitmap;
  private final Drawable mDrawable;

  public FrescoReleasableImage(CloseableReference<CloseableImage> imageReference) {
    this.mImageReference = imageReference;

    CloseableImage image = (this.mImageReference != null) ? this.mImageReference.get() : null;
    if (image instanceof CloseableBitmap) {
      this.mBitmap = ((CloseableBitmap) image).getUnderlyingBitmap();
    } else {
      this.mBitmap = null;
    }
    this.mDrawable = null;
  }

  public FrescoReleasableImage(
      Drawable drawable, CloseableReference<CloseableImage> imageReference) {
    this.mImageReference = imageReference;
    this.mDrawable = drawable;
    this.mBitmap = null;
  }

  @Override
  public Bitmap getBitmap() {
    return mBitmap;
  }

  @Override
  public Drawable getDrawable() {
    return mDrawable;
  }

  @Override
  public void release() {
    if (mImageReference != null) {
      CloseableReference.closeSafely(mImageReference);
    }
  }
}
