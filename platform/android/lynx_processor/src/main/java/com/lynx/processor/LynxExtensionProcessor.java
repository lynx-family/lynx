// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor;

import static javax.lang.model.element.Modifier.PUBLIC;
import static javax.tools.Diagnostic.Kind.ERROR;

import androidx.annotation.Keep;
import com.google.auto.service.AutoService;
import com.lynx.jsbridge.LynxAutolinkNativeModule;
import com.lynx.tasm.behavior.LynxAutolinkElement;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.service.LynxAutolinkService;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.TypeSpec;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import javax.annotation.processing.AbstractProcessor;
import javax.annotation.processing.Filer;
import javax.annotation.processing.Messager;
import javax.annotation.processing.ProcessingEnvironment;
import javax.annotation.processing.Processor;
import javax.annotation.processing.RoundEnvironment;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.Element;
import javax.lang.model.element.TypeElement;

@AutoService(Processor.class)
public class LynxExtensionProcessor extends AbstractProcessor {
  public static final String OPTION_EXTENSION_PACKAGE_NAME = "lynx.extension.packageName";

  private Filer mFiler;
  private Messager mMessager;
  private boolean mCreated;

  @Override
  public synchronized void init(ProcessingEnvironment processingEnvironment) {
    super.init(processingEnvironment);
    mFiler = processingEnvironment.getFiler();
    mMessager = processingEnvironment.getMessager();
  }

  @Override
  public SourceVersion getSupportedSourceVersion() {
    return SourceVersion.latestSupported();
  }

  @Override
  public Set<String> getSupportedAnnotationTypes() {
    Set<String> annotations = new HashSet<>();
    annotations.add(LynxBehavior.class.getCanonicalName());
    annotations.add(LynxAutolinkElement.class.getCanonicalName());
    annotations.add(LynxAutolinkNativeModule.class.getCanonicalName());
    annotations.add(LynxAutolinkService.class.getCanonicalName());
    annotations.add(LynxGeneratorName.class.getCanonicalName());
    return annotations;
  }

  @Override
  public Set<String> getSupportedOptions() {
    return new HashSet<>(Arrays.asList(OPTION_EXTENSION_PACKAGE_NAME));
  }

  @Override
  public boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv) {
    if (mCreated || roundEnv.processingOver()) {
      return false;
    }

    String providerPackageName = getProviderPackageName();
    if (providerPackageName.length() == 0) {
      return false;
    }
    mCreated = true;

    String behaviorGeneratorPackageName = getBehaviorGeneratorPackageName(roundEnv);
    List<ModuleInfo> modules = getNativeModules(roundEnv);
    List<ClassName> services = getServices(roundEnv);
    boolean hasBehaviors = behaviorGeneratorPackageName.length() > 0;

    try {
      generateProvider(
          providerPackageName, behaviorGeneratorPackageName, hasBehaviors, modules, services);
    } catch (IOException e) {
      error(e.getMessage());
    }
    return false;
  }

  private String getProviderPackageName() {
    Map<String, String> options = processingEnv.getOptions();
    String packageName = options.get(OPTION_EXTENSION_PACKAGE_NAME);
    if (packageName != null && packageName.trim().length() > 0) {
      return packageName.trim();
    }
    // Provider generation is opt-in through the Autolink Gradle plugin. Regular Lynx
    // modules may use the same annotations for behavior generation and should not emit
    // an extension provider.
    return "";
  }

  private String getBehaviorGeneratorPackageName(RoundEnvironment roundEnv) {
    for (Element element : roundEnv.getElementsAnnotatedWith(LynxGeneratorName.class)) {
      LynxGeneratorName annotation = element.getAnnotation(LynxGeneratorName.class);
      if (annotation.packageName().length() > 0) {
        return annotation.packageName();
      }
    }
    for (Element element : roundEnv.getElementsAnnotatedWith(LynxBehavior.class)) {
      return ClassName.get((TypeElement) element).packageName();
    }
    for (Element element : roundEnv.getElementsAnnotatedWith(LynxAutolinkElement.class)) {
      return ClassName.get((TypeElement) element).packageName();
    }
    return "";
  }

  private List<ModuleInfo> getNativeModules(RoundEnvironment roundEnv) {
    List<ModuleInfo> modules = new ArrayList<>();
    for (Element element : roundEnv.getElementsAnnotatedWith(LynxAutolinkNativeModule.class)) {
      TypeElement typeElement = (TypeElement) element;
      LynxAutolinkNativeModule annotation =
          typeElement.getAnnotation(LynxAutolinkNativeModule.class);
      modules.add(new ModuleInfo(annotation.name(), ClassName.get(typeElement)));
    }
    return modules;
  }

  private List<ClassName> getServices(RoundEnvironment roundEnv) {
    List<ClassName> services = new ArrayList<>();
    for (Element element : roundEnv.getElementsAnnotatedWith(LynxAutolinkService.class)) {
      services.add(ClassName.get((TypeElement) element));
    }
    return services;
  }

  private void generateProvider(String providerPackageName, String behaviorGeneratorPackageName,
      boolean hasBehaviors, List<ModuleInfo> modules, List<ClassName> services) throws IOException {
    ClassName provider = ClassName.get("com.lynx.tasm.extension", "LynxExtensionProvider");
    ClassName registry = ClassName.get("com.lynx.tasm.extension", "LynxExtensionRegistry");

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

    TypeSpec providerClass = TypeSpec.classBuilder("LynxExtensionProviderImpl")
                                 .addAnnotation(Keep.class)
                                 .addModifiers(PUBLIC)
                                 .addSuperinterface(provider)
                                 .addMethod(register.build())
                                 .build();

    JavaFile.builder(providerPackageName, providerClass)
        .addFileComment("Generated by " + getClass().getName())
        .build()
        .writeTo(mFiler);
  }

  private void error(String message) {
    mMessager.printMessage(ERROR, message);
  }

  private static class ModuleInfo {
    final String name;
    final ClassName className;

    ModuleInfo(String name, ClassName className) {
      this.name = name;
      this.className = className;
    }
  }
}
