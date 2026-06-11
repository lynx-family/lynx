// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.processor.ksp;

import static javax.lang.model.element.Modifier.PUBLIC;

import com.google.devtools.ksp.processing.CodeGenerator;
import com.google.devtools.ksp.processing.KSPLogger;
import com.google.devtools.ksp.processing.Resolver;
import com.google.devtools.ksp.processing.SymbolProcessor;
import com.google.devtools.ksp.processing.SymbolProcessorEnvironment;
import com.google.devtools.ksp.processing.SymbolProcessorProvider;
import com.google.devtools.ksp.symbol.KSAnnotated;
import com.google.devtools.ksp.symbol.KSClassDeclaration;
import com.google.devtools.ksp.symbol.KSDeclaration;
import com.google.devtools.ksp.symbol.KSFunctionDeclaration;
import com.google.devtools.ksp.symbol.KSNode;
import com.google.devtools.ksp.symbol.KSValueParameter;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.CodeBlock;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.ParameterizedTypeName;
import com.squareup.javapoet.TypeName;
import com.squareup.javapoet.TypeSpec;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * KSP port of {@code com.lynx.processor.LynxUIMethodsProcessor}.
 *
 * <p>Generates {@code <Class>$$MethodInvoker} holders for classes annotated with
 * {@code @LynxUIMethodsHolder}, producing the same Java sources as the javac implementation.
 */
public class LynxUIMethodsSymbolProcessor implements SymbolProcessor {
  private static final String UI_METHODS_HOLDER_ANNOTATION =
      "com.lynx.tasm.behavior.LynxUIMethodsHolder";
  private static final String UI_METHOD_ANNOTATION = "com.lynx.tasm.behavior.LynxUIMethod";

  private static final TypeName STRING_TYPE = TypeName.get(String.class);
  private static final TypeName READABLE_MAP_TYPE =
      ClassName.get("com.lynx.react.bridge", "ReadableMap");
  private static final TypeName CALLBACK_TYPE = ClassName.get("com.lynx.react.bridge", "Callback");
  private static final ClassName LYNX_UI_METHOD_INVOKER_TYPE =
      ClassName.get("com.lynx.tasm.behavior.utils", "LynxUIMethodInvoker");
  private static final ClassName LYNX_UI_METHOD_CONSTANTS =
      ClassName.get("com.lynx.tasm.behavior", "LynxUIMethodConstants");
  // The javac processor references androidx.annotation.Keep through Keep.class; using the
  // ClassName keeps the generated import identical without a compile-time androidx dependency.
  private static final ClassName KEEP_TYPE = ClassName.get("androidx.annotation", "Keep");

  private final CodeGenerator mCodeGenerator;
  private final KSPLogger mLogger;
  private final Map<ClassName, ClassInfo> mClasses = new HashMap<>();

  public LynxUIMethodsSymbolProcessor(SymbolProcessorEnvironment environment) {
    mCodeGenerator = environment.getCodeGenerator();
    mLogger = environment.getLogger();
  }

  @Override
  public List<KSAnnotated> process(Resolver resolver) {
    // Clear classes from previous rounds
    mClasses.clear();

    List<KSClassDeclaration> holders = new ArrayList<>();
    Iterator<KSAnnotated> symbols =
        resolver.getSymbolsWithAnnotation(UI_METHODS_HOLDER_ANNOTATION, false).iterator();
    while (symbols.hasNext()) {
      KSAnnotated symbol = symbols.next();
      if (symbol instanceof KSClassDeclaration) {
        holders.add((KSClassDeclaration) symbol);
      }
    }

    System.out.print("LynxUIMethodProcessor: process start size = " + holders.size() + "\n");
    for (KSClassDeclaration classType : holders) {
      try {
        System.out.print("LynxUIMethodProcessor: process classType = "
            + KspUtils.qualifiedNameOf(classType) + "\n");
        ClassName className = KspUtils.toClassName(classType);
        ClassInfo classInfo = parseClass(className, classType);
        if (classInfo.mMethods.size() > 0) {
          mClasses.put(className, classInfo);
        } else {
          System.out.print("no methods");
        }
      } catch (Exception e) {
        error(classType, e.getMessage());
      }
    }

    for (ClassInfo classInfo : mClasses.values()) {
      try {
        generateCode(classInfo);
      } catch (IOException e) {
        error(null, e.getMessage());
      } catch (Exception e) {
        error(classInfo.mDeclaration, e.getMessage());
      }
    }

    return Collections.emptyList();
  }

  @Override
  public void finish() {}

  @Override
  public void onError() {}

  private ClassInfo parseClass(ClassName className, KSClassDeclaration classType) {
    ClassInfo classInfo = new ClassInfo(className, classType);

    // findLynxUIMethods: like the javac processor, walk the whole superclass chain so
    // inherited @LynxUIMethod methods end up in the subclass invoker as well.
    KSClassDeclaration current = classType;
    while (current != null) {
      Iterator<KSDeclaration> declarations = current.getDeclarations().iterator();
      while (declarations.hasNext()) {
        KSDeclaration declaration = declarations.next();
        if (!(declaration instanceof KSFunctionDeclaration)) {
          continue;
        }
        if (KspUtils.findAnnotation(declaration, UI_METHOD_ANNOTATION) != null) {
          classInfo.addMethod((KSFunctionDeclaration) declaration);
        }
      }

      current = KspUtils.superclassOf(current);
    }

    return classInfo;
  }

  private void generateCode(ClassInfo classInfo) throws IOException {
    TypeName superType =
        ParameterizedTypeName.get(LYNX_UI_METHOD_INVOKER_TYPE, classInfo.mClassName);
    ClassName className = classInfo.mClassName;
    String holderClassName =
        getClassName(classInfo.mDeclaration, className.packageName()) + "$$MethodInvoker";
    TypeSpec holderClass = TypeSpec.classBuilder(holderClassName)
                               .addAnnotation(KEEP_TYPE)
                               .addSuperinterface(superType)
                               .addModifiers(PUBLIC)
                               .addMethod(generateMethodInvokerSpec(classInfo))
                               .build();

    JavaFile javaFile =
        JavaFile
            .builder(className.packageName(), holderClass)
            // Hardcoded to the javac processor FQCN so the output stays byte-identical.
            .addFileComment("Generated by com.lynx.processor.LynxUIMethodsProcessor")
            .addStaticImport(LYNX_UI_METHOD_CONSTANTS, "METHOD_NOT_FOUND")
            .build();

    KspUtils.write(mCodeGenerator, classInfo.mDeclaration.getContainingFile(),
        className.packageName(), holderClassName, javaFile);
  }

  private static MethodSpec generateMethodInvokerSpec(ClassInfo classInfo) {
    MethodSpec.Builder builder = MethodSpec.methodBuilder("invoke")
                                     .addModifiers(PUBLIC)
                                     .addAnnotation(Override.class)
                                     .returns(TypeName.VOID);

    builder.addParameter(classInfo.mClassName, "ui")
        .addParameter(STRING_TYPE, "methodName")
        .addParameter(READABLE_MAP_TYPE, "params")
        .addParameter(CALLBACK_TYPE, "callback");

    builder.addCode(generateMethodInvokerCodeBlock(classInfo));

    return builder.build();
  }

  private static CodeBlock generateMethodInvokerCodeBlock(ClassInfo classInfo) {
    CodeBlock.Builder builder = CodeBlock.builder();
    builder.add("switch (methodName) {\n").indent();

    List<KSFunctionDeclaration> methods = classInfo.mMethods;
    for (int i = 0, size = methods.size(); i < size; i++) {
      KSFunctionDeclaration method = methods.get(i);
      String methodName = method.getSimpleName().asString();
      builder.add("case \"$L\":\n", methodName).indent();
      builder.add("ui.$L(", methodName);

      List<KSValueParameter> parameters = method.getParameters();
      if (parameters.size() > 2) {
        throw new IllegalArgumentException("params size of method annotated with LynxUIMethod "
            + "should not be greater than 2, class: " + classInfo.mClassName);
      }

      // add params
      List<String> params = new ArrayList<>();
      for (KSValueParameter param : parameters) {
        TypeName targetType = KspUtils.toTypeName(param.getType().resolve());
        if (targetType.equals(READABLE_MAP_TYPE)) {
          params.add("params");
        } else if (targetType.equals(CALLBACK_TYPE)) {
          params.add("callback");
        }
      }
      builder.add(String.join(",", params));
      builder.addStatement(")");
      builder.addStatement("break").unindent();
    }
    builder.add("default:\n").indent();
    builder.addStatement("callback.invoke(METHOD_NOT_FOUND)");
    builder.addStatement("break").unindent();
    builder.unindent().add("}\n");

    return builder.build();
  }

  private static String getClassName(KSClassDeclaration type, String packageName) {
    int packageLen = packageName.length() + 1;
    return KspUtils.qualifiedNameOf(type).substring(packageLen).replace('.', '$');
  }

  private void error(KSNode node, String message) {
    mLogger.error(message != null ? message : "unknown error", node);
  }

  private static class ClassInfo {
    final ClassName mClassName;
    final KSClassDeclaration mDeclaration;
    final List<KSFunctionDeclaration> mMethods;

    ClassInfo(ClassName className, KSClassDeclaration declaration) {
      mClassName = className;
      mDeclaration = declaration;
      mMethods = new ArrayList<>();
    }

    void addMethod(KSFunctionDeclaration method) {
      String name = method.getSimpleName().asString();
      if (checkMethodExists(name)) {
        System.out.print("Module " + mClassName + " has already registered a method named \"" + name
            + "\". If you want to override a method, don't add"
            + "the @LynxUIMethod annotation to the property in the subclass");
        return;
      }

      mMethods.add(method);
    }

    private boolean checkMethodExists(String name) {
      for (KSFunctionDeclaration method : mMethods) {
        if (method.getSimpleName().asString().equals(name)) {
          return true;
        }
      }

      return false;
    }
  }

  /** Entry point registered in META-INF/services. */
  public static class Provider implements SymbolProcessorProvider {
    @Override
    public SymbolProcessor create(SymbolProcessorEnvironment environment) {
      return new LynxUIMethodsSymbolProcessor(environment);
    }
  }
}
