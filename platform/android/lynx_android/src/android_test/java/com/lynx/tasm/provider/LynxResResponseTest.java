// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.provider;

import junit.framework.TestCase;

public class LynxResResponseTest extends TestCase {
  private LynxResResponse res;
  public void setUp() throws Exception {
    super.setUp();
    res = new LynxResResponse();
  }

  public void testGetReasonPhrase() {
    assertNotNull(res.getReasonPhrase());
  }
}
