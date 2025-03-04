// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.base.CalledByNative;

public class AttributeDescriptor {
  private String mName;
  private JavaOnlyArray mValue;

  public AttributeDescriptor(String name, JavaOnlyArray value) {
    this.mName = name;
    this.mValue = value;
  }

  @CalledByNative
  public String getName() {
    return mName;
  }

  @CalledByNative
  public JavaOnlyArray getValue() {
    return mValue;
  }
}
