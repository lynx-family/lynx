// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.processor;

import static javax.lang.model.element.Modifier.PUBLIC;
import static javax.tools.Diagnostic.Kind.ERROR;
import static javax.tools.Diagnostic.Kind.WARNING;

import androidx.annotation.Keep;
import com.google.auto.service.AutoService;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodsHolder;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.CodeBlock;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.ParameterizedTypeName;
import com.squareup.javapoet.TypeName;
import com.squareup.javapoet.TypeSpec;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
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
import javax.annotation.processing.SupportedSourceVersion;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.Element;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.TypeElement;
import javax.lang.model.element.VariableElement;
import javax.lang.model.util.Types;

@AutoService(Processor.class)
@SupportedSourceVersion(SourceVersion.RELEASE_8)
public class LynxUIMethodsProcessor extends AbstractProcessor {
  private static final TypeName STRING_TYPE = TypeName.get(String.class);
  private static final TypeName READABLE_MAP_TYPE =
      ClassName.get("com.lynx.react.bridge", "ReadableMap");
  private static final TypeName CALLBACK_TYPE = ClassName.get("com.lynx.react.bridge", "Callback");
  private static final ClassName LYNX_UI_METHOD_INVOKER_TYPE =
      ClassName.get("com.lynx.tasm.behavior.utils", "LynxUIMethodInvoker");
  private static final ClassName LYNX_UI_METHOD_CONSTANTS =
      ClassName.get("com.lynx.tasm.behavior", "LynxUIMethodConstants");

  private Filer mFiler;
  private Messager mMessager;
  private Types mTypes;

  private final Map<ClassName, ClassInfo> mClasses;

  public LynxUIMethodsProcessor() {
    this.mClasses = new HashMap<>();
  }

  @Override
  public synchronized void init(ProcessingEnvironment processingEnv) {
    super.init(processingEnv);

    mFiler = processingEnv.getFiler();
    mMessager = processingEnv.getMessager();
    mTypes = processingEnv.getTypeUtils();
  }

  @Override
  public Set<String> getSupportedAnnotationTypes() {
    HashSet<String> annotationSet = new HashSet<>();
    annotationSet.add(LynxUIMethodsHolder.class.getCanonicalName());
    return annotationSet;
  }

  @Override
  public boolean process(Set<? extends TypeElement> set, RoundEnvironment roundEnv) {
    mClasses.clear();
    Set<? extends Element> elements = roundEnv.getElementsAnnotatedWith(LynxUIMethodsHolder.class);
    System.out.print("LynxUIMethodProcessor: process start size = " + elements.size() + "\n");
    for (Element element : elements) {
      try {
        TypeElement classType = (TypeElement) element;
        System.out.print(
            "LynxUIMethodProcessor: process classType = " + classType.toString() + "\n");
        ClassName className = ClassName.get(classType);
        ClassInfo classInfo = parseClass(className, classType);
        if (classInfo.mMethods.size() > 0) {
          mClasses.put(className, classInfo);
        } else {
          System.out.print("no methods");
        }
      } catch (Exception e) {
        error(element, e.getMessage());
      }
    }

    for (ClassInfo classInfo : mClasses.values()) {
      try {
        generateCode(classInfo);
      } catch (IOException e) {
        error(e.getMessage());
      } catch (Exception e) {
        error(classInfo.mElement, e.getMessage());
      }
    }

    return true;
  }

  private void generateCode(ClassInfo classInfo) throws IOException {
    TypeName superType =
        ParameterizedTypeName.get(LYNX_UI_METHOD_INVOKER_TYPE, classInfo.mClassName);
    ClassName className = classInfo.mClassName;
    String holderClassName =
        getClassName((TypeElement) classInfo.mElement, className.packageName()) + "$$MethodInvoker";
    TypeSpec holderClass = TypeSpec.classBuilder(holderClassName)
                               .addAnnotation(Keep.class)
                               .addSuperinterface(superType)
                               .addModifiers(PUBLIC)
                               .addMethod(generateMethodInvokerSpec(classInfo))
                               .build();

    JavaFile javaFile = JavaFile.builder(className.packageName(), holderClass)
                            .addFileComment("Generated by " + getClass().getName())
                            .addStaticImport(LYNX_UI_METHOD_CONSTANTS, "METHOD_NOT_FOUND")
                            .build();

    javaFile.writeTo(mFiler);
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

    List<Element> methods = classInfo.mMethods;
    for (int i = 0, size = methods.size(); i < size; i++) {
      ExecutableElement method = (ExecutableElement) methods.get(i);
      String methodName = method.getSimpleName().toString();
      builder.add("case \"$L\":\n", methodName).indent();
      builder.add("ui.$L(", methodName);

      List<? extends VariableElement> parameters = method.getParameters();
      if (parameters.size() > 2) {
        throw new IllegalArgumentException("params size of method annotated with LynxUIMethod "
            + "should not be greater than 2, class: " + classInfo.mClassName);
      }

      // add params
      List<String> params = new ArrayList<>();
      for (VariableElement param : parameters) {
        TypeName targetType = TypeName.get(param.asType());
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

  private String getClassName(TypeElement type, String packageName) {
    int packageLen = packageName.length() + 1;
    return type.getQualifiedName().toString().substring(packageLen).replace('.', '$');
  }

  private ClassInfo parseClass(ClassName className, TypeElement typeElement) {
    ClassInfo classInfo = new ClassInfo(className, typeElement);

    // findLynxUIMethods
    while (typeElement != null) {
      for (Element element : typeElement.getEnclosedElements()) {
        LynxUIMethod method = element.getAnnotation(LynxUIMethod.class);
        if (method != null) {
          classInfo.addMethod(element);
        }
      }

      typeElement = (TypeElement) mTypes.asElement(typeElement.getSuperclass());
    }

    return classInfo;
  }

  private void error(Element element, String message) {
    mMessager.printMessage(ERROR, message, element);
  }

  private void error(String message) {
    mMessager.printMessage(ERROR, message);
  }

  private void warning(Element element, String message) {
    mMessager.printMessage(WARNING, message, element);
  }

  private static class ClassInfo {
    public final ClassName mClassName;
    public final Element mElement;
    public final List<Element> mMethods;

    public ClassInfo(ClassName mClassName, Element mElement) {
      this.mClassName = mClassName;
      this.mElement = mElement;
      mMethods = new ArrayList<>();
    }

    public void addMethod(Element info) {
      String name = info.getSimpleName().toString();
      if (checkMethodExists(name)) {
        System.out.print("Module " + mClassName + " has already registered a method named \"" + name
            + "\". If you want to override a method, don't add"
            + "the @LynxUIMethod annotation to the property in the subclass");
        return;
      }

      mMethods.add(info);
    }

    private boolean checkMethodExists(String name) {
      for (Element method : mMethods) {
        if (method.getSimpleName().toString().equals(name)) {
          return true;
        }
      }

      return false;
    }
  }
}
