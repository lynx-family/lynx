// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor;

import static javax.lang.model.element.Modifier.PUBLIC;
import static javax.lang.model.element.Modifier.STATIC;
import static javax.tools.Diagnostic.Kind.ERROR;
import static javax.tools.Diagnostic.Kind.NOTE;
import static javax.tools.Diagnostic.Kind.WARNING;

import androidx.annotation.Keep;
import com.google.auto.service.AutoService;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.behavior.LynxShadowNode;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.ParameterizedTypeName;
import com.squareup.javapoet.TypeName;
import com.squareup.javapoet.TypeSpec;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
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
import javax.lang.model.SourceVersion;
import javax.lang.model.element.Element;
import javax.lang.model.element.TypeElement;
import javax.lang.model.util.Types;

@AutoService(Processor.class)
public class LynxBehaviorProcessor extends AbstractProcessor {
  private Filer mFiler;
  private Messager mMessager;
  private Types mTypes;
  private boolean isCreated = false;

  private final Map<ClassName, ClassInfo> mBehaviorClasses;
  private final Map<String, ClassInfo> mShadowNodeClasses;

  public LynxBehaviorProcessor() {
    this.mBehaviorClasses = new HashMap<>();
    this.mShadowNodeClasses = new HashMap<>();
  }

  @Override
  public SourceVersion getSupportedSourceVersion() {
    return SourceVersion.latestSupported();
  }

  @Override
  public Set<String> getSupportedAnnotationTypes() {
    Set<String> types = new HashSet<>();
    types.add(LynxBehavior.class.getCanonicalName());
    types.add(LynxShadowNode.class.getCanonicalName());
    types.add(LynxGeneratorName.class.getCanonicalName());
    return types;
  }

  @Override
  public synchronized void init(ProcessingEnvironment processingEnvironment) {
    super.init(processingEnvironment);
    mFiler = processingEnv.getFiler();
    mMessager = processingEnv.getMessager();
    mTypes = processingEnv.getTypeUtils();
  }

  @Override
  public boolean process(Set<? extends TypeElement> set, RoundEnvironment roundEnvironment) {
    if (isCreated) {
      return false;
    }
    isCreated = true;
    mBehaviorClasses.clear();
    String packageName = "";

    Set<? extends Element> behaviorNameElement =
        roundEnvironment.getElementsAnnotatedWith(LynxGeneratorName.class);
    if (behaviorNameElement != null) {
      for (Element element : behaviorNameElement) {
        try {
          LynxGeneratorName annotation = element.getAnnotation(LynxGeneratorName.class);
          if (packageName.isEmpty()) {
            packageName = annotation.packageName();
          }
        } catch (Exception e) {
          error(element, e.getMessage());
        }
      }
    }

    Set<? extends Element> elements = roundEnvironment.getElementsAnnotatedWith(LynxBehavior.class);
    if (elements != null) {
      for (Element element : elements) {
        try {
          TypeElement classType = (TypeElement) element;
          ClassName className = ClassName.get(classType);
          ClassInfo classInfo = parseBehaviorClass(className, classType);
          if (packageName.isEmpty()) {
            packageName = className.packageName();
          }
          mBehaviorClasses.put(className, classInfo);
        } catch (Exception e) {
          error(element, e.getMessage());
        }
      }
    }

    Set<? extends Element> shadowNodesElement =
        roundEnvironment.getElementsAnnotatedWith(LynxShadowNode.class);
    if (shadowNodesElement != null) {
      for (Element element : shadowNodesElement) {
        try {
          TypeElement classType = (TypeElement) element;

          ClassName className = ClassName.get(classType);
          ClassInfo classInfo = parseShadowNodeClass(className, classType);
          if (classInfo.shadowNodeTag != null) {
            mShadowNodeClasses.put(classInfo.shadowNodeTag, classInfo);
          }
        } catch (Exception e) {
          error(element, e.getMessage());
        }
      }
    }

    if (mBehaviorClasses.size() <= 0) {
      return false;
    }

    try {
      generateClass(packageName);
    } catch (Exception e) {
      error(e.getMessage());
    }

    return false;
  }

  private void generateClass(String packageName) throws IOException {
    String className = "BehaviorGenerator";

    TypeSpec holderClass = TypeSpec.classBuilder(className)
                               .addAnnotation(Keep.class)
                               .addModifiers(PUBLIC)
                               .addMethod(generateMethodInvokerSpec())
                               .build();

    JavaFile javaFile = JavaFile.builder(packageName, holderClass)
                            .addFileComment("Generated by " + getClass().getName())
                            .build();
    javaFile.writeTo(mFiler);
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

  private void generateClassAndMethod(MethodSpec.Builder builder, ClassInfo classInfo) {
    boolean createAsync = classInfo.isCreateAsync;

    for (String tag : classInfo.tagName) {
      ClassName lynxContextCln = ClassName.get("com.lynx.tasm.behavior", "LynxContext");
      ClassName lynxUICln = ClassName.get("com.lynx.tasm.behavior.ui", "LynxUI");
      ClassName lynxShadowCln = ClassName.get("com.lynx.tasm.behavior.shadow", "ShadowNode");

      builder.addCode(
          "result.add(new Behavior($S, false, " + (createAsync ? "true" : "false") + ") {\n", tag);
      builder.addCode("@Override\n");
      builder.addCode("public $T createUI($T context) {\n", lynxUICln, lynxContextCln);
      builder.addCode("return new $T(context);\n", classInfo.mClassName);
      builder.addCode(" }\n");

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

  private String getClassName(TypeElement type, String packageName) {
    int packageLen = packageName.length() + 1;
    return type.getQualifiedName().toString().substring(packageLen).replace('.', '$');
  }

  private ClassInfo parseBehaviorClass(ClassName className, TypeElement typeElement) {
    ClassInfo classInfo = new ClassInfo(className, typeElement);
    classInfo.addBehaviorTag(typeElement);
    classInfo.addBehaviorIsCreateAsync(typeElement);
    return classInfo;
  }

  private ClassInfo parseShadowNodeClass(ClassName className, TypeElement typeElement) {
    ClassInfo classInfo = new ClassInfo(className, typeElement);
    classInfo.addShadowNodeTag(typeElement);
    return classInfo;
  }

  private void note(String message) {
    mMessager.printMessage(NOTE, message);
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
    public final List<String> tagName;
    public boolean isCreateAsync;
    public String shadowNodeTag;

    public ClassInfo(ClassName mClassName, Element mElement) {
      this.mClassName = mClassName;
      this.mElement = mElement;
      this.tagName = new ArrayList<>();
      this.isCreateAsync = false;
    }

    public void addBehaviorTag(Element element) {
      LynxBehavior annotation = element.getAnnotation(LynxBehavior.class);
      tagName.addAll(Arrays.asList(annotation.tagName()));
    }

    public void addBehaviorIsCreateAsync(Element element) {
      LynxBehavior annotation = element.getAnnotation(LynxBehavior.class);
      isCreateAsync = annotation.isCreateAsync();
    }

    public void addShadowNodeTag(Element element) {
      LynxShadowNode annotation = element.getAnnotation(LynxShadowNode.class);
      shadowNodeTag = annotation.tagName();
    }
  }
}
