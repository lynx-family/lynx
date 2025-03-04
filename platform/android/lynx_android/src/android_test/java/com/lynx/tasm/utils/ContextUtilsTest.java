// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.*;

import android.content.Context;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Before;
import org.junit.Test;

public class ContextUtilsTest {
  @Before
  public void setUp() {}

  @Test
  public void getContext() {
    assertNull(ContextUtils.getWindow(null));
    assertNull(ContextUtils.getActivity(null));
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    assertNull(ContextUtils.getWindow(context));
    assertNull(ContextUtils.getActivity(context));
    assertNull(ContextUtils.toLynxContext(context));
  }
}
