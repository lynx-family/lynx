// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.config;

import androidx.annotation.RestrictTo;
import com.lynx.BuildConfig;

/**
 * The LynxLiteConfig class stores a series of configuration switches that limit certain behaviors
 * of Lynx. These switches ensure that certain parts of the code are surrounded by dead branches in
 * the produced Android AAR, with the goal of creating a more lightweight Android product.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
public class LynxLiteConfigs {
  public static boolean supportCustomEmojiInInput() {
    return !BuildConfig.enable_lite;
  }

  public static boolean supportCustomEmojiInText() {
    return !BuildConfig.enable_lite;
  }

  public static boolean enablePrimJSTrail() {
    return !BuildConfig.enable_lite;
  }

  public static boolean enableNewGesture() {
    return !BuildConfig.enable_lite;
  }

  // LynxLite statically compiles certain libraries into the liblynx.so
  public static boolean requireQuickSharedLibrary() {
    return !BuildConfig.enable_lite;
  }

  // Many APIs in Fresco 2.x are not compatible with 1.x.
  public static boolean enableNewFresco() {
    return !BuildConfig.enable_lite;
  }
}
