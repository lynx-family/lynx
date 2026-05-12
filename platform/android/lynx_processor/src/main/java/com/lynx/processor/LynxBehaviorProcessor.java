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
import com.lynx.tasm.behavior.LynxAutolinkElement;
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
import javax.lang.model.element.ElementKind;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.TypeElement;
import javax.lang.model.type.MirroredTypeException;
import javax.lang.model.type.TypeMirror;
import javax.lang.model.util.Types;

@AutoService(Processor.class)
public class LynxBehaviorProcessor extends AbstractProcessor {
  private static final String DEFAULT_RENDERER_HOST_TYPE_NAME =
      "com.lynx.tasm.behavior.render.IRendererHost";
  private static final String VOID_TYPE_NAME = "void";

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
    types.add(LynxAutolinkElement.class.getCanonicalName());
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

    Set<? extends Element> elementAnnotatedClasses =
        roundEnvironment.getElementsAnnotatedWith(LynxAutolinkElement.class);
    if (elementAnnotatedClasses != null) {
      for (Element element : elementAnnotatedClasses) {
        try {
          TypeElement classType = (TypeElement) element;
          ClassName className = ClassName.get(classType);
          ClassInfo classInfo = parseAutolinkElementClass(className, classType);
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

  private boolean checkHasContextAndObjectConstructors(TypeElement classElement) {
    boolean hasParamConstructor = false;

    ClassName lynxContextCln = ClassName.get("com.lynx.tasm.behavior", "LynxContext");
    ClassName objectCln = ClassName.get("java.lang", "Object");

    for (Element enclosed : classElement.getEnclosedElements()) {
      if (enclosed.getKind() != ElementKind.CONSTRUCTOR) {
        continue;
      }

      ExecutableElement constructor = (ExecutableElement) enclosed;

      if (constructor.getParameters().size() != 2) {
        continue;
      }

      if (constructor.getParameters().get(0).asType().toString().equals(lynxContextCln.toString())
          && constructor.getParameters().get(1).asType().toString().equals(objectCln.toString())) {
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
        ClassName rendererHostCln = ClassName.bestGuess(classInfo.fragmentLayerRendererHost);
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

  private String getClassName(TypeElement type, String packageName) {
    int packageLen = packageName.length() + 1;
    return type.getQualifiedName().toString().substring(packageLen).replace('.', '$');
  }

  private ClassInfo parseBehaviorClass(ClassName className, TypeElement typeElement) {
    ClassInfo classInfo = new ClassInfo(className, typeElement);
    classInfo.addBehaviorTag(typeElement);
    classInfo.addBehaviorIsCreateAsync(typeElement);
    classInfo.addBehaviorNeedProcessDirection(typeElement);
    classInfo.addBehaviorSupportFragmentLayerRender(typeElement);
    classInfo.addBehaviorFragmentLayerRendererHost(typeElement);
    return classInfo;
  }

  private ClassInfo parseAutolinkElementClass(ClassName className, TypeElement typeElement) {
    ClassInfo classInfo = new ClassInfo(className, typeElement);
    classInfo.addAutolinkElementTag(typeElement);
    classInfo.addAutolinkElementIsCreateAsync(typeElement);
    classInfo.addAutolinkElementNeedProcessDirection(typeElement);
    classInfo.addAutolinkElementSupportFragmentLayerRender(typeElement);
    classInfo.addAutolinkElementFragmentLayerRendererHost(typeElement);
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
    public final TypeElement mElement;
    public final List<String> tagName;
    public boolean isCreateAsync;
    public String shadowNodeTag;
    public boolean needProcessDirection;
    public boolean supportFragmentLayerRender;
    public String fragmentLayerRendererHost;

    public ClassInfo(ClassName mClassName, TypeElement mElement) {
      this.mClassName = mClassName;
      this.mElement = mElement;
      this.tagName = new ArrayList<>();
      this.isCreateAsync = false;
      this.needProcessDirection = false;
      this.supportFragmentLayerRender = false;
      this.fragmentLayerRendererHost = null;
    }

    public void addBehaviorTag(Element element) {
      LynxBehavior annotation = element.getAnnotation(LynxBehavior.class);
      tagName.addAll(Arrays.asList(annotation.tagName()));
    }

    public void addAutolinkElementTag(Element element) {
      LynxAutolinkElement annotation = element.getAnnotation(LynxAutolinkElement.class);
      tagName.add(annotation.name());
    }

    public void addBehaviorIsCreateAsync(Element element) {
      LynxBehavior annotation = element.getAnnotation(LynxBehavior.class);
      isCreateAsync = annotation.isCreateAsync();
    }

    public void addAutolinkElementIsCreateAsync(Element element) {
      LynxAutolinkElement annotation = element.getAnnotation(LynxAutolinkElement.class);
      isCreateAsync = annotation.isCreateAsync();
    }

    public void addBehaviorNeedProcessDirection(Element element) {
      LynxBehavior annotation = element.getAnnotation(LynxBehavior.class);
      needProcessDirection = annotation.needProcessDirection();
    }

    public void addAutolinkElementNeedProcessDirection(Element element) {
      LynxAutolinkElement annotation = element.getAnnotation(LynxAutolinkElement.class);
      needProcessDirection = annotation.needProcessDirection();
    }

    public void addBehaviorSupportFragmentLayerRender(Element element) {
      LynxBehavior annotation = element.getAnnotation(LynxBehavior.class);
      supportFragmentLayerRender = annotation.supportFragmentLayerRender();
    }

    public void addAutolinkElementSupportFragmentLayerRender(Element element) {
      LynxAutolinkElement annotation = element.getAnnotation(LynxAutolinkElement.class);
      supportFragmentLayerRender = annotation.supportFragmentLayerRender();
    }

    public void addBehaviorFragmentLayerRendererHost(Element element) {
      LynxBehavior annotation = element.getAnnotation(LynxBehavior.class);
      TypeMirror typeMirror = null;
      try {
        annotation.fragmentLayerRendererHost();
      } catch (MirroredTypeException e) {
        typeMirror = e.getTypeMirror();
      }
      if (typeMirror != null) {
        String typeName = typeMirror.toString();
        if (isFragmentLayerRendererHostSet(typeName)) {
          fragmentLayerRendererHost = typeName;
        }
      }
    }

    public void addAutolinkElementFragmentLayerRendererHost(Element element) {
      LynxAutolinkElement annotation = element.getAnnotation(LynxAutolinkElement.class);
      TypeMirror typeMirror = null;
      try {
        annotation.fragmentLayerRendererHost();
      } catch (MirroredTypeException e) {
        typeMirror = e.getTypeMirror();
      }
      if (typeMirror != null) {
        String typeName = typeMirror.toString();
        if (isFragmentLayerRendererHostSet(typeName)) {
          fragmentLayerRendererHost = typeName;
        }
      }
    }

    public void addShadowNodeTag(Element element) {
      LynxShadowNode annotation = element.getAnnotation(LynxShadowNode.class);
      shadowNodeTag = annotation.tagName();
    }
  }

  private static boolean isFragmentLayerRendererHostSet(String typeName) {
    return !VOID_TYPE_NAME.equals(typeName) && !DEFAULT_RENDERER_HOST_TYPE_NAME.equals(typeName);
  }
}
