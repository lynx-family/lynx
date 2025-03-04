// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import com.lynx.tasm.base.CalledByNative;
import java.lang.reflect.Method;

public class MethodDescriptor {
  Method method;
  String signature;
  String name;

  @CalledByNative
  public String getName() {
    return name;
  }

  @CalledByNative
  public String getSignature() {
    return signature;
  }

  @CalledByNative
  public Method getMethod() {
    return method;
  }
}
