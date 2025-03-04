// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.jsi;

import androidx.annotation.Keep;
import androidx.annotation.RestrictTo;

/**
 * Descriptor of JSProperty
 */
@RestrictTo(RestrictTo.Scope.LIBRARY_GROUP)
@Keep
public class LynxJSPropertyDescriptor {
  public LynxJSPropertyDescriptor(String name, String jniDescriptor) {
    this.fieldName = name;
    this.fieldJNIDescriptor = jniDescriptor;
  }

  final public String fieldName;
  final public String fieldJNIDescriptor;
}
