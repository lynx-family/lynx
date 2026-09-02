// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor.ksp;

import static javax.lang.model.element.Modifier.PUBLIC;
import static javax.lang.model.element.Modifier.STATIC;

import com.google.devtools.ksp.UtilsKt;
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
import com.google.devtools.ksp.symbol.KSFunctionDeclaration;
import com.google.devtools.ksp.symbol.KSNode;
import com.google.devtools.ksp.symbol.KSType;
import com.google.devtools.ksp.symbol.KSValueParameter;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.ParameterizedTypeName;
import com.squareup.javapoet.TypeName;
import com.squareup.javapoet.TypeSpec;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * KSP port of {@code com.lynx.processor.LynxBehaviorProcessor}.
 *
 * <p>Aggregates classes annotated with {@code @LynxBehavior}, {@code @LynxElement} and
 * {@code @LynxShadowNode} into a single {@code BehaviorGenerator} registry class, producing the
 * same Java source as the javac implementation.
 */
public class LynxBehaviorSymbolProcessor implements SymbolProcessor {
  private static final String BEHAVIOR_ANNOTATION = "com.lynx.tasm.behavior.LynxBehavior";
  private static final String ELEMENT_ANNOTATION = "com.lynx.tasm.behavior.LynxElement";
  private static final String SHADOW_NODE_ANNOTATION = "com.lynx.tasm.behavior.LynxShadowNode";
  private static final String GENERATOR_NAME_ANNOTATION =
      "com.lynx.tasm.behavior.LynxGeneratorName";

  private static final String DEFAULT_RENDERER_HOST_TYPE_NAME =
      "com.lynx.tasm.behavior.render.IRendererHost";
  private static final String LYNX_CONTEXT_QUALIFIED = "com.lynx.tasm.behavior.LynxContext";

  private static final ClassName KEEP_TYPE = ClassName.get("androidx.annotation", "Keep");

  private final CodeGenerator mCodeGenerator;
  private final KSPLogger mLogger;
  private boolean isCreated = false;

  private final Map<ClassName, ClassInfo> mBehaviorClasses;
  private final Map<String, ClassInfo> mShadowNodeClasses;
  /** Originating files of every parsed symbol, used for the aggregating output dependency. */
  private final List<KSFile> mSourceFiles;

  public LynxBehaviorSymbolProcessor(SymbolProcessorEnvironment environment) {
    mCodeGenerator = environment.getCodeGenerator();
    mLogger = environment.getLogger();
    mBehaviorClasses = new HashMap<>();
    mShadowNodeClasses = new HashMap<>();
    mSourceFiles = new ArrayList<>();
  }

  @Override
  public List<KSAnnotated> process(Resolver resolver) {
    // The javac processor only emits the registry on the first round (isCreated guard); the
    // registry file name is fixed, so later rounds must not regenerate it.
    if (isCreated) {
      return Collections.emptyList();
    }
    isCreated = true;
    mBehaviorClasses.clear();
    String packageName = "";

    Iterator<KSAnnotated> generatorNameSymbols =
        resolver.getSymbolsWithAnnotation(GENERATOR_NAME_ANNOTATION, false).iterator();
    while (generatorNameSymbols.hasNext()) {
      KSAnnotated symbol = generatorNameSymbols.next();
      try {
        KSAnnotation annotation = KspUtils.findAnnotation(symbol, GENERATOR_NAME_ANNOTATION);
        if (packageName.isEmpty() && annotation != null) {
          String value = KspUtils.argument(annotation, "packageName");
          packageName = value != null ? value : "";
        }
        addSourceFile(symbol);
      } catch (Exception e) {
        error(symbol instanceof KSNode ? (KSNode) symbol : null, e.getMessage());
      }
    }

    Iterator<KSAnnotated> behaviorSymbols =
        resolver.getSymbolsWithAnnotation(BEHAVIOR_ANNOTATION, false).iterator();
    while (behaviorSymbols.hasNext()) {
      KSAnnotated symbol = behaviorSymbols.next();
      if (!(symbol instanceof KSClassDeclaration)) {
        continue;
      }
      KSClassDeclaration classType = (KSClassDeclaration) symbol;
      try {
        ClassName className = KspUtils.toClassName(classType);
        ClassInfo classInfo = parseBehaviorClass(className, classType);
        if (packageName.isEmpty()) {
          packageName = className.packageName();
        }
        mBehaviorClasses.put(className, classInfo);
        addSourceFile(classType);
      } catch (Exception e) {
        error(classType, e.getMessage());
      }
    }

    Iterator<KSAnnotated> elementSymbols =
        resolver.getSymbolsWithAnnotation(ELEMENT_ANNOTATION, false).iterator();
    while (elementSymbols.hasNext()) {
      KSAnnotated symbol = elementSymbols.next();
      if (!(symbol instanceof KSClassDeclaration)) {
        continue;
      }
      KSClassDeclaration classType = (KSClassDeclaration) symbol;
      try {
        ClassName className = KspUtils.toClassName(classType);
        ClassInfo classInfo = parseLynxElementClass(className, classType);
        if (packageName.isEmpty()) {
          packageName = className.packageName();
        }
        mBehaviorClasses.put(className, classInfo);
        addSourceFile(classType);
      } catch (Exception e) {
        error(classType, e.getMessage());
      }
    }

    Iterator<KSAnnotated> shadowNodeSymbols =
        resolver.getSymbolsWithAnnotation(SHADOW_NODE_ANNOTATION, false).iterator();
    while (shadowNodeSymbols.hasNext()) {
      KSAnnotated symbol = shadowNodeSymbols.next();
      if (!(symbol instanceof KSClassDeclaration)) {
        continue;
      }
      KSClassDeclaration classType = (KSClassDeclaration) symbol;
      try {
        ClassName className = KspUtils.toClassName(classType);
        ClassInfo classInfo = parseShadowNodeClass(className, classType);
        if (classInfo.shadowNodeTag != null) {
          mShadowNodeClasses.put(classInfo.shadowNodeTag, classInfo);
        }
        addSourceFile(classType);
      } catch (Exception e) {
        error(classType, e.getMessage());
      }
    }

    if (mBehaviorClasses.size() <= 0) {
      return Collections.emptyList();
    }

    try {
      generateClass(packageName);
    } catch (Exception e) {
      error(null, e.getMessage());
    }

    return Collections.emptyList();
  }

  @Override
  public void finish() {}

  @Override
  public void onError() {}

  private void generateClass(String packageName) throws IOException {
    String className = "BehaviorGenerator";

    TypeSpec holderClass = TypeSpec.classBuilder(className)
                               .addAnnotation(KEEP_TYPE)
                               .addModifiers(PUBLIC)
                               .addMethod(generateMethodInvokerSpec())
                               .build();

    JavaFile javaFile = JavaFile.builder(packageName, holderClass)
                            .addFileComment("Generated by com.lynx.processor.LynxBehaviorProcessor")
                            .build();
    writeAggregating(packageName, className, javaFile);
  }

  /**
   * Mirrors {@code javaFile.writeTo(mFiler)}; the registry aggregates symbols from many files, so
   * the output is registered as aggregating with every originating file as input.
   */
  private void writeAggregating(String packageName, String fileName, JavaFile javaFile)
      throws IOException {
    Dependencies dependencies = new Dependencies(true, mSourceFiles.toArray(new KSFile[0]));
    OutputStream stream = mCodeGenerator.createNewFile(dependencies, packageName, fileName, "java");
    try (Writer writer = new OutputStreamWriter(stream, StandardCharsets.UTF_8)) {
      javaFile.writeTo(writer);
    }
  }

  private MethodSpec generateMethodInvokerSpec() {
    ClassName behavior = ClassName.get("com.lynx.tasm.behavior", "Behavior");
    ClassName list = ClassName.get("java.util", "List");
    ClassName arrayList = ClassName.get("java.util", "ArrayList");
    TypeName listOfBehavior = ParameterizedTypeName.get(list, behavior);

    MethodSpec.Builder builder =
        MethodSpec.methodBuilder("getBehaviors")
            .addModifiers(PUBLIC)
            .addModifiers(STATIC)
            .returns(listOfBehavior)
            .addStatement("$T result = new $T<>()", listOfBehavior, arrayList);
    for (ClassName name : mBehaviorClasses.keySet()) {
      generateClassAndMethod(builder, mBehaviorClasses.get(name));
    }
    for (ClassName name : mBehaviorClasses.keySet()) {
      checkIfShadowNodeOnly(builder, mBehaviorClasses.get(name));
    }

    if (mShadowNodeClasses.size() > 0) {
      generateShadowNodeOnly(builder);
    }

    builder.addStatement("return result");
    return builder.build();
  }

  private void checkIfShadowNodeOnly(MethodSpec.Builder builder, ClassInfo classInfo) {
    for (String tag : classInfo.tagName) {
      if (mShadowNodeClasses.get(tag) != null) {
        mShadowNodeClasses.remove(tag);
        if (mShadowNodeClasses.size() <= 0) {
          // no need generate shadow node only
          return;
        }
      }
    }
  }

  private void generateShadowNodeOnly(MethodSpec.Builder builder) {
    for (String tag : mShadowNodeClasses.keySet()) {
      builder.addCode("result.add(new Behavior($S) {\n", tag);
      ClassName lynxShadowCln = ClassName.get("com.lynx.tasm.behavior.shadow", "ShadowNode");
      builder.addCode("@Override\n");
      builder.addCode("public $T createShadowNode() {\n", lynxShadowCln);
      builder.addCode("return new $T();\n", mShadowNodeClasses.get(tag).mClassName);
      builder.addCode(" }\n");
      builder.addCode("});\n");
    }
  }

  private boolean checkHasContextAndObjectConstructors(KSClassDeclaration classElement) {
    boolean hasParamConstructor = false;

    Iterator<KSFunctionDeclaration> constructors = UtilsKt.getConstructors(classElement).iterator();
    while (constructors.hasNext()) {
      KSFunctionDeclaration constructor = constructors.next();

      List<KSValueParameter> parameters = constructor.getParameters();
      if (parameters.size() != 2) {
        continue;
      }

      String first =
          KspUtils.qualifiedNameOf(parameters.get(0).getType().resolve().getDeclaration());
      String second =
          KspUtils.qualifiedNameOf(parameters.get(1).getType().resolve().getDeclaration());
      // java.lang.Object surfaces as kotlin.Any through KSP.
      if (LYNX_CONTEXT_QUALIFIED.equals(first)
          && ("java.lang.Object".equals(second) || "kotlin.Any".equals(second))) {
        hasParamConstructor = true;
        break;
      }
    }

    return hasParamConstructor;
  }

  private void generateClassAndMethod(MethodSpec.Builder builder, ClassInfo classInfo) {
    boolean createAsync = classInfo.isCreateAsync;
    boolean needProcessDirection = classInfo.needProcessDirection;
    boolean supportFragmentLayerRender = classInfo.supportFragmentLayerRender;

    for (String tag : classInfo.tagName) {
      ClassName lynxContextCln = ClassName.get("com.lynx.tasm.behavior", "LynxContext");
      ClassName lynxUICln = ClassName.get("com.lynx.tasm.behavior.ui", "LynxUI");
      ClassName lynxShadowCln = ClassName.get("com.lynx.tasm.behavior.shadow", "ShadowNode");

      String behaviorArgs = "$S, false, " + (createAsync ? "true" : "false") + ", "
          + (needProcessDirection ? "true" : "false");
      if (supportFragmentLayerRender) {
        behaviorArgs += ", " + (supportFragmentLayerRender ? "true" : "false");
      }

      if (checkHasContextAndObjectConstructors(classInfo.mElement)) {
        builder.addCode("result.add(new Behavior(" + behaviorArgs + ") {\n", tag);
        builder.addCode("@Override\n");
        builder.addCode("public $T createUIWithParams($T context, Object params) {\n", lynxUICln,
            lynxContextCln);
        builder.addCode("return new $T(context, params);\n", classInfo.mClassName);
        builder.addCode(" }\n");
      } else {
        builder.addCode("result.add(new Behavior(" + behaviorArgs + ") {\n", tag);
        builder.addCode("@Override\n");
        builder.addCode("public $T createUI($T context) {\n", lynxUICln, lynxContextCln);
        builder.addCode("return new $T(context);\n", classInfo.mClassName);
        builder.addCode(" }\n");
      }

      if (supportFragmentLayerRender && classInfo.fragmentLayerRendererHost != null) {
        ClassName iRendererHostCln =
            ClassName.get("com.lynx.tasm.behavior.render", "IRendererHost");
        ClassName rendererHostCln = classInfo.fragmentLayerRendererHost;
        builder.addCode("@Override\n");
        builder.addCode("public $T createPlatformRendererHost($T context) {\n", iRendererHostCln,
            lynxContextCln);
        builder.addCode("return new $T(context);\n", rendererHostCln);
        builder.addCode(" }\n");
      }

      if (mShadowNodeClasses.get(tag) != null) {
        builder.addCode("@Override\n");
        builder.addCode("public $T createShadowNode() {\n", lynxShadowCln);
        builder.addCode("return new $T();\n", mShadowNodeClasses.get(tag).mClassName);
        builder.addCode(" }\n");
        builder.addCode("});\n");
      } else {
        builder.addCode("});\n");
      }
    }
  }

  private ClassInfo parseBehaviorClass(ClassName className, KSClassDeclaration typeElement) {
    ClassInfo classInfo = new ClassInfo(className, typeElement);
    classInfo.addBehaviorTag(typeElement);
    classInfo.addBehaviorIsCreateAsync(typeElement);
    classInfo.addBehaviorNeedProcessDirection(typeElement);
    classInfo.addBehaviorSupportFragmentLayerRender(typeElement);
    classInfo.addBehaviorFragmentLayerRendererHost(typeElement);
    return classInfo;
  }

  private ClassInfo parseLynxElementClass(ClassName className, KSClassDeclaration typeElement) {
    ClassInfo classInfo = new ClassInfo(className, typeElement);
    classInfo.addLynxElementTag(typeElement);
    classInfo.addLynxElementIsCreateAsync(typeElement);
    classInfo.addLynxElementNeedProcessDirection(typeElement);
    classInfo.addLynxElementSupportFragmentLayerRender(typeElement);
    classInfo.addLynxElementFragmentLayerRendererHost(typeElement);
    return classInfo;
  }

  private ClassInfo parseShadowNodeClass(ClassName className, KSClassDeclaration typeElement) {
    ClassInfo classInfo = new ClassInfo(className, typeElement);
    classInfo.addShadowNodeTag(typeElement);
    return classInfo;
  }

  private void addSourceFile(KSAnnotated symbol) {
    if (!(symbol instanceof KSDeclaration)) {
      return;
    }
    KSFile file = ((KSDeclaration) symbol).getContainingFile();
    if (file != null && !mSourceFiles.contains(file)) {
      mSourceFiles.add(file);
    }
  }

  private void error(KSNode node, String message) {
    mLogger.error(message != null ? message : "unknown error", node);
  }

  private static class ClassInfo {
    public final ClassName mClassName;
    public final KSClassDeclaration mElement;
    public final List<String> tagName;
    public boolean isCreateAsync;
    public String shadowNodeTag;
    public boolean needProcessDirection;
    public boolean supportFragmentLayerRender;
    public ClassName fragmentLayerRendererHost;

    public ClassInfo(ClassName mClassName, KSClassDeclaration mElement) {
      this.mClassName = mClassName;
      this.mElement = mElement;
      this.tagName = new ArrayList<>();
      this.isCreateAsync = false;
      this.needProcessDirection = false;
      this.supportFragmentLayerRender = false;
      this.fragmentLayerRendererHost = null;
    }

    public void addBehaviorTag(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, BEHAVIOR_ANNOTATION);
      tagName.addAll(KspUtils.stringList(KspUtils.argument(annotation, "tagName")));
    }

    public void addLynxElementTag(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, ELEMENT_ANNOTATION);
      String name = KspUtils.argument(annotation, "name");
      tagName.add(name);
    }

    public void addBehaviorIsCreateAsync(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, BEHAVIOR_ANNOTATION);
      isCreateAsync = booleanArgument(annotation, "isCreateAsync");
    }

    public void addLynxElementIsCreateAsync(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, ELEMENT_ANNOTATION);
      isCreateAsync = booleanArgument(annotation, "isCreateAsync");
    }

    public void addBehaviorNeedProcessDirection(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, BEHAVIOR_ANNOTATION);
      needProcessDirection = booleanArgument(annotation, "needProcessDirection");
    }

    public void addLynxElementNeedProcessDirection(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, ELEMENT_ANNOTATION);
      needProcessDirection = booleanArgument(annotation, "needProcessDirection");
    }

    public void addBehaviorSupportFragmentLayerRender(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, BEHAVIOR_ANNOTATION);
      supportFragmentLayerRender = booleanArgument(annotation, "supportFragmentLayerRender");
    }

    public void addLynxElementSupportFragmentLayerRender(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, ELEMENT_ANNOTATION);
      supportFragmentLayerRender = booleanArgument(annotation, "supportFragmentLayerRender");
    }

    public void addBehaviorFragmentLayerRendererHost(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, BEHAVIOR_ANNOTATION);
      fragmentLayerRendererHost = resolveRendererHost(annotation);
    }

    public void addLynxElementFragmentLayerRendererHost(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, ELEMENT_ANNOTATION);
      fragmentLayerRendererHost = resolveRendererHost(annotation);
    }

    public void addShadowNodeTag(KSClassDeclaration element) {
      KSAnnotation annotation = KspUtils.findAnnotation(element, SHADOW_NODE_ANNOTATION);
      shadowNodeTag = KspUtils.argument(annotation, "tagName");
    }

    private static boolean booleanArgument(KSAnnotation annotation, String name) {
      Boolean value = KspUtils.argument(annotation, name);
      return value != null && value;
    }

    /**
     * Reads the {@code fragmentLayerRendererHost} Class argument. Class-typed annotation values
     * arrive as {@link KSType}; the javac {@code void.class} default surfaces through KSP as
     * {@code kotlin.Unit} (mapped to {@code java.lang.Void} on the Java side), and both sentinels
     * mean "not set", as does the default {@code IRendererHost} itself.
     */
    private static ClassName resolveRendererHost(KSAnnotation annotation) {
      Object value = KspUtils.argument(annotation, "fragmentLayerRendererHost");
      KSDeclaration declaration = null;
      if (value instanceof KSType) {
        declaration = ((KSType) value).getDeclaration();
      } else if (value instanceof KSClassDeclaration) {
        declaration = (KSDeclaration) value;
      }
      if (!(declaration instanceof KSClassDeclaration)) {
        return null;
      }
      String typeName = KspUtils.qualifiedNameOf(declaration);
      if (isFragmentLayerRendererHostSet(typeName)) {
        return KspUtils.toClassName((KSClassDeclaration) declaration);
      }
      return null;
    }
  }

  private static boolean isFragmentLayerRendererHostSet(String typeName) {
    return !"void".equals(typeName) && !"kotlin.Unit".equals(typeName)
        && !"java.lang.Void".equals(typeName) && !"kotlin.Nothing".equals(typeName)
        && !DEFAULT_RENDERER_HOST_TYPE_NAME.equals(typeName);
  }

  /** Entry point registered in META-INF/services. */
  public static class Provider implements SymbolProcessorProvider {
    @Override
    public SymbolProcessor create(SymbolProcessorEnvironment environment) {
      return new LynxBehaviorSymbolProcessor(environment);
    }
  }
}
