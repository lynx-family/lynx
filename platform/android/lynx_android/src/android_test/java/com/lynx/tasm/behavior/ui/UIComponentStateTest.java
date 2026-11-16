// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.withSettings;

import com.lynx.devtoolwrapper.DevToolLifecycle;
import com.lynx.devtoolwrapper.DevToolSettings;
import com.lynx.devtoolwrapper.DevToolState;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import org.junit.Test;

public class UIComponentStateTest {
  @Test
  public void stateQueryUsesNormalUIMethodDispatch() throws Exception {
    DevToolSettings.inst().init(TestingUtils.getLynxContext());
    DevToolLifecycle lifecycle = DevToolLifecycle.getInstance();
    Field state = DevToolLifecycle.class.getDeclaredField("mState");
    state.setAccessible(true);
    DevToolState previousState = lifecycle.getState();
    boolean wasEnabled = DevToolSettings.inst().isDevToolEnabled();
    try {
      state.set(lifecycle, DevToolState.ENABLED);
      DevToolSettings.inst().setDevToolEnabled(true);
      LynxBaseUI ui = mock(
          LynxBaseUI.class, withSettings().extraInterfaces(UIComponentInspectorProvider.class));
      Callback callback = mock(Callback.class);
      JavaOnlyMap snapshot = new JavaOnlyMap();
      snapshot.putString("state", "ready");
      when(((UIComponentInspectorProvider) ui).getSnapshot()).thenReturn(snapshot);
      doCallRealMethod().when(ui).getComponentState(callback);

      new LynxBaseUI$$MethodInvoker().invoke(ui, "getComponentState", null, callback);

      verify(callback).invoke(LynxUIMethodConstants.SUCCESS, snapshot);
      LynxBaseUI unsupported = mock(LynxBaseUI.class);
      Callback unsupportedCallback = mock(Callback.class);
      doCallRealMethod().when(unsupported).getComponentState(unsupportedCallback);
      unsupported.getComponentState(unsupportedCallback);
      verify(unsupportedCallback).invoke(LynxUIMethodConstants.METHOD_NOT_FOUND);
    } finally {
      DevToolSettings.inst().setDevToolEnabled(wasEnabled);
      state.set(lifecycle, previousState);
    }
  }

  @Test
  public void disabledDevtoolDoesNotQueryProvider() {
    DevToolSettings settings = DevToolSettings.inst();
    boolean wasEnabled = settings.isDevToolEnabled();
    try {
      settings.setDevToolEnabled(false);
      LynxBaseUI ui = mock(
          LynxBaseUI.class, withSettings().extraInterfaces(UIComponentInspectorProvider.class));
      Callback callback = mock(Callback.class);
      doCallRealMethod().when(ui).getComponentState(callback);

      ui.getComponentState(callback);

      verify(callback).invoke(LynxUIMethodConstants.METHOD_NOT_FOUND);
      verify((UIComponentInspectorProvider) ui, never()).getSnapshot();
    } finally {
      settings.setDevToolEnabled(wasEnabled);
    }
  }
}
