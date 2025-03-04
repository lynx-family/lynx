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
import com.lynx.tasm.base.CalledByNative;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import org.json.JSONObject;

public class JavaOnlyMap extends HashMap<String, Object> implements ReadableMap, WritableMap {
  private static final String TAG = "JavaOnlyMap";
  public static JavaOnlyMap from(Map map) {
    return new JavaOnlyMap(map);
  }

  private JavaOnlyMap(Map map) {
    super(map);
  }
  public JavaOnlyMap() {
    super();
  }

  public static JavaOnlyMap deepClone(ReadableMap map) {
    JavaOnlyMap res = new JavaOnlyMap();
    ReadableMapKeySetIterator iter = map.keySetIterator();
    while (iter.hasNextKey()) {
      String propKey = iter.nextKey();
      ReadableType type = map.getType(propKey);
      switch (type) {
        case Null:
          res.putNull(propKey);
          break;
        case Boolean:
          res.putBoolean(propKey, map.getBoolean(propKey));
          break;
        case Int:
          res.putInt(propKey, map.getInt(propKey));
          break;
        case Long:
          res.putLong(propKey, map.getLong(propKey));
          break;
        case Number:
          res.putDouble(propKey, map.getDouble(propKey));
          break;
        case String:
          res.putString(propKey, map.getString(propKey));
          break;
        case Map:
          res.putMap(propKey, deepClone(map.getMap(propKey)));
          break;
        case Array:
          res.putArray(propKey, JavaOnlyArray.deepClone(map.getArray(propKey)));
          break;
        case ByteArray:
          res.putByteArray(propKey, map.getByteArray(propKey).clone());
      }
    }
    return res;
  }

  public static JavaOnlyMap shallowCopy(ReadableMap map) {
    JavaOnlyMap res = new JavaOnlyMap();
    ReadableMapKeySetIterator iter = map.keySetIterator();
    while (iter.hasNextKey()) {
      String propKey = iter.nextKey();
      ReadableType type = map.getType(propKey);
      switch (type) {
        case Null:
          res.putNull(propKey);
          break;
        case Boolean:
          res.putBoolean(propKey, map.getBoolean(propKey));
          break;
        case Int:
          res.putInt(propKey, map.getInt(propKey));
          break;
        case Long:
          res.putLong(propKey, map.getLong(propKey));
          break;
        case Number:
          res.putDouble(propKey, map.getDouble(propKey));
          break;
        case String:
          res.putString(propKey, map.getString(propKey));
          break;
        case Map:
          res.putMap(propKey, (WritableMap) map.getMap(propKey));
          break;
        case Array:
          res.putArray(propKey, (WritableArray) map.getArray(propKey));
          break;
        case ByteArray:
          res.putByteArray(propKey, map.getByteArray(propKey));
      }
    }
    return res;
  }

  @Override
  public boolean hasKey(String name) {
    return containsKey(name);
  }

  @Override
  public boolean isNull(String name) {
    return get(name) == null;
  }

  @Override
  @CalledByNative
  public boolean getBoolean(String name) {
    Object obj = get(name);
    if (obj instanceof Boolean) {
      return (Boolean) obj;
    }
    if (obj instanceof String) {
      return ((String) obj).equalsIgnoreCase("true") || ((String) obj).equalsIgnoreCase("yes");
    }

    if (obj instanceof Number) {
      return ((Number) obj).intValue() != 0;
    }

    if (obj == null) {
      throw new NullPointerException("key: " + name);
    }
    throw new ClassCastException(obj.getClass().getName()
        + " cannot be cast to java.lang.Boolean, key: " + name + ", value: " + obj);
  }

  @Override
  @CalledByNative
  public double getDouble(String name) {
    Object obj = get(name);
    if (obj instanceof Number) {
      return ((Number) obj).doubleValue();
    }

    if (obj instanceof Boolean) {
      return ((Boolean) obj) ? 1 : 0;
    }

    if (obj instanceof String) {
      try {
        return Double.parseDouble(((String) obj));
      } catch (NumberFormatException e) {
        throw new IllegalArgumentException(
            "String cannot be cast to double, key: " + name + ", value: " + obj);
      }
    }

    if (obj == null) {
      throw new NullPointerException("key: " + name);
    }
    throw new ClassCastException(obj.getClass().getName()
        + " cannot be cast to java.lang.Number, key: " + name + ", value: " + obj);
  }

  @Override
  @CalledByNative
  public long getLong(String name) {
    Object obj = get(name);
    if (obj instanceof Number) {
      return ((Number) obj).longValue();
    }

    if (obj instanceof Boolean) {
      return ((Boolean) obj) ? 1 : 0;
    }

    if (obj instanceof String) {
      try {
        return Long.parseLong(((String) obj));
      } catch (NumberFormatException e) {
        throw new IllegalArgumentException(
            "String cannot be cast to long int, key: " + name + ", value: " + obj);
      }
    }

    if (obj == null) {
      throw new NullPointerException("key: " + name);
    }
    throw new ClassCastException(obj.getClass().getName()
        + " cannot be cast to java.lang.Long, key: " + name + ", value: " + obj);
  }

  @Override
  @CalledByNative
  public int getInt(String name) {
    Object obj = get(name);
    if (obj instanceof Number) {
      return ((Number) obj).intValue();
    }

    if (obj instanceof Boolean) {
      return ((Boolean) obj) ? 1 : 0;
    }

    if (obj instanceof String) {
      try {
        return Integer.parseInt(((String) obj));
      } catch (NumberFormatException e) {
        throw new IllegalArgumentException(
            "String cannot be cast to int, key: " + name + ", value: " + obj);
      }
    }

    if (obj == null) {
      throw new NullPointerException("key: " + name);
    }
    throw new ClassCastException(obj.getClass().getName()
        + " cannot be cast to java.lang.Number, key: " + name + ", value: " + obj);
  }

  @Override
  @CalledByNative
  public String getString(String name) {
    Object obj = get(name);
    if (obj instanceof String) {
      return (String) obj;
    }
    if (obj instanceof Boolean) {
      return ((Boolean) obj) ? "true" : "false";
    }

    if (obj instanceof Number) {
      return ((Number) obj).toString();
    }

    if (obj == null) {
      return null;
    }
    throw new ClassCastException(obj.getClass().getName()
        + " cannot be cast to java.lang.String, key: " + name + ", value: " + obj);
  }

  @Override
  @CalledByNative
  public ReadableMap getMap(String name) {
    Object obj = get(name);
    if (obj instanceof ReadableMap) {
      return (ReadableMap) obj;
    }
    if (obj == null) {
      return null;
    }
    throw new ClassCastException(obj.getClass().getName() + " cannot be cast to "
        + ReadableMap.class.getName() + ", key: " + name + ", value: " + obj);
  }

  @Override
  public boolean getBoolean(String name, boolean defaultValue) {
    Object result = get(name);
    if (result instanceof Boolean) {
      return (Boolean) result;
    }

    if (result instanceof Number) {
      return ((Number) result).intValue() != 0;
    }

    if (result instanceof String) {
      if (((String) result).equalsIgnoreCase("true") || ((String) result).equalsIgnoreCase("yes")) {
        return true;
      }
      if (((String) result).equalsIgnoreCase("false") || ((String) result).equalsIgnoreCase("no")) {
        return false;
      }
    }
    return defaultValue;
  }

  @Override
  public double getDouble(String name, double defaultValue) {
    Object result = get(name);
    if (result instanceof Number) {
      return ((Number) result).doubleValue();
    }

    if (result instanceof Boolean) {
      return ((Boolean) result) ? 1 : 0;
    }

    if (result instanceof String) {
      try {
        return Double.parseDouble(((String) result));
      } catch (NumberFormatException e) {
        e.printStackTrace();
      }
    }

    return defaultValue;
  }

  @Override
  public int getInt(String name, int defaultValue) {
    Object result = get(name);
    if (result instanceof Number) {
      return ((Number) result).intValue();
    }

    if (result instanceof Boolean) {
      return ((Boolean) result) ? 1 : 0;
    }

    if (result instanceof String) {
      try {
        return Integer.parseInt(((String) result));
      } catch (NumberFormatException e) {
        e.printStackTrace();
      }
    }

    return defaultValue;
  }

  @Override
  public long getLong(String name, long defaultValue) {
    Object result = get(name);
    if (result instanceof Number) {
      return ((Number) result).longValue();
    }

    if (result instanceof Boolean) {
      return ((Boolean) result) ? 1 : 0;
    }

    if (result instanceof String) {
      try {
        return Long.parseLong(((String) result));
      } catch (NumberFormatException e) {
        e.printStackTrace();
      }
    }

    return defaultValue;
  }

  @Override
  public String getString(String name, String defaultValue) {
    Object result = get(name);
    if (result instanceof String) {
      return (String) result;
    }
    if (result instanceof Boolean) {
      return ((Boolean) result) ? "true" : "false";
    }

    if (result instanceof Number) {
      return ((Number) result).toString();
    }

    return defaultValue;
  }

  @Override
  public ReadableArray getArray(String name, ReadableArray defaultValue) {
    Object result = get(name);
    if (result instanceof ReadableArray) {
      return (ReadableArray) result;
    }
    return defaultValue;
  }

  @Override
  public ReadableMap getMap(String name, ReadableMap defaultValue) {
    Object result = get(name);
    if (result instanceof ReadableMap) {
      return (ReadableMap) result;
    }
    return defaultValue;
  }

  @CalledByNative
  ArrayList<String> getKeys() {
    return new ArrayList<>(keySet());
  }

  @Override
  @CalledByNative
  public ReadableArray getArray(String name) {
    Object obj = get(name);
    if (obj instanceof ReadableArray) {
      return (ReadableArray) obj;
    }
    if (obj == null) {
      return null;
    }
    throw new ClassCastException(obj.getClass().getName() + " cannot be cast to "
        + ReadableArray.class.getName() + ", key: " + name + ", value: " + obj);
  }

  @Override
  public byte[] getByteArray(String name, byte[] defaultValue) {
    Object result = get(name);
    if (result instanceof byte[]) {
      return (byte[]) result;
    }
    return defaultValue;
  }

  @Override
  @CalledByNative
  public byte[] getByteArray(String name) {
    Object obj = get(name);
    if (obj instanceof byte[]) {
      return (byte[]) obj;
    }
    if (obj == null) {
      return null;
    }
    throw new ClassCastException(obj.getClass().getName() + " cannot be cast to "
        + byte[].class.getName() + ", key: " + name + ", value: " + obj);
  }

  @Override
  public PiperData getPiperData(String name, PiperData defaultValue) {
    Object result = get(name);
    if (result instanceof PiperData) {
      return (PiperData) result;
    }
    return defaultValue;
  }

  @Override
  @CalledByNative
  public PiperData getPiperData(String name) {
    Object obj = get(name);
    if (obj instanceof PiperData) {
      return (PiperData) obj;
    }
    if (obj == null) {
      return null;
    }
    throw new ClassCastException(obj.getClass().getName() + " cannot be cast to "
        + PiperData.class.getName() + ", key: " + name);
  }

  @Override
  public Dynamic getDynamic(String name) {
    return new DynamicFromMap(this, name);
  }

  @Override
  public ReadableType getType(String name) {
    Object value = get(name);
    if (value == null) {
      return ReadableType.Null;
    } else if (value instanceof Integer) {
      return ReadableType.Int;
    } else if (value instanceof Long) {
      return ReadableType.Long;
    } else if (value instanceof Number || value instanceof Character) {
      return ReadableType.Number;
    } else if (value instanceof String) {
      return ReadableType.String;
    } else if (value instanceof Boolean) {
      return ReadableType.Boolean;
    } else if (value instanceof ReadableMap) {
      return ReadableType.Map;
    } else if (value instanceof ReadableArray) {
      return ReadableType.Array;
    } else if (value instanceof byte[]) {
      return ReadableType.ByteArray;
    } else if (value instanceof PiperData) {
      return ReadableType.PiperData;
    } else if (value instanceof Dynamic) {
      return ((Dynamic) value).getType();
    } else {
      throw new IllegalArgumentException(
          "Invalid value " + value.toString() + " for key " + name + "contained in JavaOnlyMap");
    }
  }

  @CalledByNative
  public int getTypeIndex(String name) {
    return getType(name).ordinal();
  }

  @Override
  public ReadableMapKeySetIterator keySetIterator() {
    return new ReadableMapKeySetIterator() {
      Iterator<String> mIterator = keySet().iterator();

      @Override
      public boolean hasNextKey() {
        return mIterator.hasNext();
      }

      @Override
      public String nextKey() {
        return mIterator.next();
      }
    };
  }

  @Override
  @CalledByNative
  public void putBoolean(String key, boolean value) {
    put(key, value);
  }

  @Override
  @CalledByNative
  public void putDouble(String key, double value) {
    put(key, value);
  }

  @Override
  @CalledByNative
  public void putInt(String key, int value) {
    put(key, value);
  }

  @Override
  @CalledByNative
  public void putLong(String key, long value) {
    put(key, value);
  }

  @Override
  @CalledByNative
  public void putString(String key, String value) {
    put(key, value);
  }

  @Override
  @CalledByNative
  public void putNull(String key) {
    put(key, null);
  }

  @Override
  @CalledByNative
  public void putMap(String key, WritableMap value) {
    put(key, value);
  }

  @Override
  @CalledByNative
  public int size() {
    return super.size();
  }

  @Override
  public void merge(ReadableMap source) {
    putAll(source.asHashMap());
  }

  @Override
  @CalledByNative
  public void putArray(String key, WritableArray value) {
    put(key, value);
  }

  @Override
  @CalledByNative
  public void putByteArrayAsString(byte[] key, byte[] value) {
    put(new String(key), new String(value));
  }

  @Override
  @CalledByNative
  public void putByteArray(String key, byte[] value) {
    put(key, value);
  }

  @Override
  public void putPiperData(String key, PiperData value) {
    put(key, value);
  }

  @Override
  @Deprecated
  public HashMap<String, Object> toHashMap() {
    return new HashMap<String, Object>(this);
  }

  @Override
  public HashMap<String, Object> asHashMap() {
    return this;
  }

  // Convert JavaOnlyMap to JSONObject
  public JSONObject toJSONObject() {
    JSONObject resultJson = new JSONObject();
    if (this.size() == 0) {
      return resultJson;
    }
    Iterator it = this.keySet().iterator();
    while (it.hasNext()) {
      String nextKey = (String) it.next();
      Object nextValue = this.get(nextKey);
      try {
        if (this.getType(nextKey) == ReadableType.Map) {
          JavaOnlyMap map = (JavaOnlyMap) nextValue;
          resultJson.putOpt(nextKey, map.toJSONObject());
        } else if (this.getType(nextKey) == ReadableType.Array) {
          JavaOnlyArray array = (JavaOnlyArray) nextValue;
          resultJson.putOpt(nextKey, array.toJSONArray());
        } else if (this.getType(nextKey) == ReadableType.Null) {
          resultJson.putOpt(nextKey, JSONObject.NULL);
        } else {
          resultJson.putOpt(nextKey, nextValue);
        }
      } catch (Exception e) {
        Log.e(TAG, e.toString());
      }
    }
    return resultJson;
  }

  @CalledByNative
  private static JavaOnlyMap create() {
    return new JavaOnlyMap();
  }

  @CalledByNative
  private JavaOnlyMap shallowCopy() {
    return shallowCopy(this);
  }
}
