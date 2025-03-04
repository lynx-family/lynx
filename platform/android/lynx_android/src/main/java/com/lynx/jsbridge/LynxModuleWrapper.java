// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.util.Pair;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

public class LynxModuleWrapper {
  private static final String TAG = "LynxModuleWrapper";
  private final LynxModule mModule;
  private final ArrayList<MethodDescriptor> mDescriptors;
  private final ArrayList<AttributeDescriptor> mAttributeDescriptors;
  private final String mName;

  public LynxModuleWrapper(String name, LynxModule module) {
    mName = name;
    mModule = module;
    mDescriptors = new ArrayList<>();
    mAttributeDescriptors = new ArrayList<>();
  }

  @CalledByNative
  public LynxModule getModule() {
    return mModule;
  }

  @CalledByNative
  public String getName() {
    return mName;
  }

  private void findMethods() {
    Set<String> methodNames = new HashSet<>();
    Class<? extends LynxModule> classForMethods = mModule.getClass();
    Method[] targetMethods = classForMethods.getDeclaredMethods();
    for (Method targetMethod : targetMethods) {
      LynxMethod annotation = targetMethod.getAnnotation(LynxMethod.class);
      if (annotation != null) {
        String methodName = targetMethod.getName();
        if (methodNames.contains(methodName)) {
          throw new IllegalArgumentException(
              "Java Module " + getName() + " method name already registered: " + methodName);
        }
        methodNames.add(methodName);
        MethodDescriptor md = new MethodDescriptor();
        LynxMethodWrapper method = new LynxMethodWrapper(targetMethod);
        md.name = methodName;
        md.signature = method.getSignature();
        md.method = targetMethod;
        mDescriptors.add(md);
      }
    }
  }

  private void findAttributes() {
    Set<String> attributeNames = new HashSet<>();
    Class<? extends LynxModule> classForAttributes = mModule.getClass();
    Field[] targetAttributes = classForAttributes.getDeclaredFields();
    for (Field targetAttr : targetAttributes) {
      LynxAttribute annotation = targetAttr.getAnnotation(LynxAttribute.class);
      if (annotation != null) {
        String attributeName = targetAttr.getName();
        if (attributeNames.contains(attributeName)) {
          throw new IllegalArgumentException(
              "Java Module " + getName() + " attribute name already registered: " + attributeName);
        }
        attributeNames.add(attributeName);
        JavaOnlyArray value = new JavaOnlyArray();
        try {
          value.add(targetAttr.get(mModule));
        } catch (IllegalAccessException exp) {
          LLog.e(TAG, exp.toString());
        }
        mAttributeDescriptors.add(new AttributeDescriptor(attributeName, value));
      }
    }
  }

  @CalledByNative
  public Collection<MethodDescriptor> getMethodDescriptors() {
    if (mDescriptors.isEmpty()) {
      try {
        findMethods();
      } catch (RuntimeException exp) {
        LLog.e(TAG, exp.toString());
      }
    }
    return mDescriptors;
  }

  @CalledByNative
  public Collection<AttributeDescriptor> getAttributeDescriptor() {
    if (mAttributeDescriptors.isEmpty()) {
      try {
        findAttributes();
      } catch (RuntimeException exp) {
        LLog.e(TAG, exp.toString());
      }
    }
    return mAttributeDescriptors;
  }

  public void destroy() {
    if (mModule != null) {
      mModule.destroy();
    }
  }
}
