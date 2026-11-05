// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.processor.ksp;

import com.google.devtools.ksp.processing.CodeGenerator;
import com.google.devtools.ksp.processing.Dependencies;
import com.google.devtools.ksp.symbol.ClassKind;
import com.google.devtools.ksp.symbol.KSAnnotated;
import com.google.devtools.ksp.symbol.KSAnnotation;
import com.google.devtools.ksp.symbol.KSClassDeclaration;
import com.google.devtools.ksp.symbol.KSDeclaration;
import com.google.devtools.ksp.symbol.KSFile;
import com.google.devtools.ksp.symbol.KSType;
import com.google.devtools.ksp.symbol.KSTypeReference;
import com.google.devtools.ksp.symbol.Nullability;
import com.squareup.javapoet.ClassName;
import com.squareup.javapoet.JavaFile;
import com.squareup.javapoet.TypeName;
import java.io.IOException;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

/**
 * Shared helpers for the KSP based Lynx processors.
 *
 * <p>The javac processors operate on {@code javax.lang.model} mirrors while KSP exposes symbols
 * through Kotlin's type system (e.g. Java {@code int} is surfaced as {@code kotlin.Int} with
 * {@link Nullability#NOT_NULL}). The helpers in this class translate KSP symbols back to the
 * JavaPoet {@link TypeName}s the original code generators were written against, so the generated
 * sources stay byte-identical with the javac processors.
 */
final class KspUtils {
  private static final Map<String, TypeName> KOTLIN_PRIMITIVES;
  private static final Map<String, ClassName> KOTLIN_TO_JAVA;

  static {
    KOTLIN_PRIMITIVES = new HashMap<>();
    KOTLIN_PRIMITIVES.put("kotlin.Boolean", TypeName.BOOLEAN);
    KOTLIN_PRIMITIVES.put("kotlin.Byte", TypeName.BYTE);
    KOTLIN_PRIMITIVES.put("kotlin.Short", TypeName.SHORT);
    KOTLIN_PRIMITIVES.put("kotlin.Int", TypeName.INT);
    KOTLIN_PRIMITIVES.put("kotlin.Long", TypeName.LONG);
    KOTLIN_PRIMITIVES.put("kotlin.Char", TypeName.CHAR);
    KOTLIN_PRIMITIVES.put("kotlin.Float", TypeName.FLOAT);
    KOTLIN_PRIMITIVES.put("kotlin.Double", TypeName.DOUBLE);

    KOTLIN_TO_JAVA = new HashMap<>();
    KOTLIN_TO_JAVA.put("kotlin.Any", ClassName.get("java.lang", "Object"));
    KOTLIN_TO_JAVA.put("kotlin.String", ClassName.get("java.lang", "String"));
    KOTLIN_TO_JAVA.put("kotlin.CharSequence", ClassName.get("java.lang", "CharSequence"));
    KOTLIN_TO_JAVA.put("kotlin.Unit", ClassName.get("java.lang", "Void"));
  }

  private KspUtils() {}

  /** Maps a resolved KSP type to the JavaPoet type the javac processor would have seen. */
  static TypeName toTypeName(KSType type) {
    KSDeclaration declaration = type.getDeclaration();
    String qualifiedName =
        declaration.getQualifiedName() != null ? declaration.getQualifiedName().asString() : null;
    if (qualifiedName == null) {
      throw new IllegalArgumentException("Could not resolve type " + type);
    }

    TypeName primitive = KOTLIN_PRIMITIVES.get(qualifiedName);
    if (primitive != null) {
      // Java primitives surface as non-null kotlin types; boxed/nullable variants stay boxed.
      return type.getNullability() == Nullability.NOT_NULL ? primitive : primitive.box();
    }

    ClassName mapped = KOTLIN_TO_JAVA.get(qualifiedName);
    if (mapped != null) {
      return mapped;
    }

    if (declaration instanceof KSClassDeclaration) {
      return toClassName((KSClassDeclaration) declaration);
    }
    return ClassName.bestGuess(qualifiedName);
  }

  /** Builds a JavaPoet ClassName, preserving nesting (package.Outer.Inner). */
  static ClassName toClassName(KSClassDeclaration declaration) {
    String packageName = declaration.getPackageName().asString();
    Deque<String> simpleNames = new ArrayDeque<>();
    KSDeclaration current = declaration;
    while (current != null) {
      simpleNames.addFirst(current.getSimpleName().asString());
      current = current.getParentDeclaration();
    }
    String first = simpleNames.removeFirst();
    return ClassName.get(packageName, first, simpleNames.toArray(new String[0]));
  }

  /** Returns the direct superclass declaration, skipping interfaces. Null for kotlin.Any roots. */
  static KSClassDeclaration superclassOf(KSClassDeclaration declaration) {
    Iterator<KSTypeReference> it = declaration.getSuperTypes().iterator();
    while (it.hasNext()) {
      KSType resolved = it.next().resolve();
      KSDeclaration superDeclaration = resolved.getDeclaration();
      if (superDeclaration instanceof KSClassDeclaration
          && ((KSClassDeclaration) superDeclaration).getClassKind() == ClassKind.CLASS) {
        return (KSClassDeclaration) superDeclaration;
      }
    }
    return null;
  }

  static String qualifiedNameOf(KSDeclaration declaration) {
    return declaration.getQualifiedName() != null ? declaration.getQualifiedName().asString()
                                                  : declaration.getSimpleName().asString();
  }

  /** Finds an annotation by fully qualified name. */
  static KSAnnotation findAnnotation(KSAnnotated annotated, String qualifiedName) {
    String shortName = qualifiedName.substring(qualifiedName.lastIndexOf('.') + 1);
    Iterator<KSAnnotation> it = annotated.getAnnotations().iterator();
    while (it.hasNext()) {
      KSAnnotation annotation = it.next();
      if (!shortName.equals(annotation.getShortName().asString())) {
        continue;
      }
      KSDeclaration declaration = annotation.getAnnotationType().resolve().getDeclaration();
      if (qualifiedName.equals(qualifiedNameOf(declaration))) {
        return annotation;
      }
    }
    return null;
  }

  /** Reads an annotation argument; KSP fills unspecified arguments with their defaults. */
  @SuppressWarnings("unchecked")
  static <T> T argument(KSAnnotation annotation, String name) {
    for (com.google.devtools.ksp.symbol.KSValueArgument argument : annotation.getArguments()) {
      if (argument.getName() != null && name.equals(argument.getName().asString())) {
        return (T) argument.getValue();
      }
    }
    return null;
  }

  /** Annotation array arguments arrive as List; normalizes to a String list. */
  static List<String> stringList(Object value) {
    List<String> result = new ArrayList<>();
    if (value instanceof List) {
      for (Object item : (List<?>) value) {
        result.add(String.valueOf(item));
      }
    } else if (value instanceof String[]) {
      for (String item : (String[]) value) {
        result.add(item);
      }
    }
    return result;
  }

  /** Writes a JavaFile through the KSP CodeGenerator, mirroring Filer#createSourceFile. */
  static void write(CodeGenerator codeGenerator, KSFile source, String packageName, String fileName,
      JavaFile javaFile) throws IOException {
    Dependencies dependencies =
        source != null ? new Dependencies(false, source) : new Dependencies(false);
    OutputStream stream = codeGenerator.createNewFile(dependencies, packageName, fileName, "java");
    try (Writer writer = new OutputStreamWriter(stream, StandardCharsets.UTF_8)) {
      javaFile.writeTo(writer);
    }
  }
}
