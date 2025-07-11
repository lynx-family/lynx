// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.network;

import static org.junit.Assert.*;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.annotation.NonNull;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.LynxBackgroundRuntime;
import com.lynx.tasm.LynxBackgroundRuntimeOptions;
import com.lynx.tasm.behavior.LynxContext;
import org.junit.Before;
import org.junit.Test;

public class LynxFetchModuleEventSenderTest {
  private LynxFetchModuleEventSender sender;
  private Context context;

  // Mock LynxContext implementation
  static class MockLynxContext extends LynxContext {
    public String lastEventName;
    public JavaOnlyArray lastParams;

    public MockLynxContext(Context base, DisplayMetrics screenMetrics) {
      super(base, screenMetrics);
    }

    @Override
    public void sendGlobalEvent(String name, JavaOnlyArray params) {
      this.lastEventName = name;
      this.lastParams = params;
    }

    @Override
    public void handleException(Exception e) {}
  }

  // Mock LynxBackgroundRuntime implementation
  static class MockLynxBackgroundRuntime extends LynxBackgroundRuntime {
    public String lastEventName;
    public JavaOnlyArray lastParams;

    public MockLynxBackgroundRuntime(
        @NonNull Context context, @NonNull LynxBackgroundRuntimeOptions options) {
      super(context, options);
    }

    @Override
    public void sendGlobalEvent(String name, JavaOnlyArray params) {
      this.lastEventName = name;
      this.lastParams = params;
    }
  }

  @Before
  public void setUp() {
    sender = new LynxFetchModuleEventSender();

    context =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
  }

  @Test
  public void testSendGlobalEventWithContext() {
    MockLynxContext mockContext = new MockLynxContext(context, new DisplayMetrics());
    sender.setWeakContext(mockContext);

    JavaOnlyArray params = new JavaOnlyArray();
    sender.sendGlobalEvent("test_event", params);

    assertEquals("test_event", mockContext.lastEventName);
    assertSame(params, mockContext.lastParams);
  }

  @Test
  public void testSendGlobalEventWithRuntime() {
    MockLynxBackgroundRuntime mockRuntime =
        new MockLynxBackgroundRuntime(context, new LynxBackgroundRuntimeOptions());
    sender.setWeakRuntime(mockRuntime);

    JavaOnlyArray params = new JavaOnlyArray();
    sender.sendGlobalEvent("test_event", params);

    assertEquals("test_event", mockRuntime.lastEventName);
    assertSame(params, mockRuntime.lastParams);
  }
}
