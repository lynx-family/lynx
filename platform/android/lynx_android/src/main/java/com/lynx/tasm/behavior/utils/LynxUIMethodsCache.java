// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.utils;

import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;

public class LynxUIMethodsCache {
  static HashMap<Class, HashMap<String, Method>> CLASS_METHODS_CACHE = new HashMap<>();

  public static HashMap<String, Method> getNativeMethodsForLynxUIClass(
      Class<? extends LynxBaseUI> cls) {
    if (cls == null) {
      return null;
    }
    return extractNativeMethodsForLynxUIClass(cls);
  }

  private static HashMap<String, Method> extractNativeMethodsForLynxUIClass(Class<?> cls) {
    if (CLASS_METHODS_CACHE.containsKey(cls)) {
      return CLASS_METHODS_CACHE.get(cls);
    }

    HashMap<String, Method> methods = new HashMap<>();
    Method[] targetMethods = cls.getDeclaredMethods();
    for (Method targetMethod : targetMethods) {
      LynxUIMethod annotation = targetMethod.getAnnotation(LynxUIMethod.class);
      if (annotation != null) {
        String methodName = targetMethod.getName();
        methods.put(methodName, targetMethod);
      }
    }
    Class<?> superClass = cls.getSuperclass();
    if (superClass != null) {
      methods.putAll(extractNativeMethodsForLynxUIClass(superClass));
    }
    CLASS_METHODS_CACHE.put(cls, methods);
    return methods;
  }
}
