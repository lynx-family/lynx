// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

/**
 * Preferred color scheme for media query evaluation (prefers-color-scheme).
 */
public enum LynxColorScheme {
  LIGHT(0),
  DARK(1);

  private final int mId;

  LynxColorScheme(int id) {
    mId = id;
  }

  public int id() {
    return mId;
  }
}
