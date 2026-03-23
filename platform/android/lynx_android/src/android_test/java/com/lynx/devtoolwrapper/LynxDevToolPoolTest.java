// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import static org.junit.Assert.*;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.List;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxDevToolPoolTest {
  private LynxDevToolPool mPool;

  private Field mDebuggableField;
  private Field mUrlField;
  private Field mDevtoolsField;
  private Field mNextContextIdField;

  @Before
  public void setUp() {
    try {
      Context context =
          InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
      LynxEnv.inst().init((Application) context, System::loadLibrary, null, null, null);
      DevToolLifecycle.getInstance().reset();
      DevToolLifecycle.getInstance().onAttached();
      mPool = new LynxDevToolPool("test-url", true);

      ILynxDevToolService devToolService = mock(ILynxDevToolService.class);
      doReturn(ILynxDevToolService.class).when(devToolService).getServiceClass();
      LynxServiceCenter.inst().registerService(devToolService);

      mDebuggableField = LynxDevToolPool.class.getDeclaredField("mDebuggable");
      mDebuggableField.setAccessible(true);

      mUrlField = LynxDevToolPool.class.getDeclaredField("mUrl");
      mUrlField.setAccessible(true);

      mDevtoolsField = LynxDevToolPool.class.getDeclaredField("mDevtools");
      mDevtoolsField.setAccessible(true);

      mNextContextIdField = LynxDevToolPool.class.getDeclaredField("nextContextId");
      mNextContextIdField.setAccessible(true);
    } catch (NoSuchFieldException e) {
      fail("Failed to get field via reflection: " + e.getMessage());
    }
  }

  @After
  public void tearDown() {
    try {
      DevToolLifecycle.getInstance().reset();
      mNextContextIdField.set(null, 0);
    } catch (IllegalAccessException e) {
      fail("Failed to reset static field via reflection: " + e.getMessage());
    }
  }

  @Test
  public void testCreate() throws IllegalAccessException {
    LynxDevToolPool pool1 = new LynxDevToolPool("", false);
    assertEquals("anonymous bundle", mUrlField.get(pool1));
    assertFalse((boolean) mDebuggableField.get(pool1));
    assertNotEquals(0L, pool1.getNativePtr());

    assertEquals("test-url", mUrlField.get(mPool));
    assertTrue((boolean) mDebuggableField.get(mPool));
    assertNotEquals(0L, mPool.getNativePtr());
  }

  @Test
  public void testDestroy() throws IllegalAccessException {
    // Test destroying with an initially empty devtools list
    mPool.destroy();
    assertEquals(0, ((List<LynxDevtool>) mDevtoolsField.get(mPool)).size());
    assertEquals(0L, mPool.getNativePtr());

    // Test destroying with a non-empty devtools list
    LynxDevtool devtool = mock(LynxDevtool.class);
    List<LynxDevtool> devtools = new ArrayList<>();
    devtools.add(devtool);
    mDevtoolsField.set(mPool, devtools);

    mPool.destroy();
    assertEquals(0, ((List<LynxDevtool>) mDevtoolsField.get(mPool)).size());
    assertEquals(0L, mPool.getNativePtr());
    verify(devtool).destroy();
  }

  @Test
  public void testCreateAndPopDevTool() throws IllegalAccessException {
    // Test create when devtools are disabled, list should remain empty
    mPool.createDevTool();
    assertEquals(0, ((List<LynxDevtool>) mDevtoolsField.get(mPool)).size());

    // Test pop on an empty list, should not crash and list remains empty
    mPool.popDevTool();
    assertEquals(0, ((List<LynxDevtool>) mDevtoolsField.get(mPool)).size());

    DevToolLifecycle.getInstance().onEnabled();

    mPool.createDevTool();
    List<LynxDevtool> devtools = (List<LynxDevtool>) mDevtoolsField.get(mPool);
    assertEquals(1, devtools.size());

    // Replace real object with a mock to verify destroy() call
    LynxDevtool devtoolMock = mock(LynxDevtool.class);
    devtools.set(0, devtoolMock);

    mPool.popDevTool();
    assertEquals(0, ((List<LynxDevtool>) mDevtoolsField.get(mPool)).size());
    verify(devtoolMock).destroy();

    DevToolLifecycle.getInstance().onDisabled();
  }

  @Test
  public void testOnMTSRuntimeCreated() throws IllegalAccessException {
    // Test with empty devtools list, should not crash
    mDevtoolsField.set(mPool, new ArrayList<LynxDevtool>());
    mPool.onMTSRuntimeCreated(12345L);

    // 1. Create a mock
    LynxDevtool devtool = mock(LynxDevtool.class);

    // 2. Inject the mock into the pool
    List<LynxDevtool> devtools = new ArrayList<>();
    devtools.add(devtool);
    mDevtoolsField.set(mPool, devtools);

    // 3. Call the method under test
    mPool.onMTSRuntimeCreated(12345L);

    // 4. Verify the interactions and results
    verify(devtool).onMTSRuntimeCreated(12345L);
    verify(devtool).attachToDebugBridge("test-url - context0");
  }
}
