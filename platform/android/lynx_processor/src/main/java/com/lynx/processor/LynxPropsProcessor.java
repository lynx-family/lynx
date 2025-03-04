// Copyright 2004-present Facebook. All Rights Reserved.

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.processor;

import static javax.lang.model.element.Modifier.PRIVATE;
import static javax.lang.model.element.Modifier.PUBLIC;
import static javax.tools.Diagnostic.Kind.ERROR;
import static javax.tools.Diagnostic.Kind.WARNING;

import com.google.auto.service.AutoService;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxPropGroup;
import com.lynx.tasm.behavior.LynxPropsHolder;
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
import java.util.Comparator;
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
import javax.lang.model.element.ElementKind;
import javax.lang.model.element.ExecutableElement;
import javax.lang.model.element.TypeElement;
import javax.lang.model.element.VariableElement;
import javax.lang.model.type.TypeMirror;
import javax.lang.model.util.Elements;
import javax.lang.model.util.Types;

@AutoService(Processor.class)
@SupportedSourceVersion(SourceVersion.RELEASE_8)
public class LynxPropsProcessor extends AbstractProcessor {
  private static final Map<TypeName, String> DEFAULT_TYPES;
  private static final Set<TypeName> BOXED_PRIMITIVES;

  private static final TypeName PROPS_TYPE =
      ClassName.get("com.lynx.tasm.behavior", "StylesDiffMap");
  private static final TypeName STRING_TYPE = TypeName.get(String.class);
  private static final TypeName READABLE_MAP_TYPE =
      ClassName.get("com.lynx.react.bridge", "ReadableMap");
  private static final TypeName READABLE_ARRAY_TYPE =
      ClassName.get("com.lynx.react.bridge", "ReadableArray");
  private static final TypeName DYNAMIC_TYPE = ClassName.get("com.lynx.react.bridge", "Dynamic");

  private static final ClassName LYNX_UI_TYPE =
      ClassName.get("com.lynx.tasm.behavior.ui", "LynxBaseUI");
  private static final ClassName SHADOW_NODE_IMPL_TYPE =
      ClassName.get("com.lynx.tasm.behavior.shadow", "ShadowNode");

  private static final ClassName LYNX_UI_SETTER_TYPE =
      ClassName.get("com.lynx.tasm.behavior.utils", "LynxUISetter");
  private static final ClassName SHADOW_NODE_SETTER_TYPE =
      ClassName.get("com.lynx.tasm.behavior.utils", "ShadowNodeSetter");

  private final Map<ClassName, ClassInfo> mClasses;

  private Filer mFiler;
  private Messager mMessager;
  private Elements mElements;
  private Types mTypes;

  static {
    DEFAULT_TYPES = new HashMap<>();

    // Primitives
    DEFAULT_TYPES.put(TypeName.BOOLEAN, "boolean");
    DEFAULT_TYPES.put(TypeName.DOUBLE, "number");
    DEFAULT_TYPES.put(TypeName.FLOAT, "number");
    DEFAULT_TYPES.put(TypeName.INT, "number");

    // Boxed primitives
    DEFAULT_TYPES.put(TypeName.BOOLEAN.box(), "boolean");
    DEFAULT_TYPES.put(TypeName.INT.box(), "number");

    // Class types
    DEFAULT_TYPES.put(STRING_TYPE, "String");
    DEFAULT_TYPES.put(READABLE_ARRAY_TYPE, "Array");
    DEFAULT_TYPES.put(READABLE_MAP_TYPE, "Map");
    DEFAULT_TYPES.put(DYNAMIC_TYPE, "Dynamic");

    BOXED_PRIMITIVES = new HashSet<>();
    BOXED_PRIMITIVES.add(TypeName.BOOLEAN.box());
    BOXED_PRIMITIVES.add(TypeName.FLOAT.box());
    BOXED_PRIMITIVES.add(TypeName.INT.box());
  }

  public LynxPropsProcessor() {
    mClasses = new HashMap<>();
  }

  @Override
  public synchronized void init(ProcessingEnvironment processingEnv) {
    super.init(processingEnv);

    mFiler = processingEnv.getFiler();
    mMessager = processingEnv.getMessager();
    mElements = processingEnv.getElementUtils();
    mTypes = processingEnv.getTypeUtils();
  }

  @Override
  public boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv) {
    // Clear properties from previous rounds
    mClasses.clear();

    Set<? extends Element> elements = roundEnv.getElementsAnnotatedWith(LynxPropsHolder.class);
    System.out.print("LynxProcessor: process start size = " + elements.size() + "\n");
    for (Element element : elements) {
      try {
        TypeElement classType = (TypeElement) element;
        System.out.print("LynxProcessor: process classType = " + classType.toString() + "\n");
        ClassName className = ClassName.get(classType);
        mClasses.put(className, parseClass(className, classType));
      } catch (Exception e) {
        error(element, e.getMessage());
      }
    }

    for (ClassInfo classInfo : mClasses.values()) {
      try {
        if (!shouldIgnoreClass(classInfo)) {
          // Sort by name
          Collections.sort(classInfo.mProperties, new Comparator<PropertyInfo>() {
            @Override
            public int compare(PropertyInfo a, PropertyInfo b) {
              return a.mProperty.name().compareTo(b.mProperty.name());
            }
          });
          generateCode(classInfo, classInfo.mProperties);
        } else if (shouldWarnClass(classInfo)) {
          warning(classInfo.mElement, "Class was skipped. Classes need to be non-private.");
        }
      } catch (IOException e) {
        error(e.getMessage());
      } catch (ReactPropertyException e) {
        error(e.element, e.getMessage());
      } catch (Exception e) {
        error(classInfo.mElement, e.getMessage());
      }
    }

    return true;
  }

  private static boolean isShadowNodeType(TypeName typeName) {
    return typeName.equals(SHADOW_NODE_IMPL_TYPE);
  }

  private static boolean isLynxUIType(TypeName typeName) {
    return typeName.equals(LYNX_UI_TYPE);
  }

  private ClassInfo parseClass(ClassName className, TypeElement typeElement) {
    TypeName targetType = getTargetType(typeElement.asType());
    SettableType settableType =
        isShadowNodeType(targetType) ? SettableType.SHADOW_NODE : SettableType.LYNX_UI;

    ClassInfo classInfo = new ClassInfo(className, typeElement, settableType);
    findProperties(classInfo, typeElement);

    return classInfo;
  }

  private void findProperties(ClassInfo classInfo, TypeElement typeElement) {
    PropertyInfo.Builder propertyBuilder = new PropertyInfo.Builder(mTypes, mElements, classInfo);

    if (typeElement != null) {
      for (Element element : typeElement.getEnclosedElements()) {
        LynxProp prop = element.getAnnotation(LynxProp.class);
        LynxPropGroup propGroup = element.getAnnotation(LynxPropGroup.class);

        try {
          if (prop != null || propGroup != null) {
            checkElement(element);
          }

          if (prop != null) {
            classInfo.addProperty(propertyBuilder.build(element, new RegularProperty(prop)));
          } else if (propGroup != null) {
            for (int i = 0, size = propGroup.names().length; i < size; i++) {
              classInfo.addProperty(
                  propertyBuilder.build(element, new GroupProperty(propGroup, i)));
            }
          }
        } catch (ReactPropertyException e) {
          error(e.element, e.getMessage());
        }
      }
    }
  }

  private TypeName getTargetType(TypeMirror mirror) {
    TypeName typeName = TypeName.get(mirror);
    if (isLynxUIType(typeName)) {
      return LYNX_UI_TYPE;
    } else if (isShadowNodeType(typeName)) {
      return SHADOW_NODE_IMPL_TYPE;
    } else if (typeName.equals(TypeName.OBJECT)) {
      throw new IllegalArgumentException("Could not find target type " + typeName);
    }

    List<? extends TypeMirror> types = mTypes.directSupertypes(mirror);
    return getTargetType(types.get(0));
  }

  private void generateCode(ClassInfo classInfo, List<PropertyInfo> properties)
      throws IOException, ReactPropertyException {
    TypeName superType = getSuperType(classInfo);
    ClassName className = classInfo.mClassName;

    ClassName pClassName = null;
    if (!isLynxUIType(TypeName.get(classInfo.mElement.asType()))
        && !isShadowNodeType(TypeName.get(classInfo.mElement.asType()))) {
      String packageName = classInfo.mParentClassName.packageName();
      String simpleName = classInfo.mParentClassName.simpleName() + "$$PropsSetter";
      pClassName = ClassName.get(packageName, simpleName);
    }

    String holderClassName =
        getClassName((TypeElement) classInfo.mElement, className.packageName()) + "$$PropsSetter";
    TypeSpec.Builder builder = TypeSpec.classBuilder(holderClassName)
                                   .addModifiers(PUBLIC)
                                   .addMethod(generateSetPropertySpec(classInfo, properties));
    if (null != pClassName) {
      builder.superclass(pClassName);
    } else {
      builder.addSuperinterface(superType);
    }

    TypeSpec holderClass = builder.build();

    JavaFile javaFile = JavaFile.builder(className.packageName(), holderClass)
                            .addFileComment("Generated by " + getClass().getName())
                            .build();

    javaFile.writeTo(mFiler);
  }

  private String getClassName(TypeElement type, String packageName) {
    int packageLen = packageName.length() + 1;
    return type.getQualifiedName().toString().substring(packageLen).replace('.', '$');
  }

  private static TypeName getSuperType(ClassInfo classInfo) {
    switch (classInfo.getType()) {
      case LYNX_UI:
        return ParameterizedTypeName.get(LYNX_UI_SETTER_TYPE, classInfo.mClassName);
      case SHADOW_NODE:
        return ParameterizedTypeName.get(SHADOW_NODE_SETTER_TYPE, classInfo.mClassName);
      default:
        throw new IllegalArgumentException();
    }
  }

  private static MethodSpec generateSetPropertySpec(
      ClassInfo classInfo, List<PropertyInfo> properties) {
    MethodSpec.Builder builder = MethodSpec.methodBuilder("setProperty")
                                     .addModifiers(PUBLIC)
                                     .addAnnotation(Override.class)
                                     .returns(TypeName.VOID);
    ClassName className;
    if (classInfo.getType() == SettableType.LYNX_UI) {
      className = LYNX_UI_TYPE;
    } else {
      className = SHADOW_NODE_IMPL_TYPE;
    }
    builder.addParameter(className, "manager");

    return builder.addParameter(STRING_TYPE, "name")
        .addParameter(PROPS_TYPE, "props")
        .addCode(generateSetProperty(classInfo, properties))
        .build();
  }

  private static CodeBlock generateSetProperty(ClassInfo info, List<PropertyInfo> properties) {
    if (properties.isEmpty()) {
      return CodeBlock.builder().addStatement("super.setProperty(manager, name, props)").build();
    }

    CodeBlock.Builder builder = CodeBlock.builder();

    builder.addStatement(
        "$T $N = ($T) $N", info.mClassName, "manager2", info.mClassName, "manager");

    builder.add("switch (name) {\n").indent();
    for (int i = 0, size = properties.size(); i < size; i++) {
      PropertyInfo propertyInfo = properties.get(i);
      builder.add("case \"$L\":\n", propertyInfo.mProperty.name()).indent();
      builder.add("manager2.$L(", propertyInfo.methodName);
      if (propertyInfo.mProperty instanceof GroupProperty) {
        builder.add("$L, ", ((GroupProperty) propertyInfo.mProperty).mGroupIndex);
      }
      if (BOXED_PRIMITIVES.contains(propertyInfo.propertyType)) {
        builder.add("props.isNull(name) ? null : ");
      }
      getPropertyExtractor(propertyInfo, builder);
      builder.addStatement(")");
      builder.addStatement("return").unindent();
    }
    builder.unindent().add("}\n");
    if (!isLynxUIType(TypeName.get(info.mElement.asType()))
        && !isShadowNodeType(TypeName.get(info.mElement.asType()))) {
      builder.addStatement("super.setProperty(manager, name, props)");
    }

    return builder.build();
  }

  private static CodeBlock.Builder getPropertyExtractor(
      PropertyInfo info, CodeBlock.Builder builder) {
    TypeName propertyType = info.propertyType;
    if (propertyType.equals(STRING_TYPE)) {
      return builder.add("props.getString(name)");
    } else if (propertyType.equals(READABLE_ARRAY_TYPE)) {
      return builder.add("props.getArray(name)");
    } else if (propertyType.equals(READABLE_MAP_TYPE)) {
      return builder.add("props.getMap(name)");
    } else if (propertyType.equals(DYNAMIC_TYPE)) {
      return builder.add("props.getDynamic(name)");
    }

    if (BOXED_PRIMITIVES.contains(propertyType)) {
      propertyType = propertyType.unbox();
    }

    if (propertyType.equals(TypeName.BOOLEAN)) {
      return builder.add("props.getBoolean(name, $L)", info.mProperty.defaultBoolean());
    }
    if (propertyType.equals(TypeName.DOUBLE)) {
      double defaultDouble = info.mProperty.defaultDouble();
      if (Double.isNaN(defaultDouble)) {
        return builder.add("props.getDouble(name, $T.NaN)", Double.class);
      } else {
        return builder.add("props.getDouble(name, $Lf)", defaultDouble);
      }
    }
    if (propertyType.equals(TypeName.FLOAT)) {
      float defaultFloat = info.mProperty.defaultFloat();
      if (Float.isNaN(defaultFloat)) {
        return builder.add("props.getFloat(name, $T.NaN)", Float.class);
      } else {
        return builder.add("props.getFloat(name, $Lf)", defaultFloat);
      }
    }
    if (propertyType.equals(TypeName.INT)) {
      return builder.add("props.getInt(name, $L)", info.mProperty.defaultInt());
    }

    throw new IllegalArgumentException();
  }

  private static void checkElement(Element element) throws ReactPropertyException {
    if (element.getKind() == ElementKind.METHOD && element.getModifiers().contains(PUBLIC)) {
      return;
    }

    throw new ReactPropertyException(
        "@LynxProp and @ReachPropGroup annotation must be on a public method", element);
  }

  private static boolean shouldIgnoreClass(ClassInfo classInfo) {
    return classInfo.mElement.getModifiers().contains(PRIVATE);
  }

  private static boolean shouldWarnClass(ClassInfo classInfo) {
    return classInfo.mElement.getModifiers().contains(PRIVATE);
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

  @Override
  public Set<String> getSupportedAnnotationTypes() {
    HashSet<String> annotationSet = new HashSet<>();
    annotationSet.add(LynxPropsHolder.class.getCanonicalName());
    return annotationSet;
  }

  private interface Property {
    String name();
    String customType();
    double defaultDouble();
    float defaultFloat();
    int defaultInt();
    boolean defaultBoolean();
  }

  private static class RegularProperty implements Property {
    private final LynxProp mProp;

    public RegularProperty(LynxProp prop) {
      mProp = prop;
    }

    @Override
    public String name() {
      return mProp.name();
    }

    @Override
    public String customType() {
      return mProp.customType();
    }

    @Override
    public double defaultDouble() {
      return mProp.defaultDouble();
    }

    @Override
    public float defaultFloat() {
      return mProp.defaultFloat();
    }

    @Override
    public int defaultInt() {
      return mProp.defaultInt();
    }

    @Override
    public boolean defaultBoolean() {
      return mProp.defaultBoolean();
    }
  }

  private static class GroupProperty implements Property {
    private final LynxPropGroup mProp;
    private final int mGroupIndex;

    public GroupProperty(LynxPropGroup prop, int groupIndex) {
      mProp = prop;
      mGroupIndex = groupIndex;
    }

    @Override
    public String name() {
      return mProp.names()[mGroupIndex];
    }

    @Override
    public String customType() {
      return mProp.customType();
    }

    @Override
    public double defaultDouble() {
      return mProp.defaultDouble();
    }

    @Override
    public float defaultFloat() {
      return mProp.defaultFloat();
    }

    @Override
    public int defaultInt() {
      return mProp.defaultInt();
    }

    @Override
    public boolean defaultBoolean() {
      throw new UnsupportedOperationException();
    }
  }

  private enum SettableType { LYNX_UI, SHADOW_NODE }

  private static class ClassInfo {
    public final ClassName mParentClassName;
    public final ClassName mClassName;
    public final Element mElement;
    public final List<PropertyInfo> mProperties;
    public final SettableType mSettableType;

    public ClassInfo(ClassName className, TypeElement element, SettableType settableType) {
      TypeName typeName = ClassName.get(element.getSuperclass());
      if (typeName instanceof ParameterizedTypeName) {
        mParentClassName = ((ParameterizedTypeName) typeName).rawType;
      } else if (typeName instanceof ClassName) {
        mParentClassName = (ClassName) typeName;
      } else {
        throw new RuntimeException("UnExpected classNameType.");
      }
      mClassName = className;
      mElement = element;
      mProperties = new ArrayList<>();
      mSettableType = settableType;
    }

    public SettableType getType() {
      return mSettableType;
    }

    public void addProperty(PropertyInfo propertyInfo) throws ReactPropertyException {
      String name = propertyInfo.mProperty.name();
      if (checkPropertyExists(name)) {
        System.out.print("Module " + mClassName + " has already registered a property named \""
            + name + "\". If you want to override a property, don't add"
            + "the @LynxProp annotation to the property in the subclass");
        return;
      }

      mProperties.add(propertyInfo);
    }

    private boolean checkPropertyExists(String name) {
      for (PropertyInfo propertyInfo : mProperties) {
        if (propertyInfo.mProperty.name().equals(name)) {
          return true;
        }
      }

      return false;
    }
  }

  private static class PropertyInfo {
    public final String methodName;
    public final TypeName propertyType;
    public final Element element;
    public final Property mProperty;

    private PropertyInfo(
        String methodName, TypeName propertyType, Element element, Property property) {
      this.methodName = methodName;
      this.propertyType = propertyType;
      this.element = element;
      mProperty = property;
    }

    public static class Builder {
      private final Types mTypes;
      private final Elements mElements;
      private final ClassInfo mClassInfo;

      public Builder(Types types, Elements elements, ClassInfo classInfo) {
        mTypes = types;
        mElements = elements;
        mClassInfo = classInfo;
      }

      public PropertyInfo build(Element element, Property property) throws ReactPropertyException {
        String methodName = element.getSimpleName().toString();

        ExecutableElement method = (ExecutableElement) element;
        List<? extends VariableElement> parameters = method.getParameters();

        if (parameters.size() != getArgCount(mClassInfo.getType(), property)) {
          throw new ReactPropertyException("Wrong number of args", element);
        }

        int index = 0;
        if (property instanceof GroupProperty) {
          TypeName indexType = TypeName.get(parameters.get(index++).asType());
          if (!indexType.equals(TypeName.INT)) {
            throw new ReactPropertyException(
                "Argument " + index + " must be an int for @LynxPropGroup", element);
          }
        }

        TypeName propertyType = TypeName.get(parameters.get(index++).asType());
        if (!DEFAULT_TYPES.containsKey(propertyType)) {
          throw new ReactPropertyException(
              "Argument " + index + " must be of a supported type", element);
        }

        return new PropertyInfo(methodName, propertyType, element, property);
      }

      private static int getArgCount(SettableType type, Property property) {
        int baseCount = 1;
        return property instanceof GroupProperty ? baseCount + 1 : baseCount;
      }
    }
  }

  private static class ReactPropertyException extends Exception {
    public final Element element;

    public ReactPropertyException(String message, PropertyInfo propertyInfo) {
      super(message);
      this.element = propertyInfo.element;
    }

    public ReactPropertyException(String message, Element element) {
      super(message);
      this.element = element;
    }
  }
}
