// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

/**
 * Status enum for tracking meaningful content loading state in LynxUI components.
 */
public enum MeaningfulContentStatus {
  /** The default value, which means the LynxUI provides no meaningful content. */
  NONE(0),
  /** The LynxUI is loading meaningful content. */
  LOADING(1),
  /** The LynxUI has loaded meaningful content. */
  LOADED(2);

  private final int mValue;

  MeaningfulContentStatus(int value) {
    this.mValue = value;
  }

  public int getValue() {
    return mValue;
  }
}
