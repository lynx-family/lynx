// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.lynx.tasm.image.LynxImageRequestOptions;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.junit.Test;

public class LynxImageRequestOptionsTest {
  @Test
  public void testOptionsKeepImmutableFallbackSnapshot() {
    List<String> fallbackUrls = new ArrayList<>(Arrays.asList("fallback-1", "fallback-2"));
    LynxImageRequestOptions options = LynxImageRequestOptions.newBuilder()
                                          .setFinalUrl("final")
                                          .setFallbackUrls(fallbackUrls)
                                          .setUseRGB565(true)
                                          .setPriority(LynxImageRequestOptions.PRIORITY_HIGH)
                                          .build();

    fallbackUrls.add("fallback-3");

    assertEquals("final", options.getFinalUrl());
    assertEquals(Arrays.asList("fallback-1", "fallback-2"), options.getFallbackUrls());
    assertTrue(options.getUseRGB565());
    assertEquals(LynxImageRequestOptions.PRIORITY_HIGH, options.getPriority());
  }
}
