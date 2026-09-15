// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.processor.ksp;

import static javax.lang.model.element.Modifier.PUBLIC;

import com.google.devtools.ksp.processing.CodeGenerator;
import com.google.devtools.ksp.processing.Dependencies;
import com.google.devtools.ksp.processing.KSPLogger;
import com.google.devtools.ksp.processing.Resolver;
import com.google.devtools.ksp.processing.SymbolProcessor;
import com.google.devtools.ksp.processing.SymbolProcessorEnvironment;
import com.google.devtools.ksp.processing.SymbolProcessorProvider;
import com.google.devtools.ksp.symbol.KSAnnotated;
import com.google.devtools.ksp.symbol.KSAnnotation;
import com.google.devtools.ksp.symbol.KSClassDeclaration;
import com.google.devtools.ksp.symbol.KSDeclaration;
import com.google.devtools.ksp.symbol.KSFile;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.TypeSpec;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * KSP port of {@code com.lynx.processor.LynxLibraryProcessor}.
 *
 * <p>Generates a single {@code LynxLibraryProviderImpl} registry entry point for libraries that
 * opt in through the {@code lynx.library.packageName} option, producing the same Java source as
 * the javac implementation. Unlike the props processor this is an aggregating processor: one
 * output file collects every {@code @LynxNativeModule}/{@code @LynxService} class plus the
 * behavior generator reference, so the file is written with aggregating {@link Dependencies}.
 */
public class LynxLibrarySymbolProcessor implements SymbolProcessor {
  public static final String OPTION_LIBRARY_PACKAGE_NAME = "lynx.library.packageName";

  private static final String BEHAVIOR_ANNOTATION = "com.lynx.tasm.behavior.LynxBehavior";
  private static final String ELEMENT_ANNOTATION = "com.lynx.tasm.behavior.LynxElement";
  private static final String NATIVE_MODULE_ANNOTATION = "com.lynx.jsbridge.LynxNativeModule";
  private static final String SERVICE_ANNOTATION = "com.lynx.tasm.service.LynxService";
  private static final String GENERATOR_NAME_ANNOTATION =
      "com.lynx.tasm.behavior.LynxGeneratorName";

  private static final ClassName KEEP_TYPE = ClassName.get("androidx.annotation", "Keep");

  private final CodeGenerator mCodeGenerator;
  private final KSPLogger mLogger;
  private final Map<String, String> mOptions;
  // Containing files of every annotated symbol feeding the aggregated provider output.
  private final Set<KSFile> mSourceFiles = new LinkedHashSet<>();
  // The provider must be generated at most once per compilation; process() runs every round.
  private boolean mCreated;

  public LynxLibrarySymbolProcessor(SymbolProcessorEnvironment environment) {
    mCodeGenerator = environment.getCodeGenerator();
    mLogger = environment.getLogger();
    mOptions = environment.getOptions();
  }

  @Override
  public List<KSAnnotated> process(Resolver resolver) {
    if (mCreated) {
      return Collections.emptyList();
    }

    String providerPackageName = getProviderPackageName();
    if (providerPackageName.length() == 0) {
      return Collections.emptyList();
    }
    mCreated = true;

    mSourceFiles.clear();
    String behaviorGeneratorPackageName = getBehaviorGeneratorPackageName(resolver);
    List<ModuleInfo> modules = getNativeModules(resolver);
    List<ClassName> services = getServices(resolver);
    boolean hasBehaviors = behaviorGeneratorPackageName.length() > 0;

    try {
      generateProvider(
          providerPackageName, behaviorGeneratorPackageName, hasBehaviors, modules, services);
    } catch (IOException e) {
      error(e.getMessage());
    }
    return Collections.emptyList();
  }

  @Override
  public void finish() {}

  @Override
  public void onError() {}

  private String getProviderPackageName() {
    String packageName = mOptions.get(OPTION_LIBRARY_PACKAGE_NAME);
    if (packageName != null && packageName.trim().length() > 0) {
      return packageName.trim();
    }
    // Provider generation is opt-in through the Autolink Gradle plugin. Regular Lynx
    // modules may use the same annotations for behavior generation and should not emit
    // a library provider.
    return "";
  }

  private String getBehaviorGeneratorPackageName(Resolver resolver) {
    Iterator<KSAnnotated> generatorNames =
        resolver.getSymbolsWithAnnotation(GENERATOR_NAME_ANNOTATION, false).iterator();
    while (generatorNames.hasNext()) {
      KSAnnotated symbol = generatorNames.next();
      collectSourceFile(symbol);
      KSAnnotation annotation = KspUtils.findAnnotation(symbol, GENERATOR_NAME_ANNOTATION);
      if (annotation == null) {
        continue;
      }
      String packageName = KspUtils.argument(annotation, "packageName");
      if (packageName != null && packageName.length() > 0) {
        return packageName;
      }
    }
    Iterator<KSAnnotated> behaviors =
        resolver.getSymbolsWithAnnotation(BEHAVIOR_ANNOTATION, false).iterator();
    while (behaviors.hasNext()) {
      KSAnnotated symbol = behaviors.next();
      if (!(symbol instanceof KSClassDeclaration)) {
        continue;
      }
      collectSourceFile(symbol);
      return KspUtils.toClassName((KSClassDeclaration) symbol).packageName();
    }
    Iterator<KSAnnotated> elements =
        resolver.getSymbolsWithAnnotation(ELEMENT_ANNOTATION, false).iterator();
    while (elements.hasNext()) {
      KSAnnotated symbol = elements.next();
      if (!(symbol instanceof KSClassDeclaration)) {
        continue;
      }
      collectSourceFile(symbol);
      return KspUtils.toClassName((KSClassDeclaration) symbol).packageName();
    }
    return "";
  }

  private List<ModuleInfo> getNativeModules(Resolver resolver) {
    List<ModuleInfo> modules = new ArrayList<>();
    Iterator<KSAnnotated> symbols =
        resolver.getSymbolsWithAnnotation(NATIVE_MODULE_ANNOTATION, false).iterator();
    while (symbols.hasNext()) {
      KSAnnotated symbol = symbols.next();
      if (!(symbol instanceof KSClassDeclaration)) {
        continue;
      }
      KSClassDeclaration classType = (KSClassDeclaration) symbol;
      collectSourceFile(classType);
      KSAnnotation annotation = KspUtils.findAnnotation(classType, NATIVE_MODULE_ANNOTATION);
      if (annotation == null) {
        continue;
      }
      String name = KspUtils.argument(annotation, "name");
      modules.add(new ModuleInfo(name, KspUtils.toClassName(classType)));
    }
    return modules;
  }

  private List<ClassName> getServices(Resolver resolver) {
    List<ClassName> services = new ArrayList<>();
    Iterator<KSAnnotated> symbols =
        resolver.getSymbolsWithAnnotation(SERVICE_ANNOTATION, false).iterator();
    while (symbols.hasNext()) {
      KSAnnotated symbol = symbols.next();
      if (!(symbol instanceof KSClassDeclaration)) {
        continue;
      }
      KSClassDeclaration classType = (KSClassDeclaration) symbol;
      collectSourceFile(classType);
      services.add(KspUtils.toClassName(classType));
    }
    return services;
  }

  private void generateProvider(String providerPackageName, String behaviorGeneratorPackageName,
      boolean hasBehaviors, List<ModuleInfo> modules, List<ClassName> services) throws IOException {
    ClassName provider = ClassName.get("com.lynx.tasm.library", "LynxLibraryProvider");
    ClassName registry = ClassName.get("com.lynx.tasm.library", "LynxLibraryRegistry");

    MethodSpec.Builder register = MethodSpec.methodBuilder("register")
                                      .addAnnotation(Override.class)
                                      .addModifiers(PUBLIC)
                                      .addParameter(registry, "registry");
    if (hasBehaviors) {
      register.addStatement("registry.addBehaviors($T.getBehaviors())",
          ClassName.get(behaviorGeneratorPackageName, "BehaviorGenerator"));
    }
    for (ModuleInfo module : modules) {
      register.addStatement("registry.registerModule($S, $T.class)", module.name, module.className);
    }
    for (ClassName service : services) {
      register.addStatement("registry.registerService($T.class)", service);
    }

    TypeSpec providerClass = TypeSpec.classBuilder("LynxLibraryProviderImpl")
                                 .addAnnotation(KEEP_TYPE)
                                 .addModifiers(PUBLIC)
                                 .addSuperinterface(provider)
                                 .addMethod(register.build())
                                 .build();

    JavaFile javaFile = JavaFile.builder(providerPackageName, providerClass)
                            .addFileComment("Generated by com.lynx.processor.LynxLibraryProcessor")
                            .build();

    // Aggregating output: any new or changed annotated class must regenerate the provider, so
    // this bypasses KspUtils.write (which records isolating dependencies).
    Dependencies dependencies = new Dependencies(true, mSourceFiles.toArray(new KSFile[0]));
    OutputStream stream = mCodeGenerator.createNewFile(
        dependencies, providerPackageName, "LynxLibraryProviderImpl", "java");
    try (Writer writer = new OutputStreamWriter(stream, StandardCharsets.UTF_8)) {
      javaFile.writeTo(writer);
    }
  }

  private void collectSourceFile(KSAnnotated symbol) {
    if (!(symbol instanceof KSDeclaration)) {
      return;
    }
    KSFile file = ((KSDeclaration) symbol).getContainingFile();
    if (file != null) {
      mSourceFiles.add(file);
    }
  }

  private void error(String message) {
    mLogger.error(message != null ? message : "unknown error", null);
  }

  private static class ModuleInfo {
    final String name;
    final ClassName className;

    ModuleInfo(String name, ClassName className) {
      this.name = name;
      this.className = className;
    }
  }

  /** Entry point registered in META-INF/services. */
  public static class Provider implements SymbolProcessorProvider {
    @Override
    public SymbolProcessor create(SymbolProcessorEnvironment environment) {
      return new LynxLibrarySymbolProcessor(environment);
    }
  }
}
