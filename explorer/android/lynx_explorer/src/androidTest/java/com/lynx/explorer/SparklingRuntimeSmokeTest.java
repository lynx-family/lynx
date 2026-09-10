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
import android.os.SystemClock;
import android.view.View;
import android.view.ViewGroup;
import androidx.appcompat.widget.Toolbar;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.explorer.loading.ExplorerLoadingView;
import com.lynx.explorer.routing.LaunchDescriptor;
import com.lynx.explorer.routing.LaunchDescriptorParser;
import com.lynx.explorer.routing.RequestedRuntime;
import com.lynx.explorer.routing.ResolvedRuntime;
import com.lynx.explorer.routing.RouteSource;
import com.lynx.explorer.sparkling.ExplorerAppearance;
import com.lynx.explorer.sparkling.ExplorerRuntimePreferences;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.Behavior;
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
    assertTrue(
        "Sparkling must register the x-element input behavior after Explorer initializes Lynx",
        hasGlobalBehavior("input"));
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

  @Test
  public void canonicalOwnershipAndQrPreferenceAreFlavorIndependent() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    LaunchDescriptor descriptor = new LaunchDescriptorParser().parse(
        "lynx://open?url=hybrid%3A%2F%2Flynxview_page%3Fbundle%3Dmain.lynx.bundle",
        RequestedRuntime.LYNX, RouteSource.SCANNER);
    assertEquals(ResolvedRuntime.SPARKLING, descriptor.getRuntime());
    context.getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE)
        .edit()
        .putString("preferredContainer", "sparkling")
        .commit();
    assertEquals(BuildConfig.ENABLE_SPARKLING ? "sparkling" : "legacy",
        ExplorerRuntimePreferences.read(context));
    context.getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE)
        .edit()
        .putString("preferredContainer", "legacy")
        .commit();
    assertEquals("legacy", ExplorerRuntimePreferences.read(context));
  }

  @Test
  public void appearanceAndLoadingRetryStateAreShared() {
    Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
    ExplorerAppearance.write(context, "Dark");
    assertEquals("Dark", ExplorerAppearance.read(context));
    AtomicReference<ExplorerLoadingView> viewRef = new AtomicReference<>();
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      ExplorerLoadingView view = new ExplorerLoadingView(context);
      view.showDownloading("Lynx");
      assertEquals(ExplorerLoadingView.State.DOWNLOADING, view.getState());
      view.showRendering("Lynx");
      assertEquals(ExplorerLoadingView.State.RENDERING, view.getState());
      view.showError("failed", () -> kotlin.Unit.INSTANCE);
      assertEquals(ExplorerLoadingView.State.ERROR, view.getState());
      viewRef.set(view);
    });
    assertNotNull(viewRef.get());
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
        "hybrid://lynxview_page?bundle=nav-chain.lynx.bundle&hide_loading=1&hide_nav_bar=1");
    JavaOnlyMap extra = new JavaOnlyMap();
    extra.putString("source", "router.open");
    extra.putDouble("count", 2.0);
    data.putMap("extra", extra);
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

    Instrumentation instrumentation = InstrumentationRegistry.getInstrumentation();
    Instrumentation.ActivityMonitor activityMonitor =
        instrumentation.addMonitor("com.tiktok.sparkling.SparklingActivity", null, false);
    Activity sparklingActivity = null;
    try {
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

      sparklingActivity = instrumentation.waitForMonitorWithTimeout(activityMonitor, 10_000);
      assertNotNull("router.open did not launch SparklingActivity", sparklingActivity);
      instrumentation.waitForIdleSync();
      int toolbarId =
          classLoader.loadClass("com.tiktok.sparkling.R$id").getField("toolbar").getInt(null);
      assertTrue("SparklingActivity must retain its public toolbar view",
          sparklingActivity.findViewById(toolbarId) instanceof Toolbar);
      waitForLoadingViewToDisappear(instrumentation, sparklingActivity);
      assertRouterExtras(classLoader, instrumentation, sparklingActivity);
    } finally {
      instrumentation.removeMonitor(activityMonitor);
      if (sparklingActivity != null) {
        sparklingActivity.finish();
      }
    }
  }

  @SuppressWarnings("unchecked")
  private static void assertRouterExtras(
      ClassLoader classLoader, Instrumentation instrumentation, Activity activity) {
    AtomicReference<Map<String, String>> extrasRef = new AtomicReference<>();
    instrumentation.runOnMainSync(() -> {
      try {
        Class<?> kitViewClass =
            classLoader.loadClass("com.tiktok.sparkling.hybridkit.lynx.SimpleLynxKitView");
        View kitView = findViewByClass(activity.getWindow().getDecorView(), kitViewClass);
        assertNotNull("SparklingActivity must contain a SimpleLynxKitView", kitView);
        Object childContext = kitViewClass.getMethod("getHybridContext").invoke(kitView);
        extrasRef.set((Map<String, String>) childContext.getClass()
                          .getMethod("getExtra")
                          .invoke(childContext));
      } catch (ReflectiveOperationException exception) {
        throw new AssertionError("Failed to inspect router.open extras", exception);
      }
    });
    assertNotNull(extrasRef.get());
    assertEquals("router.open", extrasRef.get().get("source"));
    assertEquals("2", extrasRef.get().get("count"));
  }

  private static View findViewByClass(View view, Class<?> expectedClass) {
    if (expectedClass.isInstance(view)) {
      return view;
    }
    if (!(view instanceof ViewGroup)) {
      return null;
    }
    ViewGroup group = (ViewGroup) view;
    for (int index = 0; index < group.getChildCount(); index++) {
      View result = findViewByClass(group.getChildAt(index), expectedClass);
      if (result != null) {
        return result;
      }
    }
    return null;
  }

  private static boolean hasGlobalBehavior(String name) {
    for (Behavior behavior : LynxEnv.inst().getBehaviors()) {
      if (name.equals(behavior.getName())) {
        return true;
      }
    }
    return false;
  }

  private static void waitForLoadingViewToDisappear(
      Instrumentation instrumentation, Activity activity) {
    long deadline = SystemClock.uptimeMillis() + 10_000;
    AtomicReference<Boolean> loadingVisible = new AtomicReference<>(true);
    while (SystemClock.uptimeMillis() < deadline) {
      instrumentation.runOnMainSync(
          ()
              -> loadingVisible.set(containsVisibleView(
                  activity.getWindow().getDecorView(), ExplorerLoadingView.class)));
      if (!loadingVisible.get()) {
        return;
      }
      SystemClock.sleep(50);
    }
    assertFalse("Sparkling page did not finish loading", loadingVisible.get());
  }

  private static boolean containsVisibleView(View view, Class<?> expectedClass) {
    if (expectedClass.isInstance(view) && view.getVisibility() == View.VISIBLE) {
      return true;
    }
    if (!(view instanceof ViewGroup)) {
      return false;
    }
    ViewGroup group = (ViewGroup) view;
    for (int index = 0; index < group.getChildCount(); index++) {
      if (containsVisibleView(group.getChildAt(index), expectedClass)) {
        return true;
      }
    }
    return false;
  }
}
