// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.app.Application;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.service.ILynxDevToolService;
import com.lynx.tasm.service.LynxServiceCenter;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxDevToolServiceLateLoadingTest {
  private LynxServiceCenter mServiceCenter;

  @Before
  public void setUp() {
    Application application = (Application) InstrumentationRegistry.getInstrumentation()
                                  .getTargetContext()
                                  .getApplicationContext();
    mServiceCenter = LynxServiceCenter.inst();
    mServiceCenter.unregisterService(ILynxDevToolService.class);
    mServiceCenter.initialize(application);
  }

  @After
  public void tearDown() {
    mServiceCenter.unregisterService(ILynxDevToolService.class);
  }

  @Test
  public void testGlobalHelperGetsServiceRegisteredAfterFirstLookup() {
    // Trigger the initial lookup before DevTool service is registered.
    assertNull(LynxDevtoolGlobalHelper.getDevToolService());

    ILynxDevToolService devToolService = registerDevToolService();

    // Verify a subsequent lookup gets the late-registered service.
    assertSame(devToolService, LynxDevtoolGlobalHelper.getDevToolService());
  }

  @Test
  public void testUtilsGetsServiceRegisteredAfterClassInitialization() {
    INativeLibraryLoader libraryLoader = mock(INativeLibraryLoader.class);
    // Trigger class initialization and the initial lookup before DevTool service is registered.
    LynxDevToolUtils.setDevToolLibraryLoader(libraryLoader);
    assertNull(LynxDevToolUtils.getDevToolService());

    ILynxDevToolService devToolService = registerDevToolService();
    LynxDevToolUtils.setDevToolLibraryLoader(libraryLoader);

    // Verify the call after late registration reaches the service.
    verify(devToolService).devtoolEnvSetDevToolLibraryLoader(libraryLoader);
  }

  private ILynxDevToolService registerDevToolService() {
    ILynxDevToolService devToolService = mock(ILynxDevToolService.class);
    doReturn(ILynxDevToolService.class).when(devToolService).getServiceClass();
    mServiceCenter.registerService(devToolService);
    return devToolService;
  }
}
