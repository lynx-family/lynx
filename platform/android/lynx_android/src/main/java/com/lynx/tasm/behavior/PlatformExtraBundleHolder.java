// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import androidx.annotation.Keep;
import com.lynx.tasm.base.CalledByNative;
import java.util.HashMap;
import java.util.Map;

/**
 * Helper class used in native code for passing user defined bundle from ShadowNode to corresponding
 * LynxUI.
 * It is created on **Layout thread** and released on **TASM thread**
 */
@Keep
public class PlatformExtraBundleHolder {
  private final Map<Integer, Object> mBundleHolder = new HashMap<>();

  // Do not allow create instance in Java code
  private PlatformExtraBundleHolder() {}

  /**
   * This method is called by native code on **TASM thread**
   * @param signature ShadowNode signature id
   * @return corresponding bundle create by ShadowNode or null if no bundle found.
   */
  @CalledByNative
  public Object getBundle(int signature) {
    return mBundleHolder.get(signature);
  }

  /**
   * This method is called by native code on **Layout thread**
   * @param signature ShadowNode signature id
   * @param bundle user defined bundle create in ShadowNode after layout
   */
  @CalledByNative
  public void putBundle(int signature, Object bundle) {
    if (bundle == null) {
      return;
    }

    mBundleHolder.put(signature, bundle);
  }

  /**
   * This method is called by native code on **Layout thread** when a new layout loop is begin
   * @return new PlatformExtraBundleHolder instance
   */
  @CalledByNative
  static public PlatformExtraBundleHolder generateHolder() {
    return new PlatformExtraBundleHolder();
  }
}
