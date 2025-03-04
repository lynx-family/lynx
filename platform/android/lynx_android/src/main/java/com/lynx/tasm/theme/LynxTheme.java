// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.theme;

import com.lynx.tasm.common.LepusBuffer;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Set;

/**
 * @deprecated The injection of theme resources using LynxTheme is deprecated.
 * It is recommended to use a front-end solution to achieve theme switching.
 */
@Deprecated
public class LynxTheme {
  public boolean update(String key, String value) {
    if (key == null || key.isEmpty() || key.startsWith("__")) {
      return false;
    }
    if (value == null) {
      mMap.remove(key);
    } else {
      mMap.put(key, value);
    }
    ++mChangeCount;
    return true;
  }

  public String get(String key) {
    if (key == null)
      return null;
    return mMap.get(key);
  }
  public Set<String> keySet() {
    return mMap.keySet();
  }
  public int changeCount() {
    return mChangeCount;
  }

  public LynxTheme() {
    mMap = new HashMap<String, String>();
    mChangeCount = 0;
  }

  public boolean replaceWithTheme(LynxTheme theme) {
    if (theme == null)
      return false;
    mMap = theme.mMap;
    ++mChangeCount;
    return true;
  }

  public void addToHashMap(HashMap dst, String key) {
    dst.put(key, mMap);
  }

  private HashMap<String, String> mMap;
  private int mChangeCount;
}
