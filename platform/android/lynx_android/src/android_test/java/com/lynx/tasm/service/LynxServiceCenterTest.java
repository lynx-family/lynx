// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import static org.junit.Assert.*;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.google.auto.service.AutoService;
import java.lang.reflect.Method;
import java.util.Map;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxServiceCenterTest {
  @AutoService(IServiceProvider.class)
  public static class TestService implements ILynxTrailService {
    @Override
    public void initialize(Context context) {}
    @Override
    public String stringValueForTrailKey(@NonNull String key) {
      return "test";
    }
    @Override
    public Object objectValueForTrailKey(@NonNull String key) {
      return null;
    }
    @Override
    public Map<String, Object> getAllValues() {
      return null;
    }
  }

  @Test
  public void testAutoRegister() {
    LynxServiceCenter.inst().unregisterAllService();
    Assert.assertNull(LynxServiceCenter.inst().getService(ILynxTrailService.class));

    try {
      Method doInitialize = LynxServiceCenter.class.getDeclaredMethod("doInitialize");
      doInitialize.setAccessible(true);
      doInitialize.invoke(LynxServiceCenter.inst());
    } catch (Throwable e) {
      fail(e.toString());
    }
    ILynxTrailService service = LynxServiceCenter.inst().getService(ILynxTrailService.class);
    Assert.assertNotNull(service);
    Assert.assertEquals(service.stringValueForTrailKey("test"), "test");
    LynxServiceCenter.inst().unregisterAllService();
  }
}
