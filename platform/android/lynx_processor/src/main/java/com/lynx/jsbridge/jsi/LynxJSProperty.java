// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.jsi;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Used to annotate fields in `LynxJSIObject` that can be accessed by JavaScript, allowing
 * `LynxJSPropertyProcessor` to generate `LynxJSIObjectDescriptor`.
 * Only support for types: boolean, int, long, float, double, String and ILynxJSIObject
 */
@Retention(RetentionPolicy.SOURCE)
@Target({ElementType.FIELD})
public @interface LynxJSProperty {}
