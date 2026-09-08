// Copyright 2004-present Facebook. All Rights Reserved.

// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.processor.ksp;

import static javax.lang.model.element.Modifier.PUBLIC;

import com.google.devtools.ksp.UtilsKt;
import com.google.devtools.ksp.processing.CodeGenerator;
import com.google.devtools.ksp.processing.KSPLogger;
import com.google.devtools.ksp.processing.Resolver;
import com.google.devtools.ksp.processing.SymbolProcessor;
import com.google.devtools.ksp.processing.SymbolProcessorEnvironment;
import com.google.devtools.ksp.processing.SymbolProcessorProvider;
import com.google.devtools.ksp.symbol.KSAnnotated;
import com.google.devtools.ksp.symbol.KSAnnotation;
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
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * KSP port of {@code com.lynx.processor.LynxPropsProcessor}.
 *
 * <p>Generates {@code <Class>$$PropsSetter} holders for classes annotated with
 * {@code @LynxPropsHolder}, producing the same Java sources as the javac implementation.
 */
public class LynxPropsSymbolProcessor implements SymbolProcessor {
  private static final String PROPS_HOLDER_ANNOTATION = "com.lynx.tasm.behavior.LynxPropsHolder";
  private static final String PROP_ANNOTATION = "com.lynx.tasm.behavior.LynxProp";
  private static final String PROP_GROUP_ANNOTATION = "com.lynx.tasm.behavior.LynxPropGroup";

  private static final String LYNX_UI_QUALIFIED = "com.lynx.tasm.behavior.ui.LynxBaseUI";
  private static final String SHADOW_NODE_QUALIFIED = "com.lynx.tasm.behavior.shadow.ShadowNode";

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

  static {
    DEFAULT_TYPES = new HashMap<>();

    // Primitives
    DEFAULT_TYPES.put(TypeName.BOOLEAN, "boolean");
    DEFAULT_TYPES.put(TypeName.DOUBLE, "number");
    DEFAULT_TYPES.put(TypeName.FLOAT, "number");
    DEFAULT_TYPES.put(TypeName.INT, "number");
    DEFAULT_TYPES.put(TypeName.LONG, "number");

    // Boxed primitives
    DEFAULT_TYPES.put(TypeName.BOOLEAN.box(), "boolean");
    DEFAULT_TYPES.put(TypeName.INT.box(), "number");
    DEFAULT_TYPES.put(TypeName.LONG.box(), "number");

    // Class types
    DEFAULT_TYPES.put(STRING_TYPE, "String");
    DEFAULT_TYPES.put(READABLE_ARRAY_TYPE, "Array");
    DEFAULT_TYPES.put(READABLE_MAP_TYPE, "Map");
    DEFAULT_TYPES.put(DYNAMIC_TYPE, "Dynamic");

    BOXED_PRIMITIVES = new HashSet<>();
    BOXED_PRIMITIVES.add(TypeName.BOOLEAN.box());
    BOXED_PRIMITIVES.add(TypeName.FLOAT.box());
    BOXED_PRIMITIVES.add(TypeName.INT.box());
    BOXED_PRIMITIVES.add(TypeName.LONG.box());
  }

  private final CodeGenerator mCodeGenerator;
  private final KSPLogger mLogger;
  private final Map<ClassName, ClassInfo> mClasses = new HashMap<>();

  public LynxPropsSymbolProcessor(SymbolProcessorEnvironment environment) {
    mCodeGenerator = environment.getCodeGenerator();
    mLogger = environment.getLogger();
  }

  @Override
  public List<KSAnnotated> process(Resolver resolver) {
    // Clear properties from previous rounds
    mClasses.clear();

    Iterator<KSAnnotated> symbols =
        resolver.getSymbolsWithAnnotation(PROPS_HOLDER_ANNOTATION, false).iterator();
    while (symbols.hasNext()) {
      KSAnnotated symbol = symbols.next();
      if (!(symbol instanceof KSClassDeclaration)) {
        continue;
      }
      KSClassDeclaration classType = (KSClassDeclaration) symbol;
      try {
        ClassName className = KspUtils.toClassName(classType);
        mClasses.put(className, parseClass(className, classType));
      } catch (Exception e) {
        error(classType, e.getMessage());
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
        } else {
          warning(classInfo.mDeclaration, "Class was skipped. Classes need to be non-private.");
        }
      } catch (IOException e) {
        error(null, e.getMessage());
      } catch (ReactPropertyException e) {
        error(e.node, e.getMessage());
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
    SettableType settableType = findSettableType(classType);

    ClassInfo classInfo = new ClassInfo(className, classType, settableType);
    findProperties(classInfo, classType);

    return classInfo;
  }

  /** Walks the superclass chain to decide whether the holder targets a LynxUI or a ShadowNode. */
  private static SettableType findSettableType(KSClassDeclaration classType) {
    KSClassDeclaration current = classType;
    while (current != null) {
      String qualifiedName = KspUtils.qualifiedNameOf(current);
      if (LYNX_UI_QUALIFIED.equals(qualifiedName)) {
        return SettableType.LYNX_UI;
      }
      if (SHADOW_NODE_QUALIFIED.equals(qualifiedName)) {
        return SettableType.SHADOW_NODE;
      }
      if ("kotlin.Any".equals(qualifiedName) || "java.lang.Object".equals(qualifiedName)) {
        throw new IllegalArgumentException("Could not find target type " + qualifiedName);
      }
      current = KspUtils.superclassOf(current);
    }
    throw new IllegalArgumentException("Could not find target type for " + classType);
  }

  private void findProperties(ClassInfo classInfo, KSClassDeclaration classType) {
    Iterator<KSDeclaration> declarations = classType.getDeclarations().iterator();
    while (declarations.hasNext()) {
      KSDeclaration declaration = declarations.next();
      if (!(declaration instanceof KSFunctionDeclaration)) {
        continue;
      }
      KSFunctionDeclaration method = (KSFunctionDeclaration) declaration;
      KSAnnotation prop = KspUtils.findAnnotation(method, PROP_ANNOTATION);
      KSAnnotation propGroup = KspUtils.findAnnotation(method, PROP_GROUP_ANNOTATION);

      try {
        if (prop != null || propGroup != null) {
          checkElement(method);
        }

        if (prop != null) {
          classInfo.addProperty(PropertyInfo.build(classInfo, method, new RegularProperty(prop)));
        } else if (propGroup != null) {
          List<String> names = KspUtils.stringList(KspUtils.argument(propGroup, "names"));
          for (int i = 0, size = names.size(); i < size; i++) {
            classInfo.addProperty(
                PropertyInfo.build(classInfo, method, new GroupProperty(propGroup, names, i)));
          }
        }
      } catch (ReactPropertyException e) {
        error(e.node, e.getMessage());
      }
    }
  }

  private void generateCode(ClassInfo classInfo, List<PropertyInfo> properties)
      throws IOException, ReactPropertyException {
    TypeName superType = getSuperType(classInfo);
    ClassName className = classInfo.mClassName;

    ClassName pClassName = null;
    if (!classInfo.mIsLynxUIBase && !classInfo.mIsShadowNodeBase) {
      String packageName = classInfo.mParentClassName.packageName();
      String simpleName = classInfo.mParentClassName.simpleName() + "$$PropsSetter";
      pClassName = ClassName.get(packageName, simpleName);
    }

    String holderClassName =
        getClassName(classInfo.mDeclaration, className.packageName()) + "$$PropsSetter";
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
                            .addFileComment("Generated by com.lynx.processor.LynxPropsProcessor")
                            .build();

    KspUtils.write(mCodeGenerator, classInfo.mDeclaration.getContainingFile(),
        className.packageName(), holderClassName, javaFile);
  }

  private static String getClassName(KSClassDeclaration type, String packageName) {
    int packageLen = packageName.length() + 1;
    return KspUtils.qualifiedNameOf(type).substring(packageLen).replace('.', '$');
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
    if (!info.mIsLynxUIBase && !info.mIsShadowNodeBase) {
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
    if (propertyType.equals(TypeName.LONG)) {
      return builder.add("props.getLong(name, 0L)");
    }

    throw new IllegalArgumentException();
  }

  private static void checkElement(KSFunctionDeclaration method) throws ReactPropertyException {
    if (UtilsKt.isPublic(method)) {
      return;
    }

    throw new ReactPropertyException(
        "@LynxProp and @ReachPropGroup annotation must be on a public method", method);
  }

  private static boolean shouldIgnoreClass(ClassInfo classInfo) {
    return UtilsKt.isPrivate(classInfo.mDeclaration);
  }

  private void error(KSNode node, String message) {
    mLogger.error(message != null ? message : "unknown error", node);
  }

  private void warning(KSNode node, String message) {
    mLogger.warn(message, node);
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
    private final KSAnnotation mProp;

    RegularProperty(KSAnnotation prop) {
      mProp = prop;
    }

    @Override
    public String name() {
      return KspUtils.argument(mProp, "name");
    }

    @Override
    public String customType() {
      return KspUtils.argument(mProp, "customType");
    }

    @Override
    public double defaultDouble() {
      Number value = KspUtils.argument(mProp, "defaultDouble");
      return value != null ? value.doubleValue() : 0.0;
    }

    @Override
    public float defaultFloat() {
      Number value = KspUtils.argument(mProp, "defaultFloat");
      return value != null ? value.floatValue() : 0.0f;
    }

    @Override
    public int defaultInt() {
      Number value = KspUtils.argument(mProp, "defaultInt");
      return value != null ? value.intValue() : 0;
    }

    @Override
    public boolean defaultBoolean() {
      Boolean value = KspUtils.argument(mProp, "defaultBoolean");
      return value != null && value;
    }
  }

  private static class GroupProperty implements Property {
    private final KSAnnotation mProp;
    private final List<String> mNames;
    final int mGroupIndex;

    GroupProperty(KSAnnotation prop, List<String> names, int groupIndex) {
      mProp = prop;
      mNames = names;
      mGroupIndex = groupIndex;
    }

    @Override
    public String name() {
      return mNames.get(mGroupIndex);
    }

    @Override
    public String customType() {
      return KspUtils.argument(mProp, "customType");
    }

    @Override
    public double defaultDouble() {
      Number value = KspUtils.argument(mProp, "defaultDouble");
      return value != null ? value.doubleValue() : 0.0;
    }

    @Override
    public float defaultFloat() {
      Number value = KspUtils.argument(mProp, "defaultFloat");
      return value != null ? value.floatValue() : 0.0f;
    }

    @Override
    public int defaultInt() {
      Number value = KspUtils.argument(mProp, "defaultInt");
      return value != null ? value.intValue() : 0;
    }

    @Override
    public boolean defaultBoolean() {
      throw new UnsupportedOperationException();
    }
  }

  private enum SettableType { LYNX_UI, SHADOW_NODE }

  private static class ClassInfo {
    final ClassName mParentClassName;
    final ClassName mClassName;
    final KSClassDeclaration mDeclaration;
    final List<PropertyInfo> mProperties;
    final SettableType mSettableType;
    final boolean mIsLynxUIBase;
    final boolean mIsShadowNodeBase;

    ClassInfo(ClassName className, KSClassDeclaration declaration, SettableType settableType) {
      KSClassDeclaration superclass = KspUtils.superclassOf(declaration);
      if (superclass != null) {
        ClassName superclassName = KspUtils.toClassName(superclass);
        String qualifiedName = KspUtils.qualifiedNameOf(superclass);
        mParentClassName = "kotlin.Any".equals(qualifiedName)
            ? ClassName.get("java.lang", "Object")
            : superclassName;
      } else {
        mParentClassName = ClassName.get("java.lang", "Object");
      }
      mClassName = className;
      mDeclaration = declaration;
      mProperties = new ArrayList<>();
      mSettableType = settableType;

      String ownQualifiedName = KspUtils.qualifiedNameOf(declaration);
      mIsLynxUIBase = LYNX_UI_QUALIFIED.equals(ownQualifiedName);
      mIsShadowNodeBase = SHADOW_NODE_QUALIFIED.equals(ownQualifiedName);
    }

    SettableType getType() {
      return mSettableType;
    }

    void addProperty(PropertyInfo propertyInfo) throws ReactPropertyException {
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
    final String methodName;
    final TypeName propertyType;
    final KSNode node;
    final Property mProperty;

    private PropertyInfo(String methodName, TypeName propertyType, KSNode node, Property property) {
      this.methodName = methodName;
      this.propertyType = propertyType;
      this.node = node;
      mProperty = property;
    }

    static PropertyInfo build(ClassInfo classInfo, KSFunctionDeclaration method, Property property)
        throws ReactPropertyException {
      String methodName = method.getSimpleName().asString();

      List<KSValueParameter> parameters = method.getParameters();

      if (parameters.size() != getArgCount(property)) {
        throw new ReactPropertyException("Wrong number of args", method);
      }

      int index = 0;
      if (property instanceof GroupProperty) {
        TypeName indexType = KspUtils.toTypeName(parameters.get(index++).getType().resolve());
        if (!indexType.equals(TypeName.INT)) {
          throw new ReactPropertyException(
              "Argument " + index + " must be an int for @LynxPropGroup", method);
        }
      }

      TypeName propertyType = KspUtils.toTypeName(parameters.get(index++).getType().resolve());
      if (!DEFAULT_TYPES.containsKey(propertyType)) {
        throw new ReactPropertyException(
            "Argument " + index + " must be of a supported type", method);
      }

      return new PropertyInfo(methodName, propertyType, method, property);
    }

    private static int getArgCount(Property property) {
      int baseCount = 1;
      return property instanceof GroupProperty ? baseCount + 1 : baseCount;
    }
  }

  private static class ReactPropertyException extends Exception {
    final KSNode node;

    ReactPropertyException(String message, KSNode node) {
      super(message);
      this.node = node;
    }
  }

  /** Entry point registered in META-INF/services. */
  public static class Provider implements SymbolProcessorProvider {
    @Override
    public SymbolProcessor create(SymbolProcessorEnvironment environment) {
      return new LynxPropsSymbolProcessor(environment);
    }
  }
}
