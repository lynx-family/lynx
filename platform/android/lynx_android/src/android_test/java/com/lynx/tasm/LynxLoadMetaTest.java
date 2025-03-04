// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.*;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxLoadMetaTest {
  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
  }

  @Test
  public void testBuildMeta() {
    LynxLoadMeta.Builder builder = new LynxLoadMeta.Builder();
    builder.setUrl("http://my-template.js");
    builder.setLoadMode(LynxLoadMode.NORMAL);
    builder.setInitialData(TemplateData.empty());
    builder.addLoadOption(LynxLoadOption.DUMP_ELEMENT);
    builder.addLoadOption(LynxLoadOption.RECYCLE_TEMPLATE_BUNDLE);
    LynxLoadMeta meta = builder.build();

    assertEquals(meta.url, "http://my-template.js");
    assertEquals(meta.loadMode, LynxLoadMode.NORMAL);
    assertTrue(meta.loadOptions.contains(LynxLoadOption.DUMP_ELEMENT));
    assertTrue(meta.loadOptions.contains(LynxLoadOption.RECYCLE_TEMPLATE_BUNDLE));
    assertTrue(meta.initialData.isEmpty());
  }
}
