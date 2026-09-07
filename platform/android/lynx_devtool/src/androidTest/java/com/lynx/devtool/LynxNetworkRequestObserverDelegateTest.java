// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertSame;

import android.app.Application;
import android.content.Context;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtoolwrapper.LynxNetworkRequestObserver;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxEnv;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxNetworkRequestObserverDelegateTest {
  @Test
  public void testZeroHandleIgnoresNetworkEvents() {
    assertDisabledObserver(new LynxNetworkRequestObserverDelegate(0, new Object()));
  }

  @Test
  public void testInvalidateClearsHandleAndIsIdempotent() {
    // The sentinel must never reach JNI: invalidate must clear it first.
    LynxNetworkRequestObserverDelegate observer =
        new LynxNetworkRequestObserverDelegate(1, new Object());
    observer.invalidate();
    assertDisabledObserver(observer);
    observer.invalidate();
    assertDisabledObserver(observer);
  }

  @Test
  public void testLiveHandleAndCallbacksAfterDestroy() {
    Context context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) context, System::loadLibrary, null, null, null);
    LynxDevtoolEnv.inst().init(context);
    LynxDevToolNGDelegate delegate = new LynxDevToolNGDelegate(true);
    LynxNetworkRequestObserver observer = delegate.getNetworkRequestObserver();
    try {
      assertSame(observer, delegate.getNetworkRequestObserver());
      // Without a Network.enable session, a live native owner must ignore events.
      // This exercises every JNI entry point with a real, owned native handle.
      assertDisabledObserver(observer);
    } finally {
      delegate.destroy();
    }
    // Fetch may retain the observer and finish asynchronously after destruction.
    assertDisabledObserver(observer);
    delegate.destroy();
    assertDisabledObserver(observer);
  }

  private static void assertDisabledObserver(LynxNetworkRequestObserver observer) {
    assertFalse(observer.isEnabled());
    JavaOnlyMap headers = new JavaOnlyMap();
    headers.putString("Content-Type", "text/plain");
    byte[] data = new byte[] {65, 66};
    assertEquals("", observer.requestWillBeSent("https://example.com", "POST", headers, data));
    observer.responseReceived("request", "https://example.com", 200, "OK", headers);
    observer.dataReceived("request", data);
    observer.loadingFinished("request");
    observer.loadingFailed("request", "connection closed", false);
    observer.loadingFailed("request", "canceled", true);
    assertFalse(observer.isEnabled());
  }
}
