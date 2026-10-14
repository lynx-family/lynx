// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.recorder;

import static org.junit.Assert.*;

import android.app.Application;
import android.content.Context;
import android.content.Intent;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.SystemClock;
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
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;
import org.json.JSONArray;
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
            + "ctx.sharedData('answer\\\\u0000suffix', 43);"
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
      assertEquals(43, fixture.sharedData.getInt("answer\\u0000suffix"));
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
      // The JS context may outlive both the replay view and the Java module.
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        LynxRecorderActionManager manager =
            new LynxRecorderActionManager(new Intent(), mContext, new FrameLayout(mContext), null);
        LynxRecorderReplayDataModule module = null;
        try {
          Object provider = managerField("mDataProvider").get(manager);
          Method setter =
              provider.getClass().getDeclaredMethod("setFixture", LynxRecorderFixture.class);
          setter.setAccessible(true);
          setter.invoke(provider, fixture.retain());
          module = new LynxRecorderReplayDataModule(null, provider);
          assertEquals(fixture.directory(), module.getFixtureDirectory());
          manager.destroy();
          module.destroy();
          fixture.release();
          assertTrue(directory.isDirectory());
          assertEquals("", module.getFixtureDirectory());
          module.releaseFixture();
          assertFalse(directory.exists());
        } catch (ReflectiveOperationException e) {
          throw new AssertionError(e);
        } finally {
          if (module != null) {
            module.releaseFixture();
            module.destroy();
          }
          manager.destroy();
        }
      });
    } finally {
      fixture.release();
    }
    assertFalse(directory.exists());
  }

  @Test
  public void legacyProviderKeepsJsonReplayWithoutFixture() throws Exception {
    LynxRecorderReplayDataProvider provider = new LynxRecorderReplayDataProvider() {
      @Override
      public JSONArray getFunctionCall() {
        return new JSONArray();
      }

      @Override
      public JSONObject getCallbackData() {
        return new JSONObject();
      }

      @Override
      public JSONArray getActionList() {
        return new JSONArray();
      }

      @Override
      public JSONArray getJsbIgnoredInfo() {
        return new JSONArray();
      }

      @Override
      public JSONObject getJsbSettings() {
        return new JSONObject();
      }

      @Override
      public JSONObject getSharedData() {
        return new JSONObject();
      }
    };
    LynxRecorderReplayDataModule module = new LynxRecorderReplayDataModule(null, provider);
    try {
      assertEquals("", module.getFixtureDirectory());
      String[] result = new String[1];
      module.getData(args -> result[0] = (String) args[0]);
      JSONObject data = new JSONObject(result[0]);
      assertEquals("{}", data.getString("RecordData"));
      assertEquals("[]", data.getString("JsbIgnoredInfo"));
      assertEquals("{}", data.getString("JsbSettings"));
      module.releaseFixture();
    } finally {
      module.destroy();
    }
    assertEquals("", module.getFixtureDirectory());
  }

  @Test
  public void saturatesReplayDeadlines() {
    assertEquals(125, LynxRecorderActionManager.replayTimeAfter(100, 25));
    assertEquals(100, LynxRecorderActionManager.replayTimeAfter(100, -1));
    long latest = LynxRecorderActionManager.replayTimeAfter(100, Long.MAX_VALUE);
    assertEquals(Long.MAX_VALUE, latest);
    assertEquals(Long.MAX_VALUE, LynxRecorderActionManager.replayTimeAfter(latest, 3500));
  }

  private static Field managerField(String name) throws ReflectiveOperationException {
    Field field = LynxRecorderActionManager.class.getDeclaredField(name);
    field.setAccessible(true);
    return field;
  }

  @Test
  public void fixtureAndJsonWaitForFirstScreenBeforeInteractions() throws Exception {
    byte[] bytes = zip("fixture.js", "export default function(ctx) {}");
    LynxRecorderFixture fixture =
        LynxRecorderFixture.open(new ByteArrayInputStream(bytes), mContext.getCacheDir());
    try {
      InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
        for (boolean useFixture : new boolean[] {false, true}) {
          LynxRecorderActionManager manager = new LynxRecorderActionManager(
              new Intent(), mContext, new FrameLayout(mContext), null);
          List<Message> messages = new ArrayList<>();
          List<Long> deadlines = new ArrayList<>();
          try {
            managerField("mHandler").set(manager, new Handler(Looper.getMainLooper()) {
              @Override
              public boolean sendMessageAtTime(Message message, long uptimeMillis) {
                messages.add(Message.obtain(message));
                deadlines.add(uptimeMillis);
                return true;
              }
            });
            managerField("mReplayGesture").setBoolean(manager, true);
            if (useFixture) {
              Object provider = managerField("mDataProvider").get(manager);
              Method setter =
                  provider.getClass().getDeclaredMethod("setFixture", LynxRecorderFixture.class);
              setter.setAccessible(true);
              setter.invoke(provider, fixture.retain());
            }
            JSONArray actions = new JSONArray();
            String[] names = {"setGlobalProps", "SendCustomEvent", "SendTouchEvent",
                "sendEventAndroid", "sendGlobalEvent", "updateDataByPreParsedData"};
            for (int i = 0; i < names.length; ++i) {
              long delay = i < 3 ? 0 : (i - 1) * 100;
              actions.put(new JSONObject()
                              .put("Function Name", names[i])
                              .put("Record Time", 0)
                              .put("RecordMillisecond", (useFixture ? 0 : 1000000) + delay)
                              .put("Params", new JSONObject().put("action", names[i])));
            }
            Method handle = LynxRecorderActionManager.class.getDeclaredMethod(
                "handleActionList", JSONArray.class);
            handle.setAccessible(true);
            handle.invoke(manager, actions);
            // Reloading the package must not duplicate its deferred interactions.
            for (Message message : messages) message.recycle();
            messages.clear();
            deadlines.clear();
            handle.invoke(manager, actions);
            // Only setup and completion notification are queued before the first screen.
            assertEquals(2, messages.size());
            assertEquals("setGlobalProps", ((JSONObject) messages.get(0).obj).getString("action"));
            assertEquals(managerField("ON_TESTBENCH_COMPLETE").getInt(null), messages.get(1).what);
            assertEquals(3, ((List<?>) managerField("mReloadList").get(manager)).size());
            // Simulate a first screen slower than the recorded input delay.
            SystemClock.sleep(450);
            long firstScreen = SystemClock.uptimeMillis();
            manager.reloadAction();
            long scheduled = SystemClock.uptimeMillis();
            assertEquals(6, messages.size());
            for (int i = 0; i < 3; ++i) {
              assertEquals(
                  names[i + 3], ((JSONObject) messages.get(i + 2).obj).getString("action"));
              assertTrue(deadlines.get(i + 2) >= firstScreen + (i + 2) * 100);
              assertTrue(deadlines.get(i + 2) <= scheduled + (i + 2) * 100);
            }
            assertEquals(managerField("END_TESTBENCH").getInt(null), messages.get(5).what);
            assertTrue(deadlines.get(5) >= firstScreen + 400 + 3500);
          } catch (Exception e) {
            throw new AssertionError(e);
          } finally {
            manager.destroy();
            for (Message message : messages) message.recycle();
          }
        }
      });
    } finally {
      fixture.release();
    }
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
