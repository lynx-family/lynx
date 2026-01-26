// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import static org.junit.Assert.*;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtoolwrapper.DevToolSettings;
import com.lynx.tasm.LynxEnv;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxDevtoolEnvTest {
  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, System::loadLibrary, null, null, null);
    LynxDevtoolEnv.inst().init(context);
  }

  @Test
  public void getVersion() {
    assertNotNull(LynxDevtoolEnv.inst().getVersion());
  }

  @Test
  public void enableV8() {
    LynxDevtoolEnv.inst().enableV8(DevToolSettings.V8_OFF);
    assertEquals(DevToolSettings.V8_OFF, LynxDevtoolEnv.inst().getV8Enabled());

    LynxDevtoolEnv.inst().enableV8(DevToolSettings.V8_ON);
    assertEquals(DevToolSettings.V8_ON, LynxDevtoolEnv.inst().getV8Enabled());

    LynxDevtoolEnv.inst().enableV8(DevToolSettings.V8_ALIGN_WITH_PROD);
    assertEquals(DevToolSettings.V8_ALIGN_WITH_PROD, LynxDevtoolEnv.inst().getV8Enabled());

    LynxDevtoolEnv.inst().enableV8(3);
    assertEquals(DevToolSettings.V8_ALIGN_WITH_PROD, LynxDevtoolEnv.inst().getV8Enabled());

    LynxDevtoolEnv.inst().enableV8(-1);
    assertEquals(DevToolSettings.V8_OFF, LynxDevtoolEnv.inst().getV8Enabled());
  }
}
