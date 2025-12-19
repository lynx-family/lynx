// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import androidx.annotation.NonNull;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collection;
import java.util.concurrent.ConcurrentHashMap;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxCommonModuleFactoryTest {
  public LynxModuleFactory mCommonModuleFactory;
  public LynxContext mLynxContext;

  private static class TestModule extends LynxModule {
    public TestModule(Context context) {
      super(context);
    }
    @LynxMethod
    public void testMethod1(boolean param1, int param2) {}

    @LynxMethod
    public JavaOnlyMap testMethod2(JavaOnlyArray param1, JavaOnlyMap param2) {
      return new JavaOnlyMap();
    }
  }

  private static class TestLynxContextModule extends LynxContextModule {
    public TestLynxContextModule(LynxContext context) {
      super(context);
    }
    @LynxMethod
    public void testMethod() {}
  }

  private static class TestSignatureModule extends LynxModule {
    public TestSignatureModule(Context context) {
      super(context);
    }

    @LynxMethod
    public boolean testBoolean(boolean param1, Boolean param2) {
      return param1;
    }

    @LynxMethod
    public char testCharacter(char param1, Character param2) {
      return param1;
    }

    @LynxMethod
    public int testInt(int param1, Integer param2) {
      return param1;
    }

    @LynxMethod
    public double testDouble(double param1, Double param2) {
      return param1;
    }

    @LynxMethod
    public short testShort(short param1, Short param2) {
      return param1;
    }

    @LynxMethod
    public long testLong(long param1, Long param2) {
      return param1;
    }

    @LynxMethod
    public float testFloat(float param1, Float param2) {
      return param1;
    }

    @LynxMethod
    public String testString(String param1) {
      return param1;
    }

    @LynxMethod
    public JavaOnlyMap testJavaOnlyMap(JavaOnlyMap param1, JavaOnlyArray param2) {
      return param1;
    }

    @LynxMethod
    public JavaOnlyArray testJavaOnlyArray(JavaOnlyMap param1, JavaOnlyArray param2) {
      return param2;
    }

    @LynxMethod
    public void testCallBack(JavaOnlyMap param1, JavaOnlyArray param2, Callback callback) {}
  }

  @Before
  public void setUp() {
    mCommonModuleFactory = new LynxModuleFactory();
    mLynxContext = TestingUtils.getLynxContext();
    // bind common module creator
    IContextFinder contextFinder = new IContextFinder() {
      private final WeakReference<Context> mContext = new WeakReference<>(mLynxContext);

      @Override
      @NonNull
      public WeakReference<Context> findContext(String instanceId) {
        return mContext;
      }

      @Override
      public void registerContext(String instanceId, WeakReference<Context> context) {}
    };
    mCommonModuleFactory.bind(new CommonModuleCreator(contextFinder));
  }

  @Test
  public void testRegisterModule() {
    String testLynxModuleName = "TestLynxModule";
    mCommonModuleFactory.registerModule(testLynxModuleName, TestModule.class, null);

    String testLynxContextModuleName = "TestLynxContextModule";
    mCommonModuleFactory.registerModule(
        testLynxContextModuleName, TestLynxContextModule.class, null);

    ConcurrentHashMap<String, ParamWrapper> wrappers =
        (ConcurrentHashMap<String, ParamWrapper>) mCommonModuleFactory.getWrappers();
    assertTrue(wrappers.containsKey(testLynxModuleName));
    assertTrue(wrappers.containsKey(testLynxContextModuleName));
    assertEquals(TestModule.class, wrappers.get(testLynxModuleName).getModuleClass());
    assertEquals(
        TestLynxContextModule.class, wrappers.get(testLynxContextModuleName).getModuleClass());
  }

  @Test
  public void testAddModuleParamWrapper() {
    String moduleName = "TestModule";
    ParamWrapper wrapper = new ParamWrapper();
    wrapper.setName(moduleName);
    wrapper.setModuleClass(TestModule.class);

    ArrayList<ParamWrapper> wrappersList = new ArrayList<>();
    wrappersList.add(wrapper);

    mCommonModuleFactory.addModuleParamWrapper(wrappersList);
    ConcurrentHashMap<String, ParamWrapper> wrappers =
        (ConcurrentHashMap<String, ParamWrapper>) mCommonModuleFactory.getWrappers();
    assertTrue(wrappers.containsKey(moduleName));
  }

  @Test
  public void testAddModuleParamWrapperIfAbsent() {
    String moduleName = "TestModule";
    mCommonModuleFactory.registerModule(moduleName, TestModule.class, null);

    ParamWrapper wrapper = new ParamWrapper();
    wrapper.setName(moduleName);
    wrapper.setModuleClass(TestModule.class);

    ArrayList<ParamWrapper> wrappersList = new ArrayList<>();
    wrappersList.add(wrapper);

    mCommonModuleFactory.addModuleParamWrapperIfAbsent(wrappersList);
    ConcurrentHashMap<String, ParamWrapper> wrappers =
        (ConcurrentHashMap<String, ParamWrapper>) mCommonModuleFactory.getWrappers();
    assertEquals(1, wrappers.size());
  }

  @Test
  public void testGetModule() {
    String moduleName = "TestModule";
    mCommonModuleFactory.registerModule(moduleName, TestModule.class, null);

    LynxModuleWrapper moduleWrapper = mCommonModuleFactory.getModule(moduleName);
    assertNotNull(moduleWrapper);
    Collection<MethodDescriptor> methodDescriptors = moduleWrapper.getMethodDescriptors();
    assertEquals(2, methodDescriptors.size());

    MethodDescriptor methodDescriptor1 = (MethodDescriptor) methodDescriptors.toArray()[0];
    assertEquals("testMethod1", methodDescriptor1.getName());
    assertEquals("v.zi", methodDescriptor1.getSignature());
  }

  @Test
  public void testMethodSignature() {
    String moduleName = "TestSignatureModule";
    mCommonModuleFactory.registerModule(moduleName, TestSignatureModule.class, null);

    LynxModuleWrapper moduleWrapper = mCommonModuleFactory.getModule(moduleName);
    assertNotNull(moduleWrapper);
    Collection<MethodDescriptor> methodDescriptors = moduleWrapper.getMethodDescriptors();
    assertEquals(11, methodDescriptors.size());

    // test method signature
    methodDescriptors.forEach(methodDescriptor -> {
      switch (methodDescriptor.getName()) {
        case "testBoolean":
          assertEquals("z.zZ", methodDescriptor.getSignature());
          break;
        case "testCharacter":
          assertEquals("c.cC", methodDescriptor.getSignature());
          break;
        case "testInt":
          assertEquals("i.iI", methodDescriptor.getSignature());
          break;
        case "testDouble":
          assertEquals("d.dD", methodDescriptor.getSignature());
          break;
        case "testShort":
          assertEquals("s.sS", methodDescriptor.getSignature());
          break;
        case "testLong":
          assertEquals("l.lL", methodDescriptor.getSignature());
          break;
        case "testString":
          assertEquals("T.T", methodDescriptor.getSignature());
          break;
        case "testJavaOnlyMap":
          assertEquals("M.MA", methodDescriptor.getSignature());
          break;
        case "testJavaOnlyArray":
          assertEquals("A.MA", methodDescriptor.getSignature());
          break;
        case "testCallBack":
          assertEquals("v.MAX", methodDescriptor.getSignature());
          break;
        default:
          break;
      }
    });
  }

  @After
  public void tearDown() throws InterruptedException {
    mCommonModuleFactory = null;
    mLynxContext = null;
  }
}
