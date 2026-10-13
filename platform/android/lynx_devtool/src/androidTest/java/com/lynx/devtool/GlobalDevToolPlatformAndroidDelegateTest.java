// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.app.Application;
import android.os.Looper;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtoolwrapper.DevToolLifecycle;
import com.lynx.tasm.LynxEnv;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.json.JSONObject;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class GlobalDevToolPlatformAndroidDelegateTest {
  @BeforeClass
  public static void setUpRuntime() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      Application app = (Application) InstrumentationRegistry.getInstrumentation()
                            .getTargetContext()
                            .getApplicationContext();
      LynxEnv.inst().prepareDevToolForDevelopmentBeforeInit();
      LynxEnv.inst().init(app, System::loadLibrary, null, null, null);
      // Library tests initialize DevTool directly without a host service registration.
      DevToolLifecycle.getInstance().onAttached();
      DevToolLifecycle.getInstance().onEnabled();
      LynxDevtoolEnv.inst().init(app);
    });
  }

  @Test
  public void nativeLoadCompletesOnUiThread() throws Exception {
    assertNull(
        load(new JSONObject().put("target", "script").put("script", "globalThis.probe = 42;")));
  }

  @Test
  public void nativeValidationRejectsConflictingSource() throws Exception {
    assertEquals("Unexpected schema url for target=script",
        load(new JSONObject().put("target", "script").put("script", "1").put("url", "url")));
  }

  @Test
  public void nativeExceptionPreservesUnicode() throws Exception {
    String message = "\u4e2d\u6587\ud83d\ude00";
    String error = load(new JSONObject()
                            .put("target", "script")
                            .put("script", "throw new Error('" + message + "');"));
    assertTrue(error, error != null && error.contains(message));
  }

  private String load(JSONObject params) throws Exception {
    CountDownLatch done = new CountDownLatch(1);
    AtomicReference<String> result = new AtomicReference<>();
    AtomicBoolean onUiThread = new AtomicBoolean();
    GlobalDevToolPlatformAndroidDelegate.loadHSRScriptFromSchema(params.toString(), error -> {
      result.set(error);
      onUiThread.set(Looper.myLooper() == Looper.getMainLooper());
      done.countDown();
    });
    assertTrue("HSR callback timed out", done.await(10, TimeUnit.SECONDS));
    assertTrue("HSR completion must return to UI", onUiThread.get());
    return result.get();
  }
}
