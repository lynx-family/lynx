// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.service.security;

import static org.junit.Assert.*;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class SecurityResultTest {
  @Test
  public void testOnSuccess() {
    SecurityResult result = SecurityResult.onSuccess();
    assertTrue(result.isVerified());
    assertEquals(result.getErrorMsg(), null);
  }

  @Test
  public void testOnReject() {
    SecurityResult result = SecurityResult.onReject("failed");
    assertFalse(result.isVerified());
    assertEquals(result.getErrorMsg(), "failed");
  }
}
