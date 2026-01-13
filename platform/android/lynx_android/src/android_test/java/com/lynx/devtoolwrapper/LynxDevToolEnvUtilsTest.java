// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import static org.junit.Assert.*;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.devtoolwrapper.LynxDevToolEnvUtils;
import com.lynx.tasm.LynxEnvKey;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxDevToolEnvUtilsTest {
  private ILynxDevToolService mDevToolService = mock(ILynxDevToolService.class);
  private Map<String, Set<String>> mGroupEnvMap;
  private Map<String, Object> mEnvMap;

  @Before
  public void setUp() {
    doReturn(ILynxDevToolService.class).when(mDevToolService).getServiceClass();
    LynxServiceCenter.inst().registerService(mDevToolService);

    try {
      Field devtoolServiceField = LynxDevToolEnvUtils.class.getDeclaredField("DEVTOOL_SERVICE");
      devtoolServiceField.setAccessible(true);
      devtoolServiceField.set(null, mDevToolService);
    } catch (NoSuchFieldException | IllegalAccessException e) {
      fail("Failed to set DEVTOOL_SERVICE via reflection: " + e.getMessage());
    }

    mGroupEnvMap = new HashMap<>();
    mEnvMap = new HashMap<>();

    doAnswer(invocation -> {
      String key = invocation.getArgument(0);
      Object value = invocation.getArgument(1);
      mEnvMap.put(key, value);
      return null;
    })
        .when(mDevToolService)
        .setDevtoolEnv(anyString(), any(Object.class));
    doAnswer(invocation -> {
      String key = invocation.getArgument(0);
      Set<String> value = invocation.getArgument(1);
      mGroupEnvMap.put(key, value);
      return null;
    })
        .when(mDevToolService)
        .setDevtoolGroupEnv(anyString(), any(Set.class));
    doAnswer(invocation -> {
      String key = invocation.getArgument(0);
      Boolean defaultValue = invocation.getArgument(1);
      return mEnvMap.containsKey(key) ? (Boolean) mEnvMap.get(key) : defaultValue;
    })
        .when(mDevToolService)
        .getDevtoolBooleanEnv(anyString(), any(Boolean.class));
    doAnswer(invocation -> {
      String key = invocation.getArgument(0);
      Integer defaultValue = invocation.getArgument(1);
      return mEnvMap.containsKey(key) ? (Integer) mEnvMap.get(key) : defaultValue;
    })
        .when(mDevToolService)
        .getDevtoolIntEnv(anyString(), any(Integer.class));
    doAnswer(invocation -> {
      String key = invocation.getArgument(0);
      return mGroupEnvMap.get(key);
    })
        .when(mDevToolService)
        .getDevtoolGroupEnv(anyString());
  }

  @After
  public void tearDown() {
    LynxServiceCenter.inst().unregisterService(ILynxDevToolService.class);
  }

  @Test
  public void testSetAndGetBooleanEnv() {
    LynxDevToolEnvUtils.setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL, true);
    assertEquals(true, LynxDevToolEnvUtils.getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_DEVTOOL, false));
    assertEquals(false, (Boolean) LynxDevToolEnvUtils.getDevtoolEnv("test_boolean", false));
  }

  @Test
  public void testSetAndGetIntegerEnv() {
    LynxDevToolEnvUtils.setDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_V8, 1);
    assertEquals(1, LynxDevToolEnvUtils.getDevtoolEnv(LynxEnvKey.SP_KEY_ENABLE_V8, 0));
    assertEquals(0, LynxDevToolEnvUtils.getDevtoolEnv("test_int", 0));
  }

  @Test
  public void testSetAndGetGroupEnv() {
    Set<String> domains = new HashSet<>();
    domains.add(LynxEnvKey.SP_KEY_ENABLE_CDP_DOMAIN_DOM);

    LynxDevToolEnvUtils.setDevtoolEnv(LynxEnvKey.SP_KEY_ACTIVATED_CDP_DOMAINS, domains);
    Set<String> result = LynxDevToolEnvUtils.getDevtoolEnv(LynxEnvKey.SP_KEY_ACTIVATED_CDP_DOMAINS);
    assertEquals(domains, result);

    Set<String> emptyResult = LynxDevToolEnvUtils.getDevtoolEnv("test_group");
    assertEquals(0, emptyResult.size());
  }
}
