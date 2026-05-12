// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.extension;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.app.Application;
import android.content.Context;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.jsbridge.ParamWrapper;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.atomic.AtomicBoolean;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxEnvAutolinkTest {
  @Before
  public void setUp() throws Exception {
    resetLynxEnvForTest();
    resetLynxExtensionRegistryForTest();
    LynxServiceCenter.inst().unregisterAllService();
    LynxAutolinkGenerated.resetForTest();
  }

  @After
  public void tearDown() throws Exception {
    resetLynxEnvForTest();
    resetLynxExtensionRegistryForTest();
    LynxServiceCenter.inst().unregisterAllService();
    LynxAutolinkGenerated.resetForTest();
  }

  @Test
  public void testInitSetsUpGeneratedAutolinkGlobalEntry() throws Exception {
    Application application = ApplicationProvider.getApplicationContext();
    LynxAutolinkGenerated.enableForTest();

    LynxEnv.inst().init(application, null, null, null, null);

    assertEquals(1, LynxAutolinkGenerated.getSetupCount());
    assertSame(application, LynxAutolinkGenerated.getProviderContext());
    assertTrue(hasGlobalBehavior(LynxAutolinkGenerated.GENERATED_BEHAVIOR));
    assertNotNull(
        findWrapper(LynxEnv.inst().getModuleFactory(), LynxAutolinkGenerated.GENERATED_MODULE));
    LynxAutolinkGenerated.GeneratedService service =
        LynxServiceCenter.inst().getService(LynxAutolinkGenerated.GeneratedService.class);
    assertNotNull(service);
    assertTrue(service.mInitialized);
  }

  @Test
  public void testGeneratedAutolinkGlobalEntryIsIdempotent() {
    Application application = ApplicationProvider.getApplicationContext();
    LynxAutolinkGenerated.enableForTest();

    LynxEnv.inst().init(application, null, null, null, null);
    LynxAutolinkGenerated.setupGlobal(application);

    assertEquals(1, LynxAutolinkGenerated.getSetupCount());
  }

  @Test
  public void testMissingGeneratedAutolinkGlobalEntryDoesNotThrow() throws Exception {
    Application application = ApplicationProvider.getApplicationContext();

    LynxEnv.inst().init(application, null, null, null, null);
    setupAutolinkGlobalForTest(application, "missing.generated.extensions.LynxAutolinkGenerated");
  }

  private static void resetLynxEnvForTest() throws Exception {
    LynxEnv env = LynxEnv.inst();
    ((AtomicBoolean) getField(env, "hasInit")).set(false);
    ((Map<?, ?>) getField(env, "mBehaviorMap")).clear();
    setField(env, "mModuleFactory", null);
    setField(env, "mViewManagerBundle", null);
    setField(env, "mTemplateProvider", null);
    setField(env, "mLibraryLoader", null);
  }

  private static void resetLynxExtensionRegistryForTest() throws Exception {
    Field field = LynxExtensionRegistry.class.getDeclaredField("sRegisteredGlobalProviders");
    field.setAccessible(true);
    ((Set<?>) field.get(null)).clear();
  }

  private static boolean hasGlobalBehavior(String behaviorName) {
    for (Behavior behavior : LynxEnv.inst().getBehaviors()) {
      if (behaviorName.equals(behavior.getName())) {
        return true;
      }
    }
    return false;
  }

  private static ParamWrapper findWrapper(Object moduleFactory, String moduleName)
      throws Exception {
    Map<String, ParamWrapper> wrappers = getField(moduleFactory, "mWrappers");
    for (ParamWrapper wrapper : wrappers.values()) {
      if (moduleName.equals(wrapper.getName())) {
        return wrapper;
      }
    }
    return null;
  }

  private static void setupAutolinkGlobalForTest(Context context, String generatedClassName)
      throws Exception {
    Method method =
        LynxEnv.class.getDeclaredMethod("setupAutolinkGlobal", Context.class, String.class);
    method.setAccessible(true);
    method.invoke(LynxEnv.inst(), context, generatedClassName);
  }

  @SuppressWarnings("unchecked")
  private static <T> T getField(Object target, String fieldName) throws Exception {
    Class<?> type = target.getClass();
    while (type != null) {
      try {
        Field field = type.getDeclaredField(fieldName);
        field.setAccessible(true);
        return (T) field.get(target);
      } catch (NoSuchFieldException e) {
        type = type.getSuperclass();
      }
    }
    throw new NoSuchFieldException(fieldName);
  }

  private static void setField(Object target, String fieldName, Object value) throws Exception {
    Class<?> type = target.getClass();
    while (type != null) {
      try {
        Field field = type.getDeclaredField(fieldName);
        field.setAccessible(true);
        field.set(target, value);
        return;
      } catch (NoSuchFieldException e) {
        type = type.getSuperclass();
      }
    }
    throw new NoSuchFieldException(fieldName);
  }
}
