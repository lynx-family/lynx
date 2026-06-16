// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.After;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class SparklingRuntimeSmokeTest {
  private final AtomicReference<Object> sparklingViewRef = new AtomicReference<>();

  @After
  public void tearDown() {
    final Object sparklingView = sparklingViewRef.getAndSet(null);
    if (sparklingView != null) {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        try {
          sparklingView.getClass().getMethod("release").invoke(sparklingView);
        } catch (Exception exception) {
          throw new AssertionError("Failed to release SparklingView", exception);
        }
      });
    }
  }

  @Test
  public void enabledVariantCreatesSparklingBridgeAndContainerGlobalProps() throws Throwable {
    if (!BuildConfig.ENABLE_SPARKLING) {
      return;
    }

    Context targetContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
    ClassLoader classLoader = targetContext.getClassLoader();
    Class<?> sparklingClass = classLoader.loadClass("com.tiktok.sparkling.Sparkling");
    Class<?> sparklingContextClass = classLoader.loadClass("com.tiktok.sparkling.SparklingContext");
    assertNotNull(classLoader.loadClass("com.tiktok.sparkling.SparklingActivity"));
    assertNotNull(classLoader.loadClass("com.tiktok.sparkling.hybridkit.HybridKit"));

    Object sparklingContext = sparklingContextClass.getConstructor().newInstance();
    sparklingContextClass.getMethod("setScheme", String.class)
        .invoke(sparklingContext,
            "hybrid://lynxview_page?bundle=homepage.lynx.bundle&hide_loading=1&hide_nav_bar=1");

    Method buildMethod = sparklingClass.getMethod("build", Context.class, sparklingContextClass);
    Object sparkling = buildMethod.invoke(null, targetContext, sparklingContext);
    sparklingClass.getMethod("processSparklingContext", sparklingContextClass)
        .invoke(sparkling, sparklingContext);

    String containerId =
        (String) sparklingContextClass.getMethod("getContainerId").invoke(sparklingContext);
    assertNotNull(containerId);
    assertFalse(containerId.trim().isEmpty());

    Object schemeParam =
        sparklingContextClass.getMethod("getHybridSchemeParam").invoke(sparklingContext);
    assertNotNull(schemeParam);
    assertEquals(
        "homepage.lynx.bundle", schemeParam.getClass().getMethod("getBundle").invoke(schemeParam));

    AtomicReference<Throwable> failureRef = new AtomicReference<>();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      try {
        Object sparklingView =
            sparklingClass.getMethod("createView", boolean.class).invoke(sparkling, false);
        assertNotNull(sparklingView);
        assertSame(sparklingContext,
            sparklingView.getClass().getMethod("getSparklingContext").invoke(sparklingView));
        assertSame(sparklingContext,
            sparklingView.getClass().getMethod("obtainHybridContext").invoke(sparklingView));
        assertTrue(
            sparklingView.getClass().getMethod("actualView").invoke(sparklingView) instanceof View);
        sparklingViewRef.set(sparklingView);
      } catch (Throwable throwable) {
        failureRef.set(throwable);
      }
    });

    if (failureRef.get() != null) {
      throw failureRef.get();
    }

    assertNotNull(sparklingContextClass.getMethod("getBridge").invoke(sparklingContext));
    Map<String, Object> globalProps = getSparklingGlobalProps(classLoader, containerId);
    assertEquals(containerId, globalProps.get("containerID"));
    assertEquals(Boolean.TRUE, globalProps.get("sparklingAvailable"));
    assertEquals(Boolean.TRUE, globalProps.get("sparklingNavigation"));
    assertNotNull(globalProps.get("queryItems"));
    assertRouterOpenRegistered(classLoader);
    assertRouterOpenBridgeCallSucceeds(
        classLoader, sparklingContext, sparklingContextClass, containerId);
  }

  @Test
  public void disabledVariantDoesNotExposeSparklingRuntime() throws Exception {
    if (BuildConfig.ENABLE_SPARKLING) {
      return;
    }

    Context targetContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
    try {
      targetContext.getClassLoader().loadClass("com.tiktok.sparkling.SparklingActivity");
      fail("SparklingActivity should not be loadable in a disabled Explorer build.");
    } catch (ClassNotFoundException expected) {
      // Expected: disabled builds must not package Sparkling runtime classes.
    }

    PackageInfo packageInfo = targetContext.getPackageManager().getPackageInfo(
        targetContext.getPackageName(), PackageManager.GET_ACTIVITIES);
    if (packageInfo.activities != null) {
      for (ActivityInfo activityInfo : packageInfo.activities) {
        assertNotEquals("com.tiktok.sparkling.SparklingActivity", activityInfo.name);
      }
    }
  }

  @Test
  public void disabledVariantLaunchesExplorerActivityWithoutSparkling() {
    if (BuildConfig.ENABLE_SPARKLING) {
      return;
    }

    Context targetContext = InstrumentationRegistry.getInstrumentation().getTargetContext();
    Intent intent = new Intent(targetContext, LynxViewShellActivity.class);
    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
    intent.putExtra("lynx_initial_url", "file://lynx?local://homepage.lynx.bundle");

    Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
    Activity activity = instrumentation.startActivitySync(intent);
    try {
      instrumentation.waitForIdleSync();
      assertFalse(activity.isFinishing());
    } finally {
      activity.finish();
    }
  }

  @SuppressWarnings("unchecked")
  private static Map<String, Object> getSparklingGlobalProps(
      ClassLoader classLoader, String containerId) throws Exception {
    Class<?> globalPropsUtilsClass =
        classLoader.loadClass("com.tiktok.sparkling.hybridkit.utils.GlobalPropsUtils");
    Field companionField = globalPropsUtilsClass.getField("Companion");
    Object companion = companionField.get(null);
    Object globalPropsUtils = companion.getClass().getMethod("getInstance").invoke(companion);
    return (Map<String, Object>) globalPropsUtilsClass.getMethod("getGlobalProps", String.class)
        .invoke(globalPropsUtils, containerId);
  }

  private static void assertRouterOpenRegistered(ClassLoader classLoader) throws Exception {
    Class<?> registrarClass =
        classLoader.loadClass("com.lynx.explorer.sparkling.SparklingNavigationRegistrar");
    Object registered = registrarClass.getMethod("isRouterOpenRegistered").invoke(null);
    assertEquals(Boolean.TRUE, registered);
  }

  private static void assertRouterOpenBridgeCallSucceeds(ClassLoader classLoader,
      Object sparklingContext, Class<?> sparklingContextClass, String containerId)
      throws Exception {
    Object bridge = sparklingContextClass.getMethod("getBridge").invoke(sparklingContext);
    assertNotNull(bridge);
    Object bridgeContext = bridge.getClass().getMethod("getBridgeContext").invoke(bridge);
    assertNotNull(bridgeContext);

    JavaOnlyMap data = new JavaOnlyMap();
    data.putString("scheme",
        "hybrid://lynxview_page?bundle=homepage.lynx.bundle&hide_loading=1&hide_nav_bar=1");
    JavaOnlyMap params = new JavaOnlyMap();
    params.putString("containerID", containerId);
    params.putString("protocolVersion", "1.0.0");
    params.putMap("data", data);

    CountDownLatch latch = new CountDownLatch(1);
    AtomicReference<Object[]> callbackArgsRef = new AtomicReference<>();
    Callback callback = args -> {
      callbackArgsRef.set(args);
      latch.countDown();
    };

    Class<?> delegateClass = classLoader.loadClass(
        "com.tiktok.sparkling.method.protocol.impl.lynx.RealLynxBridgeDelegate");
    Object delegate = delegateClass.getConstructor(Object.class).newInstance(bridgeContext);
    delegateClass.getMethod("call", String.class, ReadableMap.class, Callback.class, String.class)
        .invoke(delegate, "router.open", params, callback, "Lynx");

    assertTrue("router.open callback did not run", latch.await(10, TimeUnit.SECONDS));
    Object[] callbackArgs = callbackArgsRef.get();
    assertNotNull(callbackArgs);
    assertTrue(callbackArgs.length > 0);
    assertTrue(callbackArgs[0] instanceof Map);
    Object code = ((Map<?, ?>) callbackArgs[0]).get("code");
    assertTrue(code instanceof Number);
    assertEquals(1, ((Number) code).intValue());
  }
}
