// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge.jsi;

import static javax.lang.model.element.Modifier.PUBLIC;

import androidx.annotation.Keep;
import com.google.auto.service.AutoService;
import com.lynx.jsbridge.jsi.LynxJSProperty;
import com.squareup.javapoet.AnnotationSpec;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.MethodSpec;
import com.squareup.javapoet.ParameterizedTypeName;
import com.squareup.javapoet.TypeSpec;
import java.io.IOException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import javax.annotation.processing.AbstractProcessor;
import javax.annotation.processing.Filer;
import javax.annotation.processing.ProcessingEnvironment;
import javax.annotation.processing.Processor;
import javax.annotation.processing.RoundEnvironment;
import javax.annotation.processing.SupportedSourceVersion;
import javax.lang.model.SourceVersion;
import javax.lang.model.element.Element;
import javax.lang.model.element.ElementKind;
import javax.lang.model.element.Modifier;
import javax.lang.model.element.TypeElement;
import javax.lang.model.type.ArrayType;
import javax.lang.model.type.DeclaredType;
import javax.lang.model.type.TypeKind;
import javax.lang.model.type.TypeMirror;
import javax.lang.model.util.Elements;
import javax.lang.model.util.Types;

/**
 * Collect LynxJSProperty and LynxJSIObject and then generate descriptors
 */
@AutoService(Processor.class)
@SupportedSourceVersion(SourceVersion.RELEASE_8)
public class LynxJSPropertyProcessor extends AbstractProcessor {
  private static final String TAG = "LynxJSIObjectProcessor";
  private static final String DESCRIPTOR_NAME_SUFFIX = "$$Descriptor";
  private static final ClassName I_LYNX_JSI_OBJECT =
      ClassName.get("com.lynx.jsbridge.jsi", "ILynxJSIObject");
  private static final ClassName LYNX_JSI_OBJECT_DESCRIPTOR =
      ClassName.get("com.lynx.jsbridge.jsi", "AbsLynxJSIObjectDescriptor");
  private static final ClassName LYNX_JS_PROPERTY_DESCRIPTOR =
      ClassName.get("com.lynx.jsbridge.jsi", "LynxJSPropertyDescriptor");

  private Filer mFiler;
  private Elements mElements;
  private Types mTypes;

  private static class JSIObjectDescriptor {
    final String simpleClassName;
    final String reflectionClassName;
    /**
     * Fields with annotation @LynxJSProperty
     */
    final Map<String, JSPropertyDescriptor> mFields = new HashMap<>();

    JSIObjectDescriptor(ClassName className) {
      reflectionClassName = className.reflectionName();
      int lastDotIndex = reflectionClassName.lastIndexOf('.');
      simpleClassName =
          lastDotIndex < 0 ? reflectionClassName : reflectionClassName.substring(lastDotIndex + 1);
    }
  }

  private static class JSPropertyDescriptor {
    final String fieldName;
    final String jniFieldDescriptor;
    JSPropertyDescriptor(String name, String descriptor) {
      fieldName = name;
      jniFieldDescriptor = descriptor;
    }
  }

  @Override
  public synchronized void init(ProcessingEnvironment processingEnv) {
    super.init(processingEnv);
    mFiler = processingEnv.getFiler();
    mElements = processingEnv.getElementUtils();
    mTypes = processingEnv.getTypeUtils();
  }

  @Override
  public boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv) {
    Set<? extends Element> jsPropertyElements =
        roundEnv.getElementsAnnotatedWith(LynxJSProperty.class);
    if (jsPropertyElements.isEmpty()) {
      return true;
    }

    final Map<ClassName, JSIObjectDescriptor> jsiObjectDescriptorMap =
        collectJSIObjectClassesInfo(jsPropertyElements);

    return generateJSIObjectDescriptorClasses(jsiObjectDescriptorMap);
  }

  /**
   * collect classesInfo with element annotated with LynxJSProperty
   */
  private Map<ClassName, JSIObjectDescriptor> collectJSIObjectClassesInfo(
      Set<? extends Element> elements) {
    System.out.println(TAG + ", start to collect class info, element size = " + elements.size());
    Map<ClassName, JSIObjectDescriptor> descriptorMap = new HashMap<>();
    for (Element element : elements) {
      TypeElement classElement = (TypeElement) element.getEnclosingElement();
      ClassName className = ClassName.get(classElement);
      System.out.println(TAG + ", find an element, name: " + element.getSimpleName()
          + "in class: " + className.simpleName());
      checkJSPropertyValidate(element);

      // collect class info
      if (!descriptorMap.containsKey(className)) {
        JSIObjectDescriptor descriptor = new JSIObjectDescriptor(className);
        // collect elements in enclosing class
        collectFieldsFromJSIObject(element.getEnclosingElement(), descriptor);
        descriptorMap.put(className, descriptor);
      }
    }
    return descriptorMap;
  }

  /**
   * check if the JSProperty has valid type, currently only String is supported
   */
  private void checkJSPropertyValidate(Element element) {
    if (element.getKind() != ElementKind.FIELD) {
      throw new IllegalArgumentException("@LynxJSProperty must be a field");
    }
    checkJSPropertyValidate(element.asType());
  }

  // only support primitive types, string, ILynxJSIObject, array
  private void checkJSPropertyValidate(TypeMirror fieldType) {
    if (isPrimitive(fieldType)) {
      return;
    }

    if (isAssignable(fieldType, String.class)
        || isAssignable(fieldType, I_LYNX_JSI_OBJECT.reflectionName())) {
      return;
    }

    if (fieldType.getKind() == TypeKind.ARRAY) {
      TypeMirror arrayType = ((ArrayType) fieldType).getComponentType();
      checkJSPropertyValidate(arrayType);
      return;
    }

    throw new IllegalArgumentException(
        "InValidate @LynxJSProperty, Only support int, long, float, double, bool, String, ILynxJSIObject, array type, current type is "
        + fieldType);
  }

  // only support int, long, float, double, bool
  private boolean isPrimitive(TypeMirror fieldType) {
    TypeKind kind = fieldType.getKind();
    return kind == TypeKind.INT || kind == TypeKind.LONG || kind == TypeKind.FLOAT
        || kind == TypeKind.DOUBLE || kind == TypeKind.BOOLEAN;
  }

  private boolean isAssignable(TypeMirror fieldType, Class clazz) {
    TypeElement typeElement = mElements.getTypeElement(clazz.getCanonicalName());
    TypeMirror erasureTargetType = mTypes.erasure(typeElement.asType());
    TypeMirror erasureFieldType = mTypes.erasure(fieldType);
    return mTypes.isAssignable(erasureFieldType, erasureTargetType);
  }

  private boolean isAssignable(TypeMirror fieldType, String className) {
    TypeMirror targetType = mElements.getTypeElement(className).asType();
    return mTypes.isAssignable(fieldType, targetType);
  }

  private void collectFieldsFromJSIObject(Element jsiObject, JSIObjectDescriptor descriptor) {
    // collect current class field
    // TODO(zhoupeng.z): collect parents class field
    System.out.println(TAG + ", collect fields for class: " + descriptor.simpleClassName);
    if (!isAssignable(jsiObject.asType(), I_LYNX_JSI_OBJECT.reflectionName())) {
      throw new IllegalArgumentException("Enclosing class must be a ILynxJSIObject");
    }
    for (Element enclosedElement : jsiObject.getEnclosedElements()) {
      // collect all enclosed field with @LynxJSProperty
      if (enclosedElement.getKind() != ElementKind.FIELD
          || enclosedElement.getAnnotation(LynxJSProperty.class) == null) {
        continue;
      }

      String fieldName = enclosedElement.getSimpleName().toString();
      String jniFieldDescriptor = getJNIFieldDescriptor(enclosedElement.asType());
      descriptor.mFields.put(fieldName, new JSPropertyDescriptor(fieldName, jniFieldDescriptor));
      System.out.println(TAG + ", collect a field, name: " + fieldName);
    }
  }

  private String getJNIFieldDescriptor(TypeMirror type) {
    switch (type.getKind()) {
      case BOOLEAN:
        return "Z";
      case INT:
        return "I";
      case LONG:
        return "J";
      case FLOAT:
        return "F";
      case DOUBLE:
        return "D";
      case DECLARED:
        DeclaredType declaredType = (DeclaredType) type;
        TypeElement typeElement = (TypeElement) declaredType.asElement();
        String binaryName = mElements.getBinaryName(typeElement).toString();
        return "L" + binaryName.replace('.', '/') + ";";
      case ARRAY:
        ArrayType arrayType = (ArrayType) type;
        TypeMirror commonType = arrayType.getComponentType();
        String componentSig = getJNIFieldDescriptor(commonType);
        return "[" + componentSig;
      default:
        throw new IllegalArgumentException(
            TAG + ", getJNIFieldDescriptor failed, current type: " + type);
    }
  }

  private boolean generateJSIObjectDescriptorClasses(
      Map<ClassName, JSIObjectDescriptor> descriptorMap) {
    System.out.println(
        TAG + ", start to generate classes' descriptors, size: " + descriptorMap.size());
    for (Map.Entry<ClassName, JSIObjectDescriptor> entry : descriptorMap.entrySet()) {
      ClassName className = entry.getKey();
      JSIObjectDescriptor descriptor = entry.getValue();
      System.out.println(
          TAG + ", generate LynxJSIObjectDescriptor for class: " + descriptor.simpleClassName);

      // generate class name: OriginLynxJSIObject$$Descriptor
      TypeSpec.Builder classBuilder =
          TypeSpec.classBuilder(descriptor.simpleClassName + DESCRIPTOR_NAME_SUFFIX)
              .addModifiers(PUBLIC);

      // generate method: String getClassName()
      generateGetClassNameMethod(descriptor, classBuilder);

      // generate method: getFieldInfoArray()
      generateGetFieldInfoMethod(descriptor, classBuilder);

      // generate annotations: @Keep and @AutoService
      generateAnnotations(classBuilder);

      // add superclass for LynxJSIObjectDescriptor
      classBuilder.superclass(LYNX_JSI_OBJECT_DESCRIPTOR);

      // write to file
      JavaFile javaFile = JavaFile.builder(className.packageName(), classBuilder.build())
                              .addFileComment("Generated by " + getClass().getName())
                              .build();
      try {
        javaFile.writeTo(mFiler);
      } catch (IOException e) {
        System.out.println(TAG + ", fail to write javaFile: " + e.getMessage());
        return false;
      }
    }
    return true;
  }

  /**
   * build Method as:
   * @Override
   * public String getClassName() {
   *     return "com.xxx.xxx.YourClassName$YourClassName";
   * }
   */
  private void generateGetClassNameMethod(
      JSIObjectDescriptor descriptor, TypeSpec.Builder classBuilder) {
    MethodSpec getClassNameMethod = MethodSpec.methodBuilder("getClassName")
                                        .addAnnotation(Override.class)
                                        .addModifiers(Modifier.PUBLIC)
                                        .returns(String.class)
                                        .addStatement("return $S", descriptor.reflectionClassName)
                                        .build();
    classBuilder.addMethod(getClassNameMethod);
  }

  /**
   * build Method for getFieldInfoArray
   * private LynxJSIObjectDescriptorInfo[] mFieldInfos = null;
   * @Override
   * protect ConcurrentHashMap<String, LynxJSPropertyDescriptor> createFieldInfos() {
   *     ConcurrentHashMap<String, LynxJSPropertyDescriptor> fieldInfos = new HashMap();
   *     fieldInfos.add("mStr", new LynxJSPropertyDescriptor("mStr", "Ljava/lang/String;"));
   *     fieldInfos.add("xxx",  new LynxJSPropertyDescriptor("xxx", "xxx"));
   *     return fieldInfos;
   *   }
   */
  private void generateGetFieldInfoMethod(
      JSIObjectDescriptor descriptor, TypeSpec.Builder classBuilder) {
    Class mapClass = ConcurrentHashMap.class;
    MethodSpec.Builder createFieldInfosMethodBuilder =
        MethodSpec.methodBuilder("createFieldInfos")
            .addAnnotation(Override.class)
            .addModifiers(Modifier.PROTECTED)
            .returns(ParameterizedTypeName.get(
                ClassName.get(mapClass), ClassName.get(String.class), LYNX_JS_PROPERTY_DESCRIPTOR))
            .addStatement("$T<String, $T> fieldInfos = new $T()", mapClass,
                LYNX_JS_PROPERTY_DESCRIPTOR, mapClass);

    for (JSPropertyDescriptor jsPropertyDescriptor : descriptor.mFields.values()) {
      String fieldName = jsPropertyDescriptor.fieldName;
      String fieldType = jsPropertyDescriptor.jniFieldDescriptor;
      createFieldInfosMethodBuilder.addStatement("fieldInfos.put($S, new $T($S, $S))", fieldName,
          LYNX_JS_PROPERTY_DESCRIPTOR, fieldName, fieldType);
      System.out.println(TAG + ", generate LynxJSPropertyDescriptor for field: " + fieldName);
    }

    createFieldInfosMethodBuilder.addStatement("return fieldInfos");

    classBuilder.addMethod(createFieldInfosMethodBuilder.build());
  }

  /**
   * build Annotations as:
   * @Keep
   */
  private void generateAnnotations(TypeSpec.Builder classBuilder) {
    classBuilder.addAnnotation(AnnotationSpec.builder(Keep.class).build());
  }

  @Override
  public Set<String> getSupportedAnnotationTypes() {
    HashSet<String> annotationSet = new HashSet<>();
    annotationSet.add(LynxJSProperty.class.getCanonicalName());
    return annotationSet;
  }
}
