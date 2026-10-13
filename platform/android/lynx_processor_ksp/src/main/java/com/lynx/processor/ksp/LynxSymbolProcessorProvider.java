// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor.ksp;

import com.google.devtools.ksp.processing.SymbolProcessor;
import com.google.devtools.ksp.processing.SymbolProcessorEnvironment;
import com.google.devtools.ksp.processing.SymbolProcessorProvider;

public final class LynxSymbolProcessorProvider implements SymbolProcessorProvider {
  @Override
  public SymbolProcessor create(SymbolProcessorEnvironment environment) {
    return new LynxSymbolProcessor(environment);
  }
}
