// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.assertNull;

import com.lynx.tasm.image.LynxImageRequestDelegate;
import com.lynx.tasm.image.LynxImageRequestResult;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.image.model.ImageRequestInfoBuilder;
import org.junit.Test;

public class LynxImageRequestDelegateTest {
  @Test
  public void testDefaultImplementation() {
    LynxImageRequestDelegate delegate = new LynxImageRequestDelegate() {};
    ImageRequestInfo requestInfo =
        ImageRequestInfoBuilder.newBuilderWithSource("https://example.com/image.png").build();

    assertNull(delegate.getImageCallerContext());
    assertNull(delegate.prepareImageRequest(requestInfo));
    delegate.onImageRequestFinished(requestInfo, new LynxImageRequestResult(0, null));
  }
}
