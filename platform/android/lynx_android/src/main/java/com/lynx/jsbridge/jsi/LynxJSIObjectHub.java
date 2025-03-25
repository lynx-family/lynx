// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.jsi;

import androidx.annotation.RestrictTo;
import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import java.util.concurrent.ConcurrentHashMap;
/**
 * Class that collects, stores, and provides LynxJSIObjectDescriptors.
 */
@RestrictTo(RestrictTo.Scope.LIBRARY)
public class LynxJSIObjectHub {
  private final static String TAG = "LynxJSIObjectHub";
  private static volatile LynxJSIObjectHub sInstance = null;
  private final ConcurrentHashMap<String, ILynxJSIObjectDescriptor> cacheDescriptors =
      new ConcurrentHashMap<>();

  public static LynxJSIObjectHub inst() {
    if (sInstance == null) {
      synchronized (LynxJSIObjectHub.class) {
        if (sInstance == null) {
          sInstance = new LynxJSIObjectHub();
        }
      }
    }
    return sInstance;
  }

  /**
   * provide LynxJSIObjectDescriptor
   */
  public ILynxJSIObjectDescriptor getLynxJSIObjectDescriptor(String className) {
    ILynxJSIObjectDescriptor descriptor = cacheDescriptors.get(className);
    if (descriptor == null) {
      descriptor = reflectJSIObjectDescriptor(className);
      if (descriptor != null) {
        cacheDescriptors.put(className, descriptor);
      }
    }
    return descriptor;
  }

  private static ILynxJSIObjectDescriptor reflectJSIObjectDescriptor(String className) {
    try {
      Class<?> runtimeFoundClass = Class.forName(className + "$$Descriptor");
      return (ILynxJSIObjectDescriptor) runtimeFoundClass.newInstance();
    } catch (Exception e) {
      LLog.e(TAG, "getJSIObjectDescriptor failed, error: " + e + ", className: " + className);
      return null;
    }
  }

  @CalledByNative
  private static ILynxJSIObjectDescriptor getJSIObjectDescriptor(ILynxJSIObject jsiObject) {
    return LynxJSIObjectHub.inst().getLynxJSIObjectDescriptor(jsiObject.getClass().getName());
  }

  /**
   * Type of JSIObject field
   * assign with lynx::piper::LynxPlatformJSIObjectAndroid::JObjectType in native
   */
  private enum JObjectType {
    UNKNOWN_TYPE,
    LYNX_JSI_OBJECT_TYPE,
    STRING_TYPE,
    OBJECT_ARRAY_TYPE,
    BOOLEAN_ARRAY_TYPE,
    INT_ARRAY_TYPE,
    LONG_ARRAY_TYPE,
    FLOAT_ARRAY_TYPE,
    DOUBLE_ARRAY_TYPE,
  }

  @CalledByNative
  private static int getJSIObjectFieldType(Object fieldObject) {
    if (fieldObject == null) {
      return JObjectType.UNKNOWN_TYPE.ordinal();
    } else if (fieldObject instanceof ILynxJSIObject) {
      return JObjectType.LYNX_JSI_OBJECT_TYPE.ordinal();
    } else if (fieldObject instanceof String) {
      return JObjectType.STRING_TYPE.ordinal();
    } else if (fieldObject instanceof boolean[]) {
      return JObjectType.BOOLEAN_ARRAY_TYPE.ordinal();
    } else if (fieldObject instanceof int[]) {
      return JObjectType.INT_ARRAY_TYPE.ordinal();
    } else if (fieldObject instanceof long[]) {
      return JObjectType.LONG_ARRAY_TYPE.ordinal();
    } else if (fieldObject instanceof float[]) {
      return JObjectType.FLOAT_ARRAY_TYPE.ordinal();
    } else if (fieldObject instanceof double[]) {
      return JObjectType.DOUBLE_ARRAY_TYPE.ordinal();
    } else if (fieldObject.getClass().isArray()) {
      return JObjectType.OBJECT_ARRAY_TYPE.ordinal();
    }
    return JObjectType.UNKNOWN_TYPE.ordinal();
  }
}
