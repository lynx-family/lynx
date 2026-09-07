// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.modules;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import com.lynx.jsbridge.LynxContextModule;
import com.lynx.jsbridge.LynxMethod;
import com.lynx.react.bridge.JavaOnlyMap;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Test;

public class LynxTestModuleTest {
  @Test
  public void exposesContextMethodsUnderNewModuleName() {
    assertEquals("LynxTestModule", LynxTestModule.NAME);
    assertTrue(LynxContextModule.class.isAssignableFrom(LynxTestModule.class));
    Set<String> methods = new HashSet<>();
    for (Method method : LynxTestModule.class.getDeclaredMethods()) {
      if (method.isAnnotationPresent(LynxMethod.class)) {
        methods.add(method.getName());
      }
    }
    assertEquals(new HashSet<>(Arrays.asList("call", "invoke", "callSync", "invokeSync",
                     "updateData", "resetData", "updateGlobalProps", "reload", "reloadTemplate",
                     "getPageDataByKey", "updateScreenMatrix", "addButton")),
        methods);
  }

  @Test
  public void preservesSyncAndCallbackPayloads() {
    LynxTestModule module = new LynxTestModule(null);
    JavaOnlyMap params = new JavaOnlyMap();
    assertEquals("----lepus value--success", module.callSync("test", params).getString("result"));
    assertEquals("----lepus value--success", module.invokeSync(params).getString("result"));
    AtomicInteger calls = new AtomicInteger();
    module.call("test", params, args -> {
      calls.incrementAndGet();
      assertEquals(1, args.length);
      assertEquals("success", ((JavaOnlyMap) args[0]).getString("result"));
    });
    module.invoke(params, args -> {
      calls.incrementAndGet();
      assertEquals("success", ((JavaOnlyMap) args[0]).getString("result"));
    });
    assertEquals(2, calls.get());
  }

  @Test
  public void pageDataResultsAllowNullAndEmptySdkResults() {
    assertTrue(LynxTestModule.toPageDataResult(null).isEmpty());
    assertTrue(LynxTestModule.toPageDataResult(Collections.emptyMap()).isEmpty());
    assertEquals(7,
        LynxTestModule.toPageDataResult(Collections.singletonMap("selected", 7))
            .getInt("selected"));
  }

  @Test
  public void nullCallbacksAreAllowed() {
    LynxTestModule module = new LynxTestModule(null);
    module.call("test", new JavaOnlyMap(), null);
    module.invoke(new JavaOnlyMap(), null);
  }
}
