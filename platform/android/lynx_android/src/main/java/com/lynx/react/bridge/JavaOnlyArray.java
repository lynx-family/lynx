/**
 * Copyright (c) 2015-present, Facebook, Inc.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.react.bridge;

import android.util.Log;
import com.lynx.jsbridge.jsi.ILynxJSIObject;
import com.lynx.tasm.base.CalledByNative;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.json.JSONArray;

public class JavaOnlyArray
    extends ArrayList<Object> implements ReadableArray, WritableArray, NativeArrayInterface {
  private static final String TAG = "JavaOnlyArray";
  public static JavaOnlyArray from(List list) {
    return new JavaOnlyArray(list);
  }

  public static JavaOnlyArray of(Object... values) {
    return new JavaOnlyArray(Arrays.asList(values));
  }

  public static JavaOnlyArray deepClone(ReadableArray ary) {
    JavaOnlyArray res = new JavaOnlyArray();
    for (int i = 0, size = ary.size(); i < size; i++) {
      ReadableType type = ary.getType(i);
      switch (type) {
        case Null:
          res.pushNull();
          break;
        case Boolean:
          res.pushBoolean(ary.getBoolean(i));
          break;
        case Int:
          res.pushInt(ary.getInt(i));
          break;
        case Long:
          res.pushLong(ary.getLong(i));
          break;
        case Number:
          res.pushDouble(ary.getDouble(i));
          break;
        case String:
          res.pushString(ary.getString(i));
          break;
        case Map:
          res.pushMap(JavaOnlyMap.deepClone(ary.getMap(i)));
          break;
        case Array:
          res.pushArray(deepClone(ary.getArray(i)));
          break;
        case ByteArray:
          res.pushByteArray(ary.getByteArray(i).clone());
          break;
      }
    }
    return res;
  }

  public static JavaOnlyArray shallowCopy(ReadableArray ary) {
    JavaOnlyArray res = new JavaOnlyArray();
    for (int i = 0, size = ary.size(); i < size; i++) {
      ReadableType type = ary.getType(i);
      switch (type) {
        case Null:
          res.pushNull();
          break;
        case Boolean:
          res.pushBoolean(ary.getBoolean(i));
          break;
        case Int:
          res.pushInt(ary.getInt(i));
          break;
        case Long:
          res.pushLong(ary.getLong(i));
          break;
        case Number:
          res.pushDouble(ary.getDouble(i));
          break;
        case String:
          res.pushString(ary.getString(i));
          break;
        case Map:
          res.pushMap((WritableMap) ary.getMap(i));
          break;
        case Array:
          res.pushArray((WritableArray) ary.getArray(i));
          break;
        case ByteArray:
          res.pushByteArray(ary.getByteArray(i));
          break;
      }
    }
    return res;
  }

  protected JavaOnlyArray(List list) {
    super(list);
  }

  public JavaOnlyArray() {
    super();
  }

  @Override
  @CalledByNative
  public int size() {
    return super.size();
  }

  @Override
  public boolean isNull(int index) {
    return this.get(index) == null;
  }

  @Override
  @CalledByNative
  public double getDouble(int index) {
    return ((Number) this.get(index)).doubleValue();
  }

  @Override
  public byte getByte(int index) {
    return ((Number) this.get(index)).byteValue();
  }

  @Override
  public short getShort(int index) {
    return ((Number) this.get(index)).shortValue();
  }

  @Override
  @CalledByNative
  public long getLong(int index) {
    return ((Number) this.get(index)).longValue();
  }

  @Override
  public char getChar(int index) {
    return (char) ((Number) this.get(index)).shortValue();
  }

  @Override
  @CalledByNative
  public int getInt(int index) {
    return ((Number) this.get(index)).intValue();
  }

  @Override
  @CalledByNative
  public String getString(int index) {
    return (String) this.get(index);
  }

  @Override
  @CalledByNative
  public JavaOnlyArray getArray(int index) {
    return (JavaOnlyArray) this.get(index);
  }

  @Override
  @CalledByNative
  public boolean getBoolean(int index) {
    return (Boolean) this.get(index);
  }

  @Override
  @CalledByNative
  public JavaOnlyMap getMap(int index) {
    return (JavaOnlyMap) this.get(index);
  }

  @Override
  @CalledByNative
  public byte[] getByteArray(int index) {
    return (byte[]) this.get(index);
  }

  @Override
  @CalledByNative
  public PiperData getPiperData(int index) {
    return (PiperData) this.get(index);
  }

  @Override
  public Dynamic getDynamic(int index) {
    return new DynamicFromArray(this, index);
  }

  @CalledByNative
  public Object getObject(int index) {
    return this.get(index);
  }

  @Override
  public ReadableType getType(int index) {
    Object object = this.get(index);

    if (object == null) {
      return ReadableType.Null;
    } else if (object instanceof Boolean) {
      return ReadableType.Boolean;
    } else if (object instanceof Integer) {
      return ReadableType.Int;
    } else if (object instanceof Long) {
      return ReadableType.Long;
    } else if (object instanceof Number || object instanceof Character) {
      return ReadableType.Number;
    } else if (object instanceof String) {
      return ReadableType.String;
    } else if (object instanceof ReadableArray) {
      return ReadableType.Array;
    } else if (object instanceof ReadableMap) {
      return ReadableType.Map;
    } else if (object instanceof byte[]) {
      return ReadableType.ByteArray;
    } else if (object instanceof PiperData) {
      return ReadableType.PiperData;
    } else if (object instanceof ILynxJSIObject) {
      return ReadableType.LynxObject;
    } else {
      throw new IllegalArgumentException(
          "unsupported type " + object.getClass() + " contained in JavaOnlyArray");
    }
  }

  @CalledByNative
  public int getTypeIndex(int index) {
    return getType(index).ordinal();
  }

  @Override
  @CalledByNative
  public void pushBoolean(boolean value) {
    this.add(value);
  }

  @Override
  @CalledByNative
  public void pushDouble(double value) {
    this.add(value);
  }

  @Override
  @CalledByNative
  public void pushInt(int value) {
    this.add(value);
  }

  @Override
  @CalledByNative
  public void pushLong(long value) {
    this.add(value);
  }

  @Override
  @CalledByNative
  public void pushString(String value) {
    this.add(value);
  }

  @Override
  @CalledByNative
  public void pushArray(WritableArray array) {
    this.add(array);
  }

  @Override
  @CalledByNative
  public void pushMap(WritableMap map) {
    this.add(map);
  }

  @Override
  @CalledByNative
  public void pushNull() {
    this.add(null);
  }

  @Override
  @CalledByNative
  public void pushByteArray(byte[] array) {
    this.add(array);
  }

  @CalledByNative
  public void pushByteArrayAsString(byte[] array) {
    this.add(new String(array));
  }

  @Override
  public void pushPiperData(PiperData json) {
    this.add(json);
  }

  @CalledByNative
  public void clear() {
    super.clear();
  }

  // Convert JavaOnlyArray to JSONArray
  public JSONArray toJSONArray() {
    JSONArray jsonArray = new JSONArray();
    for (int i = 0, size = this.size(); i < size; i++) {
      Object obj = this.get(i);
      try {
        if (this.getType(i) == ReadableType.Map) {
          JavaOnlyMap map = (JavaOnlyMap) obj;
          jsonArray.put(map.toJSONObject());
        } else if (this.getType(i) == ReadableType.Array) {
          JavaOnlyArray array = (JavaOnlyArray) obj;
          jsonArray.put(array.toJSONArray());
        } else {
          jsonArray.put(obj);
        }
      } catch (Exception e) {
        Log.e(TAG, e.toString());
      }
    }
    return jsonArray;
  }

  @Override
  @Deprecated
  public ArrayList<Object> toArrayList() {
    return new ArrayList<Object>(this);
  }

  @Override
  public ArrayList<Object> asArrayList() {
    return this;
  }

  @CalledByNative
  public Object getAt(int index) {
    return this.get(index);
  }

  @CalledByNative
  private static JavaOnlyArray create() {
    return new JavaOnlyArray();
  }

  @CalledByNative
  private JavaOnlyArray shallowCopy() {
    return shallowCopy(this);
  }
}
