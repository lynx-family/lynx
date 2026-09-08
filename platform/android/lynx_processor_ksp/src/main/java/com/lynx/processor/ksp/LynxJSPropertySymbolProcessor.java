// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.processor.ksp;

import static javax.lang.model.element.Modifier.PUBLIC;

import com.google.devtools.ksp.processing.CodeGenerator;
import com.google.devtools.ksp.processing.Dependencies;
import com.google.devtools.ksp.processing.Resolver;
import com.google.devtools.ksp.processing.SymbolProcessor;
import com.google.devtools.ksp.processing.SymbolProcessorEnvironment;
import com.google.devtools.ksp.processing.SymbolProcessorProvider;
import com.google.devtools.ksp.symbol.KSAnnotated;
import com.google.devtools.ksp.symbol.KSAnnotation;
import com.google.devtools.ksp.symbol.KSClassDeclaration;
import com.google.devtools.ksp.symbol.KSDeclaration;
import com.google.devtools.ksp.symbol.KSFile;
import com.google.devtools.ksp.symbol.KSPropertyDeclaration;
import com.google.devtools.ksp.symbol.KSType;
import com.google.devtools.ksp.symbol.KSTypeArgument;
import com.google.devtools.ksp.symbol.KSTypeReference;
import com.google.devtools.ksp.symbol.Nullability;
import com.google.devtools.ksp.symbol.Variance;
import com.squareup.javapoet.AnnotationSpec;
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
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import javax.lang.model.element.Modifier;

/**
 * KSP port of {@code com.lynx.jsbridge.jsi.LynxJSPropertyProcessor}.
 *
 * <p>Collects fields annotated with {@code @LynxJSProperty} inside {@code ILynxJSIObject}
 * implementations and generates the {@code <Class>$$Descriptor} classes, producing the same Java
 * sources as the javac implementation.
 */
public class LynxJSPropertySymbolProcessor implements SymbolProcessor {
  private static final String TAG = "LynxJSIObjectProcessor";
  private static final String DESCRIPTOR_NAME_SUFFIX = "$$Descriptor";
  private static final String LYNX_JS_PROPERTY_ANNOTATION = "com.lynx.jsbridge.jsi.LynxJSProperty";
  private static final String SERIALIZED_NAME_ANNOTATION =
      "com.google.gson.annotations.SerializedName";
  private static final ClassName I_LYNX_JSI_OBJECT =
      ClassName.get("com.lynx.jsbridge.jsi", "ILynxJSIObject");
  private static final ClassName LYNX_JSI_OBJECT_DESCRIPTOR =
      ClassName.get("com.lynx.jsbridge.jsi", "AbsLynxJSIObjectDescriptor");
  private static final ClassName LYNX_JS_PROPERTY_DESCRIPTOR =
      ClassName.get("com.lynx.jsbridge.jsi", "LynxJSPropertyDescriptor");
  private static final ClassName KEEP = ClassName.get("androidx.annotation", "Keep");

  /** javac sees primitive TypeKinds; KSP surfaces them as non-null kotlin types. */
  private static final Map<String, String> PRIMITIVE_DESCRIPTORS;
  /** Java primitive arrays surface as the dedicated kotlin array types. */
  private static final Map<String, String> PRIMITIVE_ARRAY_DESCRIPTORS;
  /** Kotlin collection names whose JVM binary name differs from the declaration name. */
  private static final Map<String, String> BINARY_NAME_OVERRIDES;
  private static final Set<String> PRIMITIVE_OR_WRAPPER_NAMES;
  private static final Set<String> STRING_NAMES;
  private static final Set<String> LIST_NAMES;
  private static final Set<String> I_LYNX_JSI_OBJECT_NAMES;

  static {
    PRIMITIVE_DESCRIPTORS = new HashMap<>();
    PRIMITIVE_DESCRIPTORS.put("kotlin.Boolean", "Z");
    PRIMITIVE_DESCRIPTORS.put("kotlin.Int", "I");
    PRIMITIVE_DESCRIPTORS.put("kotlin.Long", "J");
    PRIMITIVE_DESCRIPTORS.put("kotlin.Float", "F");
    PRIMITIVE_DESCRIPTORS.put("kotlin.Double", "D");

    PRIMITIVE_ARRAY_DESCRIPTORS = new HashMap<>();
    PRIMITIVE_ARRAY_DESCRIPTORS.put("kotlin.BooleanArray", "[Z");
    PRIMITIVE_ARRAY_DESCRIPTORS.put("kotlin.IntArray", "[I");
    PRIMITIVE_ARRAY_DESCRIPTORS.put("kotlin.LongArray", "[J");
    PRIMITIVE_ARRAY_DESCRIPTORS.put("kotlin.FloatArray", "[F");
    PRIMITIVE_ARRAY_DESCRIPTORS.put("kotlin.DoubleArray", "[D");

    BINARY_NAME_OVERRIDES = new HashMap<>();
    BINARY_NAME_OVERRIDES.put("kotlin.collections.List", "java.util.List");
    BINARY_NAME_OVERRIDES.put("kotlin.collections.MutableList", "java.util.List");

    PRIMITIVE_OR_WRAPPER_NAMES = new HashSet<>();
    PRIMITIVE_OR_WRAPPER_NAMES.add("kotlin.Boolean");
    PRIMITIVE_OR_WRAPPER_NAMES.add("kotlin.Int");
    PRIMITIVE_OR_WRAPPER_NAMES.add("kotlin.Long");
    PRIMITIVE_OR_WRAPPER_NAMES.add("kotlin.Float");
    PRIMITIVE_OR_WRAPPER_NAMES.add("kotlin.Double");
    PRIMITIVE_OR_WRAPPER_NAMES.add("java.lang.Boolean");
    PRIMITIVE_OR_WRAPPER_NAMES.add("java.lang.Integer");
    PRIMITIVE_OR_WRAPPER_NAMES.add("java.lang.Long");
    PRIMITIVE_OR_WRAPPER_NAMES.add("java.lang.Float");
    PRIMITIVE_OR_WRAPPER_NAMES.add("java.lang.Double");

    STRING_NAMES = new HashSet<>();
    STRING_NAMES.add("kotlin.String");
    STRING_NAMES.add("java.lang.String");

    LIST_NAMES = new HashSet<>();
    LIST_NAMES.add("java.util.List");
    LIST_NAMES.add("kotlin.collections.List");
    LIST_NAMES.add("kotlin.collections.MutableList");

    I_LYNX_JSI_OBJECT_NAMES = new HashSet<>();
    I_LYNX_JSI_OBJECT_NAMES.add(I_LYNX_JSI_OBJECT.reflectionName());
  }

  private final CodeGenerator mCodeGenerator;

  private static class JSIObjectDescriptor {
    final String simpleClassName;
    final String reflectionClassName;
    /**
     * Fields with annotation @LynxJSProperty
     */
    final Map<String, JSPropertyDescriptor> mFields = new HashMap<>();
    /**
     * Source files the field walk touched; the descriptor also collects superclass fields living
     * in other files, so all of them are reported as origins for incremental processing.
     */
    final Set<KSFile> mSourceFiles = new LinkedHashSet<>();

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

  public LynxJSPropertySymbolProcessor(SymbolProcessorEnvironment environment) {
    mCodeGenerator = environment.getCodeGenerator();
  }

  @Override
  public List<KSAnnotated> process(Resolver resolver) {
    List<KSAnnotated> jsPropertyElements = new ArrayList<>();
    Iterator<KSAnnotated> symbols =
        resolver.getSymbolsWithAnnotation(LYNX_JS_PROPERTY_ANNOTATION, false).iterator();
    while (symbols.hasNext()) {
      jsPropertyElements.add(symbols.next());
    }
    if (jsPropertyElements.isEmpty()) {
      return Collections.emptyList();
    }

    final Map<ClassName, JSIObjectDescriptor> jsiObjectDescriptorMap =
        collectJSIObjectClassesInfo(jsPropertyElements);

    generateJSIObjectDescriptorClasses(jsiObjectDescriptorMap);

    return Collections.emptyList();
  }

  @Override
  public void finish() {}

  @Override
  public void onError() {}

  /**
   * collect classesInfo with element annotated with LynxJSProperty
   */
  private Map<ClassName, JSIObjectDescriptor> collectJSIObjectClassesInfo(
      List<KSAnnotated> elements) {
    System.out.println(TAG + ", start to collect class info, element size = " + elements.size());
    Map<ClassName, JSIObjectDescriptor> descriptorMap = new HashMap<>();
    for (KSAnnotated element : elements) {
      KSDeclaration declaration = (KSDeclaration) element;
      KSClassDeclaration classElement = (KSClassDeclaration) declaration.getParentDeclaration();
      ClassName className = KspUtils.toClassName(classElement);
      System.out.println(TAG + ", find an element, name: " + declaration.getSimpleName().asString()
          + "in class: " + className.simpleName());
      checkJSPropertyValidate(declaration, className);

      // collect class info
      if (!descriptorMap.containsKey(className)) {
        JSIObjectDescriptor descriptor = new JSIObjectDescriptor(className);
        // collect elements in enclosing class
        collectFieldsFromJSIObject(classElement, descriptor);
        descriptorMap.put(className, descriptor);
      }
    }
    return descriptorMap;
  }

  /**
   * check if the JSProperty has valid type, currently only String is supported
   */
  private void checkJSPropertyValidate(KSDeclaration element, ClassName className) {
    if (!(element instanceof KSPropertyDeclaration)) {
      throwException("@LynxJSProperty must be a field", element.toString(),
          element.getSimpleName().asString(), className.simpleName());
    }
    checkJSPropertyValidate(
        ((KSPropertyDeclaration) element).getType().resolve(), element, className);
  }

  // only support primitive types, string, ILynxJSIObject, array
  private void checkJSPropertyValidate(
      KSType fieldType, KSDeclaration element, ClassName className) {
    if (isPrimitiveOrWrapper(fieldType)) {
      return;
    }

    String qualifiedName = KspUtils.qualifiedNameOf(fieldType.getDeclaration());
    if (STRING_NAMES.contains(qualifiedName)
        || isAssignableTo(fieldType.getDeclaration(), I_LYNX_JSI_OBJECT_NAMES)) {
      return;
    }

    // javac sees ArrayType; KSP surfaces primitive arrays and kotlin.Array<T> instead
    if (PRIMITIVE_ARRAY_DESCRIPTORS.containsKey(qualifiedName)) {
      return;
    }
    if ("kotlin.Array".equals(qualifiedName)) {
      KSTypeReference componentType =
          fieldType.getArguments().isEmpty() ? null : fieldType.getArguments().get(0).getType();
      if (componentType != null) {
        checkJSPropertyValidate(componentType.resolve(), element, className);
        return;
      }
    }

    if (isAssignableTo(fieldType.getDeclaration(), LIST_NAMES)) {
      List<KSTypeArgument> typeArguments = fieldType.getArguments();
      if (typeArguments.isEmpty()) {
        return;
      }
      KSTypeArgument typeArgument = typeArguments.get(0);
      if (typeArgument.getVariance() == Variance.COVARIANT) {
        // `? extends X` wildcards arrive as covariant projections
        KSTypeReference upperBound = typeArgument.getType();
        if (upperBound != null
            && isAssignableTo(upperBound.resolve().getDeclaration(), I_LYNX_JSI_OBJECT_NAMES)) {
          return;
        }
      } else if (typeArgument.getVariance() == Variance.INVARIANT) {
        KSTypeReference argumentType = typeArgument.getType();
        if (argumentType != null) {
          checkJSPropertyValidate(argumentType.resolve(), element, className);
          return;
        }
      }
    }

    throwException(
        "InValidate @LynxJSProperty type, supported type: int, Integer, long, Long, float, Float, "
            + "double, Double, boolean, Boolean, String, array, List, ILynxJSIObject",
        fieldType.toString(), element.getSimpleName().asString(), className.simpleName());
  }

  // support int, long, float, double, bool, Integer, Long, Float, Double, Boolean
  private boolean isPrimitiveOrWrapper(KSType fieldType) {
    return PRIMITIVE_OR_WRAPPER_NAMES.contains(
        KspUtils.qualifiedNameOf(fieldType.getDeclaration()));
  }

  /**
   * Erasure-level assignability: walks the supertype chain looking for one of the target
   * qualified names, matching the javac processor's {@code Types#isAssignable} on erasures.
   */
  private static boolean isAssignableTo(KSDeclaration declaration, Set<String> targetNames) {
    if (!(declaration instanceof KSClassDeclaration)) {
      return false;
    }
    if (targetNames.contains(KspUtils.qualifiedNameOf(declaration))) {
      return true;
    }
    Iterator<KSTypeReference> superTypes =
        ((KSClassDeclaration) declaration).getSuperTypes().iterator();
    while (superTypes.hasNext()) {
      if (isAssignableTo(superTypes.next().resolve().getDeclaration(), targetNames)) {
        return true;
      }
    }
    return false;
  }

  private void collectFieldsFromJSIObject(
      KSClassDeclaration jsiObject, JSIObjectDescriptor descriptor) {
    // collect current class field
    System.out.println(TAG + ", collect fields for class: " + descriptor.simpleClassName);
    if (!isAssignableTo(jsiObject, I_LYNX_JSI_OBJECT_NAMES)) {
      throwException("Enclosing class must be a ILynxJSIObject",
          jsiObject.getClassKind().toString(), jsiObject.getSimpleName().asString(),
          descriptor.simpleClassName);
    }
    // Recursively get the element of the parent class
    KSClassDeclaration curElement = jsiObject;
    while (curElement != null && isAssignableTo(curElement, I_LYNX_JSI_OBJECT_NAMES)) {
      if (curElement.getContainingFile() != null) {
        descriptor.mSourceFiles.add(curElement.getContainingFile());
      }
      Iterator<KSDeclaration> enclosedElements = curElement.getDeclarations().iterator();
      while (enclosedElements.hasNext()) {
        KSDeclaration enclosedElement = enclosedElements.next();
        // collect all enclosed field with @LynxJSProperty
        if (!(enclosedElement instanceof KSPropertyDeclaration)
            || KspUtils.findAnnotation(enclosedElement, LYNX_JS_PROPERTY_ANNOTATION) == null) {
          continue;
        }

        String fieldName = enclosedElement.getSimpleName().asString();
        String jniFieldDescriptor =
            getJNIFieldDescriptor(((KSPropertyDeclaration) enclosedElement).getType().resolve(),
                fieldName, descriptor.simpleClassName);

        String serializedName = getSerializedName(enclosedElement);
        String fieldNameForScript = serializedName != null ? serializedName : fieldName;
        descriptor.mFields.put(
            fieldNameForScript, new JSPropertyDescriptor(fieldName, jniFieldDescriptor));
        System.out.println(
            TAG + ", collect a field, name: " + fieldName + ", serialized name: " + serializedName);
      }
      // get parent class element
      curElement = KspUtils.superclassOf(curElement);
    }
  }

  private String getSerializedName(KSDeclaration element) {
    KSAnnotation serializableAnno = KspUtils.findAnnotation(element, SERIALIZED_NAME_ANNOTATION);
    if (serializableAnno != null) {
      Object value = KspUtils.argument(serializableAnno, "value");
      if (value != null) {
        return (String) value;
      }
    }
    return null;
  }

  private String getJNIFieldDescriptor(KSType type, String fieldName, String className) {
    String qualifiedName = KspUtils.qualifiedNameOf(type.getDeclaration());

    // Java primitives arrive as non-null kotlin types; nullable ones stay boxed
    if (type.getNullability() == Nullability.NOT_NULL) {
      String primitive = PRIMITIVE_DESCRIPTORS.get(qualifiedName);
      if (primitive != null) {
        return primitive;
      }
    }

    String primitiveArray = PRIMITIVE_ARRAY_DESCRIPTORS.get(qualifiedName);
    if (primitiveArray != null) {
      return primitiveArray;
    }

    if ("kotlin.Array".equals(qualifiedName)) {
      KSTypeReference componentType =
          type.getArguments().isEmpty() ? null : type.getArguments().get(0).getType();
      if (componentType != null) {
        String componentSig = getJNIFieldDescriptor(componentType.resolve(), fieldName, className);
        return "[" + componentSig;
      }
      throwException(
          "getJNIFieldDescriptor failed, current type: ", type.toString(), fieldName, className);
      return null;
    }

    // javac uses Elements#getBinaryName; kotlin collection interfaces map back to java.util
    String overriddenBinaryName = BINARY_NAME_OVERRIDES.get(qualifiedName);
    if (overriddenBinaryName != null) {
      return "L" + overriddenBinaryName.replace('.', '/') + ";";
    }

    TypeName typeName = KspUtils.toTypeName(type);
    if (typeName instanceof ClassName) {
      return "L" + ((ClassName) typeName).reflectionName().replace('.', '/') + ";";
    }

    throwException(
        "getJNIFieldDescriptor failed, current type: ", type.toString(), fieldName, className);
    return null;
  }

  private void throwException(String msg, String typeName, String elementName, String className) {
    throw new IllegalArgumentException(TAG + ", error: " + msg + ", type: " + typeName
        + ", element name: " + elementName + ", in class: " + className);
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

      // write to file; the javac processor stamps its own class name into the comment
      JavaFile javaFile =
          JavaFile.builder(className.packageName(), classBuilder.build())
              .addFileComment("Generated by com.lynx.jsbridge.jsi.LynxJSPropertyProcessor")
              .build();
      try {
        write(descriptor, className.packageName(),
            descriptor.simpleClassName + DESCRIPTOR_NAME_SUFFIX, javaFile);
      } catch (IOException e) {
        System.out.println(TAG + ", fail to write javaFile: " + e.getMessage());
        return false;
      }
    }
    return true;
  }

  /**
   * Same as {@link KspUtils#write} but reporting every source file the field walk touched, since
   * descriptors also include superclass fields living in other files.
   */
  private void write(JSIObjectDescriptor descriptor, String packageName, String fileName,
      JavaFile javaFile) throws IOException {
    KSFile[] sourceFiles = descriptor.mSourceFiles.toArray(new KSFile[0]);
    OutputStream stream = mCodeGenerator.createNewFile(
        new Dependencies(false, sourceFiles), packageName, fileName, "java");
    try (Writer writer = new OutputStreamWriter(stream, StandardCharsets.UTF_8)) {
      javaFile.writeTo(writer);
    }
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

    for (Map.Entry<String, JSPropertyDescriptor> entry : descriptor.mFields.entrySet()) {
      JSPropertyDescriptor jsPropertyDescriptor = entry.getValue();
      String fieldName = jsPropertyDescriptor.fieldName;
      String fieldType = jsPropertyDescriptor.jniFieldDescriptor;
      createFieldInfosMethodBuilder.addStatement("fieldInfos.put($S, new $T($S, $S))",
          entry.getKey(), LYNX_JS_PROPERTY_DESCRIPTOR, fieldName, fieldType);
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
    classBuilder.addAnnotation(AnnotationSpec.builder(KEEP).build());
  }

  /** Entry point registered in META-INF/services. */
  public static class Provider implements SymbolProcessorProvider {
    @Override
    public SymbolProcessor create(SymbolProcessorEnvironment environment) {
      return new LynxJSPropertySymbolProcessor(environment);
    }
  }
}
