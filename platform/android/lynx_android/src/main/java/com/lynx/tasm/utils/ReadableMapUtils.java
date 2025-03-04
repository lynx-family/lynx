// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.text.TextUtils;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.shadow.text.TextHelper;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

// TOOD(huzhanbo.luc): Add ut for this utils
public class ReadableMapUtils {
  public static Map<String, String> ConvertReadableMapToStringStringMap(ReadableMap params) {
    HashMap<String, String> result = new HashMap<>();
    HashMap<String, Object> paramsMap = params.asHashMap();
    for (String key : paramsMap.keySet()) {
      Object value = paramsMap.get(key);
      if (value instanceof List) {
        result.put(key, TextUtils.join(",", (Iterable<Object>) value));
      } else if (value instanceof Number) {
        result.put(key, TextHelper.formatDoubleToStringManually(((Number) value).doubleValue()));
      } else if (value != null) {
        result.put(key, value.toString());
      }
    }
    return result;
  }
}
