// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import android.content.Context;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.jsbridge.LynxModule;
import com.lynx.jsbridge.Promise;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.PiperData;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.WritableArray;
import com.lynx.react.bridge.WritableMap;
import com.lynx.tasm.core.LynxThreadPool;

/** Bridge fixtures shared by the JSB module and standalone-background-runtime test cards. */
public class JSBTestModule extends LynxModule {
  public static final String NAME = "JSBTestModule";

  public JSBTestModule(Context context) {
    super(context);
  }

  public JSBTestModule(Context context, Object param) {
    super(context, param);
  }

  @LynxMethod
  public byte getByte(byte param) {
    return param;
  }

  @LynxMethod
  public short getShort(short param) {
    return param;
  }

  @LynxMethod
  public boolean getBoolean(boolean param) {
    return param;
  }

  @LynxMethod
  public char getChar(char param) {
    return param;
  }

  @LynxMethod
  public double getDouble(double param) {
    return param;
  }

  @LynxMethod
  public byte[] getArrayBuffer(byte[] buffer) {
    return buffer;
  }

  @LynxMethod
  public String getString(String param) {
    return param;
  }

  @LynxMethod
  public long getBigInt(long param) {
    return param;
  }

  @LynxMethod
  public WritableMap getMap(ReadableMap param) {
    JavaOnlyMap map = new JavaOnlyMap();
    map.putMap("map", (WritableMap) param);
    return map;
  }

  @LynxMethod
  public WritableArray getArray(ReadableArray param) {
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushArray((WritableArray) param);
    return array;
  }

  @LynxMethod
  public PiperData getPiperDataFromString(String json) {
    return PiperData.fromString(json);
  }

  @LynxMethod
  public PiperData getPiperDataFromObject(ReadableMap map) {
    return PiperData.fromObject(map);
  }

  @LynxMethod
  public void testAsyncCallBack(ReadableArray array, final Callback callback) {
    LynxThreadPool.getBriefIOExecutor().execute(() -> {
      if (callback != null) {
        callback.invoke(array);
      }
    });
  }

  @LynxMethod
  public void testSyncCallBack(ReadableArray array, final Callback callback) {
    if (callback != null) {
      callback.invoke(array);
    }
  }

  @LynxMethod
  public void testAsyncMultiCallBack(
      ReadableArray array, final Callback firstCallback, final Callback secondCallback) {
    LynxThreadPool.getBriefIOExecutor().execute(() -> {
      if (firstCallback != null) {
        firstCallback.invoke(array);
      }
      if (secondCallback != null) {
        secondCallback.invoke(array);
      }
    });
  }

  @LynxMethod
  public void testSyncMultiCallBack(
      ReadableArray array, final Callback firstCallback, final Callback secondCallback) {
    if (firstCallback != null) {
      firstCallback.invoke(array);
    }
    if (secondCallback != null) {
      secondCallback.invoke(array);
    }
  }

  @LynxMethod
  public void testPromise(boolean shouldResolve, Promise promise) {
    if (shouldResolve) {
      promise.resolve("resolve");
    } else {
      promise.reject("1", "reject");
    }
  }

  @LynxMethod
  public void testAsyncCallbackWithPiperData(ReadableArray array, final Callback callback) {
    LynxThreadPool.getBriefIOExecutor().execute(() -> {
      if (callback != null) {
        callback.invoke(PiperData.fromObject(array));
      }
    });
  }

  @LynxMethod
  public String testSameNameMethod(String param) {
    return param;
  }

  @LynxMethod
  public int testSameNameMethod(int param) {
    return param;
  }

  @LynxMethod
  public boolean testSameNameMethod(boolean param) {
    return param;
  }

  @LynxMethod
  public WritableMap testSameNameMethod(ReadableMap param) {
    JavaOnlyMap map = new JavaOnlyMap();
    map.putMap("map", (WritableMap) param);
    return map;
  }

  @LynxMethod
  public void testSameNameMethod(
      String param1, int param2, boolean param3, final Callback callback) {
    LynxThreadPool.getBriefIOExecutor().execute(() -> {
      if (callback != null) {
        callback.invoke(param1, param2, param3);
      }
    });
  }
}
