/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import static com.lynx.tasm.base.Assertions.assertNotNull;

import androidx.annotation.Nullable;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.PiperData;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.WritableArray;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.base.Assertions;
import java.lang.reflect.Method;

// issue: #1510
// Note: Modify lynx/jsbridge/MethodInvoker.cc if the following config changed!

public class LynxMethodWrapper {
  private final Method mMethod;
  private final Class[] mParameterTypes;
  private boolean mArgumentsProcessed = false;
  private @Nullable String mSignature;

  LynxMethodWrapper(Method method) {
    mMethod = method;
    mMethod.setAccessible(true);
    mParameterTypes = mMethod.getParameterTypes();
  }

  private static char paramTypeToChar(Class paramClass) {
    char tryCommon = commonTypeToChar(paramClass);
    if (tryCommon != '\0') {
      return tryCommon;
    }
    if (paramClass == Callback.class) {
      return 'X';
    } else if (paramClass == Promise.class) {
      return 'P';
    } else if (paramClass == ReadableMap.class) {
      return 'M';
    } else if (paramClass == ReadableArray.class) {
      return 'A';
    } else if (paramClass == Dynamic.class) {
      return 'Y';
    } else if (paramClass == byte[].class) {
      return 'a';
    } else {
      throw new RuntimeException("Got unknown param class: " + paramClass.getSimpleName());
    }
  }

  private static char returnTypeToChar(Class returnClass) {
    // Keep this in sync with MethodInvoker
    char tryCommon = commonTypeToChar(returnClass);
    if (tryCommon != '\0') {
      return tryCommon;
    }
    if (returnClass == void.class) {
      return 'v';
    } else if (returnClass == WritableMap.class) {
      return 'M';
    } else if (returnClass == WritableArray.class) {
      return 'A';
    } else if (returnClass == byte[].class) {
      return 'a';
    } else if (returnClass == PiperData.class) {
      return 'J';
    } else {
      throw new RuntimeException("Got unknown return class: " + returnClass.getSimpleName());
    }
  }

  private static char commonTypeToChar(Class typeClass) {
    if (typeClass.equals(byte.class)) {
      return 'b';
    } else if (typeClass.equals(Byte.class)) {
      return 'B';
    } else if (typeClass.equals(short.class)) {
      return 's';
    } else if (typeClass.equals(Short.class)) {
      return 'S';
    } else if (typeClass.equals(long.class)) {
      return 'l';
    } else if (typeClass.equals(Long.class)) {
      return 'L';
    } else if (typeClass.equals(char.class)) {
      return 'c';
    } else if (typeClass.equals(Character.class)) {
      return 'C';
    } else if (typeClass.equals(boolean.class)) {
      return 'z';
    } else if (typeClass.equals(Boolean.class)) {
      return 'Z';
    } else if (typeClass.equals(int.class)) {
      return 'i';
    } else if (typeClass.equals(Integer.class)) {
      return 'I';
    } else if (typeClass.equals(double.class)) {
      return 'd';
    } else if (typeClass.equals(Double.class)) {
      return 'D';
    } else if (typeClass.equals(float.class)) {
      return 'f';
    } else if (typeClass.equals(Float.class)) {
      return 'F';
    } else if (typeClass.equals(String.class)) {
      return 'T';
    } else {
      return '\0';
    }
  }

  private void processArguments() {
    if (mArgumentsProcessed) {
      return;
    }
    mArgumentsProcessed = true;
    mSignature = buildSignature(mMethod, mParameterTypes);
  }

  public Method getMethod() {
    return mMethod;
  }

  public String getSignature() {
    if (!mArgumentsProcessed) {
      processArguments();
    }
    return assertNotNull(mSignature);
  }

  private String buildSignature(Method method, Class[] paramTypes) {
    StringBuilder builder = new StringBuilder(paramTypes.length + 2);

    builder.append(returnTypeToChar(method.getReturnType()));
    builder.append('.');

    for (int i = 0; i < paramTypes.length; i++) {
      Class paramClass = paramTypes[i];
      if (paramClass == Promise.class) {
        Assertions.assertCondition(
            i == paramTypes.length - 1, "Promise must be used as last parameter only");
      }
      builder.append(paramTypeToChar(paramClass));
    }

    return builder.toString();
  }
}
