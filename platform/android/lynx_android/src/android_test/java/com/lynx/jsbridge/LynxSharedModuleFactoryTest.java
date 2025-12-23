// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collection;
import java.util.concurrent.ConcurrentHashMap;
import org.junit.Before;
import org.junit.Test;

public class LynxSharedModuleFactoryTest {
  public LynxModuleFactory mSharedModuleFactory;
  public LynxContext mLynxContext;

  private static class TestSharedModule extends LynxModule {
    public TestSharedModule(Context context) {
      super(context);
    }
    @LynxMethod
    public void testMethod1(LynxContext context, boolean param1, int param2) {}

    @LynxMethod
    public JavaOnlyMap testMethod2(LynxContext context, JavaOnlyArray param1, JavaOnlyMap param2) {
      return new JavaOnlyMap();
    }

    @LynxMethod
    public void testMethod3(LynxContext context, JavaOnlyMap param2, Callback callback) {
      return;
    }

    @LynxMethod
    public void testMethod4(JavaOnlyMap param2, Callback callback) {
      return;
    }
  }

  @Before
  public void setUp() {
    mSharedModuleFactory = new LynxModuleFactory();
    mLynxContext = TestingUtils.getLynxContext();

    SharedContextFinder contextFinder = new SharedContextFinder();
    SharedModuleCreator sharedCreator = new SharedModuleCreator(contextFinder);
    mSharedModuleFactory.bind(sharedCreator);

    contextFinder.registerContext("1", new WeakReference<>(mLynxContext));
    contextFinder.registerContext("2", new WeakReference<>(mLynxContext));
    contextFinder.registerContext("3", new WeakReference<>(mLynxContext));
  }

  @Test
  public void testGetContext() {
    String moduleName = "TestSharedLynxModule";
    mSharedModuleFactory.registerModule(moduleName, TestSharedModule.class, null);

    LynxModuleWrapper moduleWrapper = mSharedModuleFactory.getModule(moduleName);
    assertNotNull(moduleWrapper);
    assertNotNull(moduleWrapper.getModule());

    assertNotNull(moduleWrapper.getLynxContext("1"));
    assertNotNull(moduleWrapper.getLynxContext("2"));
    assertNotNull(moduleWrapper.getLynxContext("3"));
  }

  @Test
  public void testRegisterModule() {
    String moduleName = "TestSharedLynxModule";
    mSharedModuleFactory.registerModule(moduleName, TestSharedModule.class, null);

    ConcurrentHashMap<String, ParamWrapper> wrappers =
        (ConcurrentHashMap<String, ParamWrapper>) mSharedModuleFactory.getWrappers();
    assertTrue(wrappers.containsKey(moduleName));
    assertEquals(TestSharedModule.class, wrappers.get(moduleName).getModuleClass());
  }

  @Test
  public void testAddModuleParamWrapper() {
    String moduleName = "TestSharedLynxModule";
    ParamWrapper wrapper = new ParamWrapper();
    wrapper.setName(moduleName);
    wrapper.setModuleClass(TestSharedModule.class);

    ArrayList<ParamWrapper> wrappersList = new ArrayList<>();
    wrappersList.add(wrapper);

    mSharedModuleFactory.addModuleParamWrapper(wrappersList);
    ConcurrentHashMap<String, ParamWrapper> wrappers =
        (ConcurrentHashMap<String, ParamWrapper>) mSharedModuleFactory.getWrappers();
    assertTrue(wrappers.containsKey(moduleName));
  }

  @Test
  public void testGetModuleAndMethodDescriptors() {
    String moduleName = "TestSharedLynxModule";
    mSharedModuleFactory.registerModule(moduleName, TestSharedModule.class, null);

    LynxModuleWrapper moduleWrapper = mSharedModuleFactory.getModule(moduleName);
    assertNotNull(moduleWrapper);
    assertNotNull(moduleWrapper.getModule());
    Collection<MethodDescriptor> methodDescriptors = moduleWrapper.getMethodDescriptors();
    assertEquals(4, methodDescriptors.size());

    // test method signature
    methodDescriptors.forEach(methodDescriptor -> {
      switch (methodDescriptor.getName()) {
        case "testMethod1":
          assertEquals("v.Kzi", methodDescriptor.getSignature());
          break;
        case "testMethod2":
          assertEquals("M.KAM", methodDescriptor.getSignature());
          break;
        case "testMethod3":
          assertEquals("v.KMX", methodDescriptor.getSignature());
          break;
        case "testMethod4":
          assertEquals("v.MX", methodDescriptor.getSignature());
          break;
        default:
          break;
      }
    });
  }
}
