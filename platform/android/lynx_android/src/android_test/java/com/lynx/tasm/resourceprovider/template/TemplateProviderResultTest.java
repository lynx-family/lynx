// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.resourceprovider.template;
import static org.junit.Assert.*;

import android.app.Application;
import android.content.Context;
import androidx.test.InstrumentationRegistry;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.TemplateBundle;
import java.nio.ByteBuffer;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class TemplateProviderResultTest {
  @Before
  public void setUp() throws Exception {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
  }
  @After
  public void tearDown() throws Exception {}
  @Test
  public void getTemplateBinary() {
    byte[] binary = new byte[10];
    TemplateProviderResult result = TemplateProviderResult.fromBinary(binary);
    assertNotNull(result.getTemplateBinary());
    assertEquals(10, result.getTemplateBinary().length);
  }

  @Test
  public void getTemplateBundle() {
    byte[] binary = new byte[10];
    TemplateBundle bundle = TemplateBundle.fromTemplate(binary);
    TemplateProviderResult result = TemplateProviderResult.fromTemplateBundle(bundle);
    assertNotNull(result.getTemplateBundle());
  }

  @Test
  public void getTemplateBuffer() {
    ByteBuffer buffer = ByteBuffer.allocateDirect(10);
    TemplateProviderResult result = TemplateProviderResult.fromBuffer(buffer);
    assertNotNull(result.getTemplateBuffer());
    assertEquals(10, result.getTemplateBuffer().limit());
  }

  @Test
  public void fromBinary() {
    byte[] binary = new byte[10];
    TemplateProviderResult result = TemplateProviderResult.fromBinary(binary);
    assertNotNull(result.getTemplateBinary());
    assertEquals(10, result.getTemplateBinary().length);
  }

  @Test
  public void fromTemplateBundle() {
    byte[] binary = new byte[10];
    TemplateBundle bundle = TemplateBundle.fromTemplate(binary);
    TemplateProviderResult result = TemplateProviderResult.fromTemplateBundle(bundle);
    assertNotNull(result.getTemplateBundle());
  }

  @Test
  public void fromBuffer() {
    ByteBuffer buffer = ByteBuffer.allocateDirect(10);
    TemplateProviderResult result = TemplateProviderResult.fromBuffer(buffer);
    assertNotNull(result.getTemplateBuffer());
    assertEquals(10, result.getTemplateBuffer().limit());
  }
}
