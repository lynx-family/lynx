// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.image;

import java.util.Map;

public class LynxImageConfig {
  /**
   * Whether to enable image resource hint.
   * If enabled, it may provide hints to the image loader about the resource type
   * to optimize the loading process.
   */
  private boolean mEnableImageResourceHint = false;

  /**
   * Whether to enable small disk cache for images.
   * If enabled, small images will be stored in a separate disk cache to prevent them from evicting
   * larger images from the main cache, improving overall cache efficiency.
   */
  private boolean mEnableImageSmallDiskCache = false;

  /**
   * Whether to enable progressive rendering for images.
   * If enabled, images can be displayed gradually as they download,
   * providing a better user experience on slow networks.
   */
  private boolean mEnableProgressiveRendering = false;

  /**
   * Custom parameters for image loading.
   * A map of key-value pairs that can be passed to the image loader for custom configurations
   * or tracking purposes.
   */
  private Map<String, String> mImageCustomParams;

  public boolean getEnableImageResourceHint() {
    return mEnableImageResourceHint;
  }

  public void setEnableImageResourceHint(boolean enableImageResourceHint) {
    mEnableImageResourceHint = enableImageResourceHint;
  }

  public boolean getEnableImageSmallDiskCache() {
    return mEnableImageSmallDiskCache;
  }

  public void setEnableImageSmallDiskCache(boolean enableImageSmallDiskCache) {
    mEnableImageSmallDiskCache = enableImageSmallDiskCache;
  }

  public boolean getEnableProgressiveRendering() {
    return mEnableProgressiveRendering;
  }

  public void setEnableProgressiveRendering(boolean enableProgressiveRendering) {
    mEnableProgressiveRendering = enableProgressiveRendering;
  }

  public void setImageCustomParam(Map<String, String> imageCustomParam) {
    mImageCustomParams = imageCustomParam;
  }

  public Map<String, String> getImageCustomParam() {
    return mImageCustomParams;
  }
}
