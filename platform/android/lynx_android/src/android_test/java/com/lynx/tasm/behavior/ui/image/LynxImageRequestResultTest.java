// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.assertEquals;

import com.lynx.tasm.image.LynxImageRequestResult;
import org.junit.Test;

public class LynxImageRequestResultTest {
  @Test
  public void testResultKeepsStatus() {
    LynxImageRequestResult result = new LynxImageRequestResult(0, null);

    assertEquals(0, result.getErrorCode());
  }
}
