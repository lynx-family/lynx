// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.recorder;

import static org.junit.Assert.*;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.util.Base64;
import android.widget.FrameLayout;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtool.LynxDevtoolEnv;
import com.lynx.tasm.LynxEnv;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.json.JSONObject;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxRecorderFixtureTest {
  private Context mContext;

  @Before
  public void setUp() {
    mContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) mContext, System::loadLibrary, null, null, null);
    LynxDevtoolEnv.inst().init(mContext);
  }

  private byte[] zip(String... entries) throws Exception {
    ByteArrayOutputStream bytes = new ByteArrayOutputStream();
    try (ZipOutputStream zip = new ZipOutputStream(bytes)) {
      for (int i = 0; i < entries.length; i += 2) {
        zip.putNextEntry(new ZipEntry(entries[i]));
        zip.write(entries[i + 1].getBytes(StandardCharsets.UTF_8));
        zip.closeEntry();
      }
    }
    return bytes.toByteArray();
  }

  @Test
  public void evaluatesAndAdaptsFixtureWithOwnedAssets() throws Exception {
    byte[] bytes = zip("fixture.js",
        "export default function(ctx) {"
            + "ctx.setGlobalProps({label:'hello 👋'});"
            + "ctx.loadTemplate('test://page', 'template/page.bin', {title:'fixture'});"
            + "ctx.sharedData('answer', 42);"
            + "ctx.sharedData('answer\\u0000suffix', 43);"
            + "ctx.after(25, () => ctx.sendGlobalEvent({arguments:['ready',[]]}));"
            + "ctx.loadTemplate('test://second', 'template/page.bin', {});}",
        "assets/template/page.bin", "template bytes", "config.json", "{\"enablePreDecode\":true}",
        "component_list.json", "[]");
    LynxRecorderFixture fixture =
        LynxRecorderFixture.open(new ByteArrayInputStream(bytes), mContext.getCacheDir());
    File directory = new File(fixture.directory());
    try {
      assertEquals("hello 👋",
          fixture.actions.getJSONObject(0)
              .getJSONObject("Params")
              .getJSONObject("global_props")
              .getString("label"));
      JSONObject load = fixture.actions.getJSONObject(1).getJSONObject("Params");
      assertEquals("template bytes",
          new String(
              Base64.decode(load.getString("source"), Base64.DEFAULT), StandardCharsets.UTF_8));
      assertEquals("fixture", load.getJSONObject("templateData").getString("title"));
      assertEquals(25, fixture.actions.getJSONObject(2).getLong("RecordMillisecond"));
      assertEquals(42, fixture.sharedData.getInt("answer"));
      assertEquals(43, fixture.sharedData.getInt("answer" + (char) 0 + "suffix"));
      assertSame(load.getString("source"),
          fixture.actions.getJSONObject(3).getJSONObject("Params").getString("source"));
      assertTrue(fixture.config.getBoolean("enablePreDecode"));
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        LynxRecorderActionManager manager =
            new LynxRecorderActionManager(new Intent(), mContext, new FrameLayout(mContext), null);
        try {
          Field field = LynxRecorderActionManager.class.getDeclaredField("mDataProvider");
          field.setAccessible(true);
          Object provider = field.get(manager);
          Method setFixture =
              provider.getClass().getDeclaredMethod("setFixture", LynxRecorderFixture.class);
          setFixture.setAccessible(true);
          setFixture.invoke(provider, fixture.retain());
          manager.prepareReplayTimeScript();
          assertEquals(0, manager.getTestBenchPreloadScripts().length);
          setFixture.invoke(provider, (Object) null);
          manager.prepareReplayTimeScript();
          assertEquals(1, manager.getTestBenchPreloadScripts().length);
          assertTrue(manager.getTestBenchPreloadScripts()[0].startsWith("file://"));
          setFixture.invoke(provider, fixture.retain());
          manager.prepareReplayTimeScript();
          assertEquals(0, manager.getTestBenchPreloadScripts().length);
        } catch (ReflectiveOperationException e) {
          throw new AssertionError(e);
        } finally {
          manager.destroy();
        }
      });
      fixture.retain();
      fixture.release();
      assertTrue(directory.isDirectory());
    } finally {
      fixture.release();
    }
    assertFalse(directory.exists());
  }

  @Test
  public void schedulesFixtureAgainstOneUptimeOrigin() {
    assertEquals(125, LynxRecorderActionManager.fixtureTimeAfter(100, 25));
    assertEquals(100, LynxRecorderActionManager.fixtureTimeAfter(100, -1));
    long latest = LynxRecorderActionManager.fixtureTimeAfter(100, Long.MAX_VALUE);
    assertEquals(Long.MAX_VALUE, latest);
    assertEquals(Long.MAX_VALUE, LynxRecorderActionManager.fixtureTimeAfter(latest, 3500));
  }

  @Test
  public void rejectsInvalidPackagesAndRemovesPartialFiles() throws Exception {
    File cache = File.createTempFile("fixture-invalid-", "", mContext.getCacheDir());
    assertTrue(cache.delete());
    assertTrue(cache.mkdir());
    try {
      byte[][] packages = {
          zip("../escaped", "bad", "fixture.js", "export default function(ctx) {}"),
          zip("fixture.js", "while (true) {}"),
          zip("fixture.js",
              "export default function(ctx) {ctx.loadTemplate('test://page','../bad',{});}"),
          zip("config.json", "{}"),
          zip("bad/", "directory payload", "fixture.js", "export default function(ctx) {}"),
      };
      for (byte[] bytes : packages) {
        try {
          LynxRecorderFixture.open(new ByteArrayInputStream(bytes), cache);
          fail("Invalid package accepted");
        } catch (java.io.IOException expected) {
          assertEquals(0, cache.list().length);
        }
      }
    } finally {
      assertTrue(cache.delete());
    }
  }
}
