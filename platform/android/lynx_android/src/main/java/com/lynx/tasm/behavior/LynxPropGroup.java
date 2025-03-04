// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior;

import static java.lang.annotation.RetentionPolicy.RUNTIME;

import androidx.annotation.Nullable;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

@Retention(RUNTIME)
@Target(ElementType.METHOD)
public @interface LynxPropGroup {
  String USE_DEFAULT_TYPE = "__default_type__";

  String[] names();

  @Nullable String customType() default USE_DEFAULT_TYPE;

  float defaultFloat() default 0.0f;

  double defaultDouble() default 0.0;

  int defaultInt() default 0;
}
