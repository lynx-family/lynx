// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.explorer.utils;

import android.net.Uri;
import android.util.Log;
import java.util.HashMap;

/*
  eg:
    1. If your URL does not have any query, then you will get an empty map.
      For example:
        URL: https://www.example.com/
        Map: {} (empty map)
    2. If url is 'https://your.api.com/path/?key1=value1&key2=value2&key3=value3'
       you will get a Map, which contains
       {
         'key1':'value1',
         'key2':'value2',
         'key3':'value3'
       }
*/

public class QueryMapUtils {
  private final String TAG = this.getClass().toString();
  private HashMap<String, String> mQuery;

  public QueryMapUtils() {
    mQuery = new HashMap<String, String>();
  }

  public void parse(String url) {
    String queryString = getQueryStringFromUrl(url);
    saveQueryToMap(queryString);
  }

  private void saveQueryToMap(String query) {
    if (query == null || query.isEmpty()) {
      Log.i(TAG, "The URL does not contain any query information.");
      return;
    }
    String[] params = query.split("&");
    for (String param : params) {
      String[] splitResult = param.split("=");
      if (splitResult.length >= 2) {
        String name = splitResult[0];
        String value = splitResult[1];
        if (value.startsWith("http")) {
          value = Uri.decode(value);
        }
        mQuery.put(name, value);
      }
    }
  }

  private String getQueryStringFromUrl(String url) {
    if (url.startsWith("file://lynx?local://automation/")) {
      url = url.substring("file://lynx?".length());
    }
    if (url.contains("?")) {
      String queryString = url.split("\\?")[1];
      return queryString;
    } else {
      return null;
    }
  }

  public HashMap<String, String> toMap() {
    HashMap<String, String> copyQueryMap = new HashMap<String, String>();
    copyQueryMap.putAll(mQuery);
    return copyQueryMap;
  }

  public boolean getBoolean(String key, boolean defaultValue) {
    String val = mQuery.get(key);
    if (val == null) {
      return defaultValue;
    }
    try {
      return Boolean.parseBoolean(val);
    } catch (Throwable e) {
      Log.e(TAG, "The query value cannot be converted to a boolean type.");
      return defaultValue;
    }
  }

  public int getInt(String key, int defaultValue) {
    String val = mQuery.get(key);
    if (val == null) {
      return defaultValue;
    }
    try {
      return Integer.parseInt(val);
    } catch (Throwable e) {
      Log.e(TAG, "The query value cannot be converted to a Int type.");
      return defaultValue;
    }
  }

  public float getFloat(String key, float defaultValue) {
    if (mQuery.containsKey(key)) {
      String val = mQuery.get(key);
      if (val == null) {
        return defaultValue;
      }
      try {
        return Float.parseFloat(val);
      } catch (Throwable e) {
        return defaultValue;
      }
    }
    return defaultValue;
  }

  public String getString(String key) {
    String val = mQuery.get(key);
    return val;
  }

  public boolean contains(String key) {
    return mQuery.containsKey(key);
  }
}
