// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.jsbridge.CommonModuleCreator;
import com.lynx.jsbridge.LynxAccessibilityModule;
import com.lynx.jsbridge.LynxFetchModule;
import com.lynx.jsbridge.LynxModule;
import com.lynx.jsbridge.LynxModuleFactory;
import com.lynx.jsbridge.LynxUIMethodModule;
import com.lynx.jsbridge.ParamWrapper;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.core.resource.LynxResourceLoader;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceProvider;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxHostEnvironmentTest {
  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
  }

  private LynxHostEnvironment createHostEnvironment(LynxBackgroundRuntimeOptions options) {
    return new LynxHostEnvironment(options, new LynxViewBuilder().getBehaviorRegistry());
  }

  @SuppressWarnings("unchecked")
  private Map<String, ParamWrapper> registrations(LynxModuleFactory factory) throws Exception {
    Method method = LynxModuleFactory.class.getDeclaredMethod("getWrappers");
    method.setAccessible(true);
    return (Map<String, ParamWrapper>) method.invoke(factory);
  }

  @Test
  public void configurationCopiesOptionsAndRetainsBehaviors() {
    LynxViewBuilder builder = new LynxViewBuilder();
    LynxBackgroundRuntimeOptions source = builder.getLynxRuntimeOptions();
    LynxGroup group = new LynxGroup.LynxGroupBuilder().setGroupName("legacy-group").build();
    source.setLynxGroup(group);
    source.registerModule("custom", LynxModule.class, null);
    source.setBytecodeSourceUrl("initial");
    LynxHostEnvironment host =
        new LynxHostEnvironment(builder.getLynxRuntimeOptions(), builder.getBehaviorRegistry());
    source.setBytecodeSourceUrl("changed");
    assertSame(builder.getBehaviorRegistry(), host.getBehaviorRegistry());
    LynxBackgroundRuntimeOptions copy = host.getRuntimeOptions();
    assertNotSame(source, copy);
    assertEquals("initial", copy.getBytecodeSourceUrl());
    assertSame(copy, host.getRuntimeOptions());
    assertSame(group, copy.getLynxGroup());
    assertEquals(source.calcRuntimeFlags(false, true), copy.calcRuntimeFlags(false, true));
    assertNotSame(source.getWrappers(), copy.getWrappers());
    // Preserve the pre-existing shallow-copy contract of legacy runtime options.
    assertSame(source.getWrappers().get(0), copy.getWrappers().get(0));
    copy.getWrappers().clear();
    assertEquals(1, source.getWrappers().size());
  }

  @Test
  public void modulePrecedenceAndContextArePreserved() throws Exception {
    LynxBackgroundRuntimeOptions options = new LynxBackgroundRuntimeOptions();
    LynxModule.AuthValidator validator = mock(LynxModule.AuthValidator.class);
    options.registerModuleAuthValidator(validator);
    options.registerModule(LynxUIMethodModule.NAME, LynxModule.class, null);
    options.registerModule(LynxFetchModule.NAME, LynxModule.class, null);
    LynxHostEnvironment host = createHostEnvironment(options);
    LynxContext first = mock(LynxContext.class);
    LynxContext second = mock(LynxContext.class);
    LynxModuleFactory main = host.createMainThreadFactory(first, null);
    LynxModuleFactory background = host.createBackgroundThreadFactory(second, null);
    try {
      Field auth = LynxModuleFactory.class.getDeclaredField("mAuthValidator");
      auth.setAccessible(true);
      assertNull(auth.get(main));
      assertSame(validator, auth.get(background));
      Map<String, ParamWrapper> mainModules = registrations(main);
      Map<String, ParamWrapper> backgroundModules = registrations(background);
      assertEquals(LynxModule.class, mainModules.get(LynxUIMethodModule.NAME).getModuleClass());
      assertEquals(LynxUIMethodModule.class,
          backgroundModules.get(LynxUIMethodModule.NAME).getModuleClass());
      assertTrue(mainModules.containsKey(LynxAccessibilityModule.NAME));
      assertTrue(backgroundModules.containsKey(LynxAccessibilityModule.NAME));
      assertEquals(LynxModule.class, mainModules.get(LynxFetchModule.NAME).getModuleClass());
      assertEquals(
          LynxFetchModule.class, backgroundModules.get(LynxFetchModule.NAME).getModuleClass());
      assertSame(first, main.currentContextFinder().findContext(null).get());
      assertSame(second, background.currentContextFinder().findContext(null).get());
    } finally {
      main.destroy();
      background.destroy();
    }
  }

  @Test
  public void nonSharedFactoriesRemainPageOwned() throws Exception {
    LynxBackgroundRuntimeOptions options = new LynxBackgroundRuntimeOptions();
    Object userParam = new Object();
    options.registerModule("custom", LynxModule.class, userParam);
    LynxHostEnvironment host = createHostEnvironment(options);
    LynxContext firstContext = mock(LynxContext.class);
    LynxContext secondContext = mock(LynxContext.class);
    LynxModuleFactory first = host.createBackgroundThreadFactory(firstContext, null);
    LynxModuleFactory second = host.createBackgroundThreadFactory(secondContext, null);
    try {
      Map<String, ParamWrapper> firstModules = registrations(first);
      Map<String, ParamWrapper> secondModules = registrations(second);
      assertNotSame(firstModules, secondModules);
      assertSame(userParam, secondModules.get("custom").getParam());
      assertNotSame(firstModules.get(LynxFetchModule.NAME).getParam(),
          secondModules.get(LynxFetchModule.NAME).getParam());
      first.registerModule("custom", LynxUIMethodModule.class, null);
      assertEquals(LynxModule.class, secondModules.get("custom").getModuleClass());
      first.destroy();
      assertTrue(secondModules.containsKey("custom"));
      assertSame(secondContext, second.currentContextFinder().findContext(null).get());
    } finally {
      first.destroy();
      second.destroy();
    }
  }

  @Test
  public void runtimeFactoryKeepsCreatorAndBindsPageContext() throws Exception {
    LynxBackgroundRuntimeOptions options = new LynxBackgroundRuntimeOptions();
    Object hostParam = new Object();
    options.registerModule("overlap", LynxModule.class, hostParam);
    options.registerModule(LynxUIMethodModule.NAME, LynxModule.class, null);
    LynxHostEnvironment host = createHostEnvironment(options);
    LynxContext context = mock(LynxContext.class);
    LynxModuleFactory factory = spy(new LynxModuleFactory());
    Object runtimeParam = new Object();
    factory.registerModule("runtime-only", LynxModule.class, runtimeParam);
    factory.registerModule("overlap", LynxModule.class, runtimeParam);
    factory.registerModule(LynxUIMethodModule.NAME, LynxModule.class, runtimeParam);
    CommonModuleCreator creator = spy(new CommonModuleCreator(null));
    factory.bind(creator);
    try {
      host.prepareReusedBackgroundThreadFactory(factory, context, null);
      verify(factory, times(1)).bind(creator);
      verify(factory, never()).registerModuleAuthValidator(any());
      verify(creator).resetContextFinder(any());
      assertSame(context, factory.currentContextFinder().findContext(null).get());
      assertSame(runtimeParam, registrations(factory).get("runtime-only").getParam());
      assertSame(hostParam, registrations(factory).get("overlap").getParam());
      assertEquals(LynxUIMethodModule.class,
          registrations(factory).get(LynxUIMethodModule.NAME).getModuleClass());
      assertEquals(
          LynxFetchModule.class, registrations(factory).get(LynxFetchModule.NAME).getModuleClass());
    } finally {
      factory.destroy();
    }
  }

  @Test
  public void providerRegistryIsMergedOnceWithHostOverrides() {
    String key = "host-environment-test-provider";
    Map<String, LynxResourceProvider> globals = LynxEnv.inst().getResourceProvider();
    LynxResourceProvider global = mock(LynxResourceProvider.class);
    LynxResourceProvider local = mock(LynxResourceProvider.class);
    LynxResourceProvider previous = globals.put(key, global);
    try {
      LynxBackgroundRuntimeOptions options = new LynxBackgroundRuntimeOptions();
      LynxHostEnvironment globalHost = createHostEnvironment(options);
      options.setResourceProviders(key, local);
      LynxHostEnvironment host = createHostEnvironment(options);
      LynxProviderRegistry providers = host.getProviderRegistry();
      globals.put(key, local);
      host.getRuntimeOptions().setResourceProviders(key, global);
      assertSame(providers, host.getProviderRegistry());
      assertSame(local, providers.getProviderByKey(key));
      assertSame(global, globalHost.getProviderRegistry().getProviderByKey(key));
    } finally {
      if (previous == null) {
        globals.remove(key);
      } else {
        globals.put(key, previous);
      }
    }
  }

  @Test
  public void resourceLoaderIsReadyWhenEnvironmentIsCreated() throws Exception {
    LynxBackgroundRuntimeOptions options = new LynxBackgroundRuntimeOptions();
    LynxHostEnvironment host =
        new LynxHostEnvironment(options, new LynxViewBuilder().getBehaviorRegistry());
    LynxResourceLoader loader = host.getRuntimeResourceLoader();
    assertNotNull(loader);
    assertSame(loader, host.getResourceLoader());
    assertNull(host.getEngineResourceLoader());
    Field field = LynxResourceLoader.class.getDeclaredField("mLynxRuntimeOptions");
    field.setAccessible(true);
    assertSame(host.getRuntimeOptions(), field.get(loader));
  }

  @Test
  public void rendererLoadersReplaceTheDefaultAndRemainSeparate() throws Exception {
    LynxHostEnvironment host = createHostEnvironment(new LynxBackgroundRuntimeOptions());
    LynxContext context = mock(LynxContext.class);
    assertNull(host.getEngineResourceLoader());
    LynxResourceLoader defaultLoader = host.getRuntimeResourceLoader();
    assertNotNull(defaultLoader);
    LynxResourceLoader engine = host.createEngineResourceLoader(null, context);
    LynxResourceLoader runtime = host.createRuntimeResourceLoader(null, context);
    assertSame(engine, host.getEngineResourceLoader());
    assertSame(runtime, host.getRuntimeResourceLoader());
    assertNotSame(defaultLoader, runtime);
    assertNotSame(engine, runtime);
    Field field = LynxResourceLoader.class.getDeclaredField("mLynxRuntimeOptions");
    field.setAccessible(true);
    assertNull(field.get(engine));
    assertSame(host.getRuntimeOptions(), field.get(runtime));
    LynxResourceLoader rebuiltEngine = host.createEngineResourceLoader(null, context);
    assertNotSame(engine, rebuiltEngine);
    assertSame(rebuiltEngine, host.getEngineResourceLoader());
    assertSame(runtime, host.getRuntimeResourceLoader());
  }
}
