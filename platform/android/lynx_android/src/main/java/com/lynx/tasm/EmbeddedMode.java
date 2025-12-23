// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import androidx.annotation.IntDef;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Embedded mode is an experimental switch
 * When embeddedMode is set, we offer optimal performance for embedded scenarios.
 * But it will restrict business flexibility.
 * Please DO NOT enable this switch on your own for now.
 * Contact the Lynx team for more information.
 * Embedded mode configuration options using bitwise operations for multiple selections
 * Usage:
 * 1. Basic usage:
 *    - Use UNSET for no options selected
 *    - Use EMBEDDED_MODE_BASE for basic optimizations
 *    - Use EMBEDDED_MODE_ALL for all optimizations
 *
 * 2. Combine options:
 *    - Use bitwise OR (|) to combine options
 *    - Example: EMBEDDED_MODE_BASE | ENGINE_POOL
 *
 * 3. Check options:
 *    - Use bitwise AND (&) to check if an option is enabled
 *    - Example: (mode & ENGINE_POOL) != 0
 */
public final class EmbeddedMode {
  // Private constructor to prevent instantiation
  private EmbeddedMode() {}
  @IntDef(
      flag = true, // 'flag = true' is crucial, it tells the compiler these can be combined with '|'
      value = {UNSET, EMBEDDED_MODE_BASE, ENGINE_POOL, LAYOUT_IN_ELEMENT, EMBEDDED_MODE_ALL})
  @Retention(RetentionPolicy.SOURCE)
  public @interface Mode {}
  /**
   * No optimization options selected
   */
  public static final int UNSET = 0;
  /**
   * Basic embedded mode with minimal optimizations
   */
  public static final int EMBEDDED_MODE_BASE = 1 << 0;
  /**
   * Engine pool optimization in embedded mode
   */
  public static final int ENGINE_POOL = 1 << 1;
  /**
   * Layout in Element
   */
  public static final int LAYOUT_IN_ELEMENT = 1 << 2;

  /**
   * Generate render content based native `Fragment` layer.
   */
  public static final int FRAGMENT_LAYER_RENDER = 1 << 3;

  /**
   * Use TextService to layout and render text.
   */
  public static final int USE_TEXT_SERVICE = 1 << 4;

  /**
   * Combination of all optimization options
   * <p>
   * Note: When adding new optimization options, update this value
   */
  public static final int EMBEDDED_MODE_ALL = EMBEDDED_MODE_BASE | ENGINE_POOL | LAYOUT_IN_ELEMENT;

  public static boolean isEnginePoolEnable(@EmbeddedMode.Mode int mode) {
    return (mode & EmbeddedMode.ENGINE_POOL) > 0 && isBaseModeEnable(mode);
  }

  public static boolean isBaseModeEnable(@EmbeddedMode.Mode int mode) {
    return (mode & EmbeddedMode.EMBEDDED_MODE_BASE) > 0;
  }
}
