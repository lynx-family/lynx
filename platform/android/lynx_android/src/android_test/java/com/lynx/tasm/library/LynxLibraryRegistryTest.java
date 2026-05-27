// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.library;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

import android.app.Application;
import android.content.Context;
import androidx.annotation.NonNull;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.jsbridge.LynxModule;
import com.lynx.jsbridge.ParamWrapper;
import com.lynx.tasm.LynxBackgroundRuntimeOptions;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.Behavior;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.service.IServiceProvider;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.Map;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxLibraryRegistryTest {
  private static final String BUILDER_BEHAVIOR = "autolink-builder-behavior";
  private static final String BUILDER_MODULE = "AutolinkBuilderModule";
  private static final String GLOBAL_BEHAVIOR = "autolink-global-behavior";
  private static final String GLOBAL_MODULE = "AutolinkGlobalModule";

  public static class TestModule extends LynxModule {
    public TestModule(Context context) {
      super(context);
    }
  }

  public static class BuilderProvider implements LynxLibraryProvider {
    static boolean sContextRejected;

    @Override
    public void register(@NonNull LynxLibraryRegistry registry) {
      registry.addBehaviors(Arrays.asList(new Behavior(BUILDER_BEHAVIOR)));
      registry.registerModule(BUILDER_MODULE, TestModule.class, "builder-param");
      registry.registerService(new ConstructorService());
      try {
        registry.getContextOrThrow();
      } catch (IllegalStateException e) {
        sContextRejected = true;
      }
    }
  }

  public static class GlobalProvider implements LynxLibraryProvider {
    static Context sContext;

    @Override
    public void register(@NonNull LynxLibraryRegistry registry) {
      registry.addBehaviors(null);
      registry.addBehaviors(Arrays.asList(new Behavior(GLOBAL_BEHAVIOR)));
      registry.registerModule(GLOBAL_MODULE, TestModule.class);
      registry.registerService(SingletonService.class);
      sContext = registry.getContextOrThrow();
    }
  }

  public static class NonProvider {}

  public static class ConstructorService implements IServiceProvider {
    boolean mInitialized;

    @Override
    public Class<? extends IServiceProvider> getServiceClass() {
      return ConstructorService.class;
    }

    @Override
    public void onInitialize(Context context) {
      mInitialized = true;
    }
  }

  public static class SingletonService implements IServiceProvider {
    public static final SingletonService INSTANCE = new SingletonService();
    boolean mInitialized;

    @Override
    public Class<? extends IServiceProvider> getServiceClass() {
      return SingletonService.class;
    }

    @Override
    public void onInitialize(Context context) {
      mInitialized = true;
    }
  }

  @Before
  public void setUp() {
    LynxServiceCenter.inst().unregisterAllService();
    BuilderProvider.sContextRejected = false;
    GlobalProvider.sContext = null;
    SingletonService.INSTANCE.mInitialized = false;
  }

  @After
  public void tearDown() {
    LynxServiceCenter.inst().unregisterAllService();
  }

  @Test
  public void testSetupRegistersIntoBuilder() throws Exception {
    LynxViewBuilder builder = new LynxViewBuilder();

    LynxLibraryRegistry.setup(builder,
        new String[] {null, "", "missing.Provider", NonProvider.class.getName(),
            BuilderProvider.class.getName()});

    BehaviorRegistry behaviorRegistry = getField(builder, "behaviorRegistry");
    assertTrue(behaviorRegistry.getAllBehaviorRegistryName().contains(BUILDER_BEHAVIOR));

    ParamWrapper wrapper = findWrapper(getRuntimeOptions(builder), BUILDER_MODULE);
    assertNotNull(wrapper);
    assertEquals(TestModule.class, wrapper.getModuleClass());
    assertEquals("builder-param", wrapper.getParam());

    LynxServiceCenter.inst().initialize(ApplicationProvider.getApplicationContext());
    ConstructorService service = LynxServiceCenter.inst().getService(ConstructorService.class);
    assertNotNull(service);
    assertTrue(service.mInitialized);
    assertTrue(BuilderProvider.sContextRejected);
  }

  @Test
  public void testSetupGlobalRegistersIntoLynxEnv() throws Exception {
    Application application = ApplicationProvider.getApplicationContext();

    LynxLibraryRegistry.setupGlobal(application, new String[] {GlobalProvider.class.getName()});

    assertSame(application, GlobalProvider.sContext);
    assertTrue(hasGlobalBehavior(GLOBAL_BEHAVIOR));
    assertNotNull(findWrapper(LynxEnv.inst().getModuleFactory(), GLOBAL_MODULE));
    assertSame(
        SingletonService.INSTANCE, LynxServiceCenter.inst().getService(SingletonService.class));
    assertTrue(SingletonService.INSTANCE.mInitialized);
  }

  private static boolean hasGlobalBehavior(String behaviorName) {
    for (Behavior behavior : LynxEnv.inst().getBehaviors()) {
      if (behaviorName.equals(behavior.getName())) {
        return true;
      }
    }
    return false;
  }

  private static LynxBackgroundRuntimeOptions getRuntimeOptions(LynxViewBuilder builder)
      throws Exception {
    return getField(builder, "lynxRuntimeOptions");
  }

  private static ParamWrapper findWrapper(LynxBackgroundRuntimeOptions options, String moduleName) {
    for (ParamWrapper wrapper : options.getWrappers()) {
      if (moduleName.equals(wrapper.getName())) {
        return wrapper;
      }
    }
    return null;
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
}
