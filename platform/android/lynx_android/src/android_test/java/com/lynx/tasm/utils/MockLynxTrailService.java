// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.utils;

import android.content.Context;
import androidx.annotation.NonNull;
import com.lynx.tasm.service.ILynxTrailService;
import java.util.Map;

public class MockLynxTrailService implements ILynxTrailService {
  private final Map<String, Object> mTrailMap;

  public MockLynxTrailService(Map<String, Object> map) {
    mTrailMap = map;
  }

  @Override
  public void initialize(Context context) {}

  @Override
  public String stringValueForTrailKey(@NonNull String key) {
    return (String) objectValueForTrailKey(key);
  }

  @Override
  public Object objectValueForTrailKey(@NonNull String key) {
    return mTrailMap.get(key);
  }

  @Override
  public Map<String, Object> getAllValues() {
    return mTrailMap;
  }
}
