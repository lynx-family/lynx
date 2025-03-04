// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.utils;

import static org.junit.Assert.*;

import com.lynx.react.bridge.DynamicFromMap;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.image.ImageUtils;
import java.util.HashMap;
import org.junit.Test;

public class ImageUtilsTest {
  @Test
  public void parseLocalCache() {
    String key = "local-cache";
    JavaOnlyMap map = JavaOnlyMap.from(new HashMap<String, Object>());
    DynamicFromMap dynamic = new DynamicFromMap(map, key);
    map.put(key, "true");
    assertTrue(checkTrueValue(ImageUtils.parseLocalCache(dynamic)));
    map.put(key, "false");
    assertTrue(checkFalseValue(ImageUtils.parseLocalCache(dynamic)));
    map.put(key, true);
    assertTrue(checkTrueValue(ImageUtils.parseLocalCache(dynamic)));
    map.put(key, false);
    assertTrue(checkFalseValue(ImageUtils.parseLocalCache(dynamic)));
    map.put(key, "default");
    assertTrue(checkTrueValue(ImageUtils.parseLocalCache(dynamic)));
    map.put(key, "none");
    assertTrue(checkFalseValue(ImageUtils.parseLocalCache(dynamic)));
    map.put(key, "await");
    assertTrue(checkAwaitValue(ImageUtils.parseLocalCache(dynamic)));
  }

  private boolean checkTrueValue(ImageUtils.LocalCacheState localCacheState) {
    return localCacheState.mUseLocalCache && !localCacheState.mAwaitLocalCache;
  }

  private boolean checkFalseValue(ImageUtils.LocalCacheState localCacheState) {
    return !localCacheState.mUseLocalCache && !localCacheState.mAwaitLocalCache;
  }

  private boolean checkAwaitValue(ImageUtils.LocalCacheState localCacheState) {
    return localCacheState.mUseLocalCache && localCacheState.mAwaitLocalCache;
  }
}
