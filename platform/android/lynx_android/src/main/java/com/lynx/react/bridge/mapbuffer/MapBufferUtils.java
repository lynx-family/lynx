// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.react.bridge.mapbuffer;

import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.PropertyIDConstants;
import java.util.Iterator;

public class MapBufferUtils {
  /**
   * Helper Method to Convert Style PropsBundle to ReadableMap.
   */
  public static ReadableMap convertStyleMapBufferToReadableMap(ReadableMapBuffer mapBuffer) {
    JavaOnlyMap result = new JavaOnlyMap();
    if (mapBuffer != null) {
      Iterator<MapBuffer.Entry> iter = mapBuffer.iterator();
      while (iter.hasNext()) {
        MapBuffer.Entry entry = iter.next();
        int key = entry.getKey();
        MapBuffer.DataType type = entry.getType();
        String propertyName = PropertyIDConstants.PROPERTY_CONSTANT[key];
        switch (type) {
          case ARRAY:
            ReadableArray array = new ReadableMapBufferWrapper(entry.getMapBuffer());
            result.put(propertyName, array);
            break;
          case INT:
            result.put(propertyName, entry.getInt());
            break;
          case BOOL:
            result.put(propertyName, entry.getBoolean());
            break;
          case LONG:
            result.put(propertyName, entry.getLong());
            break;
          case STRING:
            result.put(propertyName, entry.getString());
            break;
          case DOUBLE:
            result.put(propertyName, entry.getDouble());
            break;
          case NULL:
            result.put(propertyName, null);
            break;
        }
      }
    }
    return result;
  }
}
