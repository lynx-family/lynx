// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor.ksp;

import com.google.devtools.ksp.processing.CodeGenerator;
import com.google.devtools.ksp.processing.KSPLogger;
import com.google.devtools.ksp.processing.Resolver;
import com.google.devtools.ksp.processing.SymbolProcessor;
import com.google.devtools.ksp.processing.SymbolProcessorEnvironment;
import com.google.devtools.ksp.symbol.KSAnnotated;
import java.util.Collections;
import java.util.List;
import java.util.Map;

final class LynxSymbolProcessor implements SymbolProcessor {
  private final BehaviorGenerator behaviorGenerator;
  private final PropsGenerator propsGenerator;
  private final UIMethodsGenerator uiMethodsGenerator;
  private final JSPropertyGenerator jsPropertyGenerator;
  private final LibraryGenerator libraryGenerator;
  private boolean processed;

  LynxSymbolProcessor(SymbolProcessorEnvironment environment) {
    CodeGenerator codeGenerator = environment.getCodeGenerator();
    KSPLogger logger = environment.getLogger();
    Map<String, String> options = environment.getOptions();
    behaviorGenerator = new BehaviorGenerator(codeGenerator, logger);
    propsGenerator = new PropsGenerator(codeGenerator, logger);
    uiMethodsGenerator = new UIMethodsGenerator(codeGenerator, logger);
    jsPropertyGenerator = new JSPropertyGenerator(codeGenerator, logger);
    libraryGenerator = new LibraryGenerator(codeGenerator, logger, options);
  }

  @Override
  public List<KSAnnotated> process(Resolver resolver) {
    if (processed) {
      return Collections.emptyList();
    }
    processed = true;
    BehaviorGenerator.Result behavior = behaviorGenerator.generate(resolver);
    propsGenerator.generate(resolver);
    uiMethodsGenerator.generate(resolver);
    jsPropertyGenerator.generate(resolver);
    libraryGenerator.generate(resolver, behavior);
    return Collections.emptyList();
  }
}
