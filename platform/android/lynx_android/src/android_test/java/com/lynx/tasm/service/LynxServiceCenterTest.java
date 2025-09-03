// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.service;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;

import android.app.Application;
import android.content.Context;
import androidx.annotation.NonNull;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.google.auto.service.AutoService;
import com.lynx.tasm.LynxViewBuilder;
import java.util.Map;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxServiceCenterTest {
  @AutoService(IServiceProvider.class)
  public static class TestService implements ILynxTrailService, ILynxTrailServiceExtension {
    public boolean isInitialized = false;

    @Override
    public void onInitialize(Context context) {
      isInitialized = true;
    }

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

    @Override
    public void parseLynxViewBuilder(@NonNull LynxViewBuilder builder) {}
  }

  @Test
  public void testOnInitialize() {
    Application app = mock(Application.class);
    TestService service = new TestService();
    LynxServiceCenter.inst().registerService(service);
    LynxServiceCenter.inst().initialize(app);
    assertTrue(service.isInitialized);
  }
}
