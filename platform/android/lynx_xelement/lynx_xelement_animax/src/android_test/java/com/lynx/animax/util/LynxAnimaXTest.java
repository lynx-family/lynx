// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.util;

import static org.mockito.Mockito.mock;

import com.lynx.animax.UIAnimaX;
import com.lynx.animax.util.LynxAnimaX;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.LynxUI;
import junit.framework.TestCase;
import org.junit.Test;

public class LynxAnimaXTest extends TestCase {
  @Test
  public void testAnimaXInit() {
    LynxAnimaX.inst().init();
    assertTrue(LynxAnimaX.inst().hasInitialized());
  }

  @Test
  public void testCreateUI() {
    LynxContext context = mock(LynxContext.class);
    LynxUI ui = LynxAnimaX.inst().createUI(context);
    assertTrue(ui == null || ui instanceof UIAnimaX);
  }
}
