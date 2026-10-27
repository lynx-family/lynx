// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.core;

import static org.junit.Assert.*;

import android.app.Application;
import android.content.Context;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.core.resource.LynxResourceLoader;
import com.lynx.tasm.resourceprovider.TestContentProvider;
import java.io.File;
import java.io.FileOutputStream;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import org.junit.Before;
import org.junit.Test;

public class ResourceLoaderTest {
  private static final String CONTENT_URI = "content://com.lynx.test.resourceprovider/resource";

  @Before
  public void setUp() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, null, null, null, null);
  }
  @Test
  public void testLoadLynxJSAsset() {
    ResourceLoader loader = new ResourceLoader();
    // file exists
    byte[] resource1 = loader.loadLynxJSAsset("lynx_assets://lynx_core.js");
    assertNotNull(resource1);
    assertTrue(resource1.length > 0);
    // file does not exist
    byte[] resource2 = loader.loadLynxJSAsset("lynx_assets://non-existing.js");
    assertNull(resource2);
  }

  @Test
  public void loadJSSourceReturnsBytesForAbsoluteFileUri() throws Exception {
    byte[] expected = "Hello Lynx ResourceLoader absolute file".getBytes(StandardCharsets.UTF_8);
    File file = File.createTempFile("resource-loader", ".js", getContext().getCacheDir());
    writeBytes(file, expected);

    try {
      assertArrayEquals(
          expected, invokeLoadJSSource(new ResourceLoader(), "file://" + file.getAbsolutePath()));
    } finally {
      file.delete();
    }
  }

  @Test
  public void loadJSSourceReturnsBytesForRelativeFileUri() throws Exception {
    byte[] expected = "Hello Lynx ResourceLoader relative file".getBytes(StandardCharsets.UTF_8);
    File file = new File(getContext().getFilesDir(), "resource_loader_relative.js");
    writeBytes(file, expected);

    try {
      assertArrayEquals(
          expected, invokeLoadJSSource(new ResourceLoader(), "file://" + file.getName()));
    } finally {
      file.delete();
    }
  }

  @Test
  public void loadJSSourceReturnsBytesForContentUri() throws Exception {
    byte[] expected = "Hello Lynx ResourceLoader ContentProvider".getBytes(StandardCharsets.UTF_8);
    TestContentProvider.setContent(expected);

    assertArrayEquals(expected, invokeLoadJSSource(new ResourceLoader(), CONTENT_URI));
  }

  @Test
  public void lynxResourceLoaderLoadJSSourceReturnsBytesForContentUri() throws Exception {
    byte[] expected = "Hello LynxResourceLoader ContentProvider".getBytes(StandardCharsets.UTF_8);
    TestContentProvider.setContent(expected);

    LynxResourceLoader loader = new LynxResourceLoader(null, null, null, null);
    assertArrayEquals(expected, invokeLoadJSSource(loader, CONTENT_URI));
  }

  @Test
  public void missingLocalResourceDoesNotPreventSubsequentLoads() throws Exception {
    LynxResourceLoader loader = new LynxResourceLoader(null, null, null, null);
    try {
      invokeLoadJSSource(loader, "assets://loader_missing_test.js");
      fail("Missing assets must preserve their failure for the native response");
    } catch (java.lang.reflect.InvocationTargetException error) {
      assertTrue(error.getCause() instanceof java.io.IOException);
    }
    assertNotNull(invokeLoadJSSource(loader, "assets://lynx_core.js"));
  }

  private Context getContext() {
    return InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
  }

  private byte[] invokeLoadJSSource(ResourceLoader loader, String name) throws Exception {
    Method loadJSSource = ResourceLoader.class.getDeclaredMethod("loadJSSource", String.class);
    loadJSSource.setAccessible(true);
    return (byte[]) loadJSSource.invoke(loader, name);
  }

  private byte[] invokeLoadJSSource(LynxResourceLoader loader, String name) throws Exception {
    Method loadJSSource = LynxResourceLoader.class.getDeclaredMethod("loadJSSource", String.class);
    loadJSSource.setAccessible(true);
    return (byte[]) loadJSSource.invoke(loader, name);
  }

  private void writeBytes(File file, byte[] bytes) throws Exception {
    FileOutputStream outputStream = new FileOutputStream(file);
    try {
      outputStream.write(bytes);
    } finally {
      outputStream.close();
    }
  }
}
