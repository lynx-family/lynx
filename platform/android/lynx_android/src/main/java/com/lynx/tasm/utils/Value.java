// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

public class Value {
  public final float value;
  public final Unit unit;

  public Value(float value, Unit unit) {
    this.value = value;
    this.unit = unit;
  }

  public enum Unit { PX, PERCENTAGE }
}
