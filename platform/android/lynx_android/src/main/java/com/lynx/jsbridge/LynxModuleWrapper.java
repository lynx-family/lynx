// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.jsbridge;

import android.content.Context;
import android.util.Pair;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
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
  private WeakReference<Context> mWeakContext;
  private LynxModule.AuthValidator mAuthValidator;

  public LynxModuleWrapper(String name, LynxModule module) {
    mName = name;
    mModule = module;
    mDescriptors = new ArrayList<>();
    mAttributeDescriptors = new ArrayList<>();
  }

  public void setLynxContext(WeakReference<Context> weakContext) {
    mWeakContext = weakContext;
  }

  @CalledByNative
  public LynxModule getModule() {
    return mModule;
  }

  @CalledByNative
  public String getName() {
    return mName;
  }

  public void setAuthValidator(LynxModule.AuthValidator authValidator) {
    mAuthValidator = authValidator;
  }

  @CalledByNative
  public boolean hasAuthValidator() {
    return mAuthValidator != null;
  }

  @CalledByNative
  public boolean verify(String moduleName, String methodName, JavaOnlyArray methodParams) {
    // If AuthValidator does not exist, it means that the current LynxMethod can be called.
    if (mAuthValidator == null) {
      return true;
    }

    return mAuthValidator.verify(moduleName, methodName, methodParams);
  }

  private void findMethods() {
    Class<? extends LynxModule> classForMethods = mModule.getClass();
    Method[] targetMethods = classForMethods.getDeclaredMethods();
    for (Method targetMethod : targetMethods) {
      LynxMethod annotation = targetMethod.getAnnotation(LynxMethod.class);
      if (annotation != null) {
        String methodName = targetMethod.getName();
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
        UIThreadUtils.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            if (mWeakContext != null) {
              Context lynxContext = mWeakContext.get();
              if (lynxContext != null && lynxContext instanceof LynxContext) {
                LynxError error = new LynxError(LynxSubErrorCode.E_NATIVE_MODULES_EXCEPTION,
                    "NativeModules: GetMethodDescriptors error!"
                        + "moduleName: " + mName + " exception: " + exp.toString());
                error.setLogBoxOnly(true);
                ((LynxContext) lynxContext).handleLynxError(error);
              }
            }
          }
        });
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
        UIThreadUtils.runOnUiThread(new Runnable() {
          @Override
          public void run() {
            if (mWeakContext != null) {
              Context lynxContext = mWeakContext.get();
              if (lynxContext != null && lynxContext instanceof LynxContext) {
                LynxError error = new LynxError(LynxSubErrorCode.E_NATIVE_MODULES_EXCEPTION,
                    "NativeModules: getAttributeDescriptors error!"
                        + "moduleName: " + mName + " exception: " + exp.toString());
                error.setLogBoxOnly(true);
                ((LynxContext) lynxContext).handleLynxError(error);
              }
            }
          }
        });
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
