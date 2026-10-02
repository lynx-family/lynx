// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor.ksp;

import static org.junit.Assert.assertTrue;

import com.google.devtools.ksp.processing.SymbolProcessorProvider;
import java.util.ServiceLoader;
import org.junit.Test;

public class LynxSymbolProcessorProviderTest {
  @Test
  public void providerIsDiscoverableByKsp() {
    boolean found = false;
    for (SymbolProcessorProvider provider : ServiceLoader.load(SymbolProcessorProvider.class)) {
      if (provider instanceof LynxSymbolProcessorProvider) {
        found = true;
      }
    }
    assertTrue("KSP must discover LynxSymbolProcessorProvider through META-INF/services", found);
  }
}
