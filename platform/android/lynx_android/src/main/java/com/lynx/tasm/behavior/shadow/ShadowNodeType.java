// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

public class ShadowNodeType {
  public static final int COMMON = 1;
  public static final int VIRTUAL = 1 << 1;
  public static final int CUSTOM = 1 << 2;
  public static final int INLINE = 1 << 5;
}
