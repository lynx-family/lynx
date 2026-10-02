// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.processor.ksp;

import com.google.devtools.ksp.symbol.KSAnnotated;
import com.google.devtools.ksp.symbol.KSAnnotation;
import com.google.devtools.ksp.symbol.KSClassDeclaration;
import com.google.devtools.ksp.symbol.KSDeclaration;
import com.google.devtools.ksp.symbol.KSName;
import com.google.devtools.ksp.symbol.KSType;
import com.google.devtools.ksp.symbol.KSTypeReference;
import com.google.devtools.ksp.symbol.KSValueArgument;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import kotlin.sequences.Sequence;
import kotlin.sequences.SequencesKt;

final class KspUtils {
  private KspUtils() {}

  static <T> List<T> toList(Sequence<? extends T> sequence) {
    return SequencesKt.toList(sequence);
  }

  static String qualifiedName(KSDeclaration declaration) {
    KSName name = declaration.getQualifiedName();
    return name == null ? declaration.getSimpleName().asString() : name.asString();
  }

  static String packageName(KSDeclaration declaration) {
    return declaration.getPackageName().asString();
  }

  static KSAnnotation annotation(KSAnnotated annotated, String annotationName) {
    for (KSAnnotation annotation : toList(annotated.getAnnotations())) {
      KSDeclaration declaration = annotation.getAnnotationType().resolve().getDeclaration();
      if (annotationName.equals(qualifiedName(declaration))) {
        return annotation;
      }
    }
    return null;
  }

  static Object argument(KSAnnotation annotation, String name) {
    if (annotation == null) {
      return null;
    }
    for (KSValueArgument argument : annotation.getArguments()) {
      KSName argumentName = argument.getName();
      if (argumentName != null && name.equals(argumentName.asString())) {
        return argument.getValue();
      }
    }
    return null;
  }

  static String stringArgument(KSAnnotation annotation, String name, String fallback) {
    Object value = argument(annotation, name);
    return value instanceof String ? (String) value : fallback;
  }

  static boolean booleanArgument(KSAnnotation annotation, String name, boolean fallback) {
    Object value = argument(annotation, name);
    return value instanceof Boolean ? (Boolean) value : fallback;
  }

  static int intArgument(KSAnnotation annotation, String name, int fallback) {
    Object value = argument(annotation, name);
    return value instanceof Integer ? (Integer) value : fallback;
  }

  static float floatArgument(KSAnnotation annotation, String name, float fallback) {
    Object value = argument(annotation, name);
    return value instanceof Float ? (Float) value : fallback;
  }

  static double doubleArgument(KSAnnotation annotation, String name, double fallback) {
    Object value = argument(annotation, name);
    return value instanceof Double ? (Double) value : fallback;
  }

  static List<String> stringListArgument(KSAnnotation annotation, String name) {
    Object value = argument(annotation, name);
    if (!(value instanceof List)) {
      return Collections.emptyList();
    }
    List<String> result = new ArrayList<>();
    for (Object item : (List<?>) value) {
      if (item instanceof String) {
        result.add((String) item);
      }
    }
    return result;
  }

  static String typeArgument(KSAnnotation annotation, String name) {
    Object value = argument(annotation, name);
    if (value instanceof KSType) {
      return qualifiedName(((KSType) value).getDeclaration());
    }
    if (value instanceof KSTypeReference) {
      return qualifiedName(((KSTypeReference) value).resolve().getDeclaration());
    }
    return null;
  }

  static KSClassDeclaration asClass(KSAnnotated symbol) {
    return symbol instanceof KSClassDeclaration ? (KSClassDeclaration) symbol : null;
  }
}
