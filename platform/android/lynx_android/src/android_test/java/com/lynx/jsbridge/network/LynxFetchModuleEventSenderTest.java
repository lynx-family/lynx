// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge.network;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.verifyNoInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.annotation.NonNull;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtoolwrapper.LynxBaseInspectorController;
import com.lynx.devtoolwrapper.LynxDevtool;
import com.lynx.devtoolwrapper.LynxNetworkRequestObserver;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.LynxBackgroundRuntime;
import com.lynx.tasm.LynxBackgroundRuntimeOptions;
import com.lynx.tasm.LynxView;
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
  public void testGetNetworkRequestObserverWithoutReceivers() {
    assertNull(sender.getNetworkRequestObserver());
    sender.sendGlobalEvent("test_event", new JavaOnlyArray());
  }

  @Test
  public void testGetNetworkRequestObserverFromContext() {
    LynxContext lynxContext = mock(LynxContext.class);
    LynxView view = mock(LynxView.class);
    LynxBaseInspectorController controller = mock(LynxBaseInspectorController.class);
    LynxNetworkRequestObserver observer = mock(LynxNetworkRequestObserver.class);
    sender.setWeakContext(lynxContext);
    assertNull(sender.getNetworkRequestObserver());

    when(lynxContext.getLynxView()).thenReturn(view);
    assertNull(sender.getNetworkRequestObserver());

    when(view.getBaseInspectorController()).thenReturn(controller);
    assertNull(sender.getNetworkRequestObserver());

    when(controller.getNetworkRequestObserver()).thenReturn(observer);
    assertSame(observer, sender.getNetworkRequestObserver());
  }

  @Test
  public void testGetNetworkRequestObserverFromRuntime() {
    LynxBackgroundRuntime runtime = mock(LynxBackgroundRuntime.class);
    LynxDevtool devtool = mock(LynxDevtool.class);
    LynxBaseInspectorController controller = mock(LynxBaseInspectorController.class);
    LynxNetworkRequestObserver observer = mock(LynxNetworkRequestObserver.class);
    sender.setWeakRuntime(runtime);
    assertNull(sender.getNetworkRequestObserver());

    when(runtime.getDevtool()).thenReturn(devtool);
    assertNull(sender.getNetworkRequestObserver());

    when(devtool.getBaseInspectorController()).thenReturn(controller);
    assertNull(sender.getNetworkRequestObserver());

    when(controller.getNetworkRequestObserver()).thenReturn(observer);
    assertSame(observer, sender.getNetworkRequestObserver());

    sender.setWeakRuntime(null);
    assertNull(sender.getNetworkRequestObserver());
  }

  @Test
  public void testContextTakesPriorityOverRuntime() {
    LynxContext lynxContext = mock(LynxContext.class);
    LynxBackgroundRuntime runtime = mock(LynxBackgroundRuntime.class);
    sender.setWeakContext(lynxContext);
    sender.setWeakRuntime(runtime);

    // A present context owns routing even when its inspector is unavailable.
    assertNull(sender.getNetworkRequestObserver());
    JavaOnlyArray params = new JavaOnlyArray();
    sender.sendGlobalEvent("test_event", params);
    verify(lynxContext).sendGlobalEvent("test_event", params);
    verifyNoInteractions(runtime);

    sender.setWeakContext(null);
    sender.sendGlobalEvent("runtime_event", params);
    verify(runtime).sendGlobalEvent("runtime_event", params);
    assertNull(sender.getNetworkRequestObserver());
    verify(runtime).getDevtool();
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
