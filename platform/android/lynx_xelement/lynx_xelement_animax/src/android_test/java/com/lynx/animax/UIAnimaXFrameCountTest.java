// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Context;
import android.view.View;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.animax.listener.AnimaXErrorParam;
import com.lynx.animax.listener.AnimaXParam;
import com.lynx.animax.listener.IAnimationListener;
import com.lynx.animax.setter.UIAnimaXPropsPrioritySetter;
import com.lynx.animax.ui.AnimaXContainerView;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.util.HashMap;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UIAnimaXFrameCountTest {
  private LynxContext mContext;
  private UIAnimaX mUI;
  private AnimaXPlayer mPlayer;
  private UIAnimaXPropsPrioritySetter mPropsSetter;
  private IAnimationListener mListener;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
    LynxView lynxView = new LynxView(mContext, new LynxViewBuilder());
    lynxView.layout(0, 0, 960, 2000);
    mContext.setLynxView(lynxView);
    mUI = new TestUIAnimaX(mContext);
    mUI.setSign(17, "animax-view");
    lynxView.addView(mUI.getView());
    mUI.getView().layout(0, 0, 720, 1168);
    mPlayer = mock(AnimaXPlayer.class);
    setField("mAnimaXPlayer", mPlayer);
    mPropsSetter = getField("mPropsSetter");
    mPropsSetter.init(mPlayer);
    mListener = getField("mFrameCountListener");
  }

  @Test
  public void totalFrameCountPreservesFractionalNativeValue() {
    mListener.onReady(param(10, 100.5));
    assertEquals(Float.valueOf(100.5f), queryFrameCount());
  }

  @Test
  public void replacementAndFailureInvalidatePreviousFrameCount() {
    mUI.setSrc("first.json");
    mPropsSetter.flush();
    mListener.onReady(param(0, 100));
    mUI.setSrc("first.json");
    mPropsSetter.flush();
    assertEquals(Float.valueOf(100), queryFrameCount());
    mUI.setSrc("second.json");
    mPropsSetter.flush();
    org.junit.Assert.assertNull(queryFrameCount());
    mListener.onReady(param(0, 50));
    mListener.onError(mock(AnimaXErrorParam.class));
    org.junit.Assert.assertNull(queryFrameCount());
  }

  private Object queryFrameCount() {
    com.lynx.devtoolwrapper.DevToolSettings settings =
        com.lynx.devtoolwrapper.DevToolSettings.inst();
    settings.init(mContext);
    com.lynx.devtoolwrapper.DevToolLifecycle lifecycle =
        com.lynx.devtoolwrapper.DevToolLifecycle.getInstance();
    try {
      Field state = lifecycle.getClass().getDeclaredField("mState");
      state.setAccessible(true);
      Object previous = state.get(lifecycle);
      boolean enabled = settings.isDevToolEnabled();
      try {
        state.set(lifecycle, com.lynx.devtoolwrapper.DevToolState.ENABLED);
        settings.setDevToolEnabled(true);
        Object[] value = {null};
        com.lynx.react.bridge.Callback callback = args -> {
          assertEquals(0, args[0]);
          value[0] = args[1];
        };
        new UIAnimaX$$MethodInvoker().invoke(mUI, "getFrameCount", null, callback);
        return value[0];
      } finally {
        settings.setDevToolEnabled(enabled);
        state.set(lifecycle, previous);
      }
    } catch (Exception e) {
      throw new AssertionError(e);
    }
  }

  @Test
  public void existingControlsKeepTheirNativeSemantics() {
    mUI.pause(null, null);
    mUI.play(null, null);
    mUI.stop(null, null);
    verify(mPlayer).pause();
    verify(mPlayer).play();
    verify(mPlayer).stop();
    verify(mPlayer, never()).resume();
    verify(mPlayer, never()).setProgress(0);
    // Current frame and running status remain available through their existing UIMethods.
    when(mPlayer.getCurrentFrame()).thenReturn(42.5);
    com.lynx.react.bridge.Callback callback = mock(com.lynx.react.bridge.Callback.class);
    mUI.getCurrentFrame(null, callback);
    verify(callback).invoke(com.lynx.tasm.behavior.LynxUIMethodConstants.SUCCESS, 42.5);
  }

  private static AnimaXParam param(double current, double total) {
    HashMap<String, Object> params = new HashMap<>();
    params.put("current", current);
    params.put("total", total);
    return new AnimaXParam(params);
  }

  private void setField(String name, Object value) throws Exception {
    Field field = UIAnimaX.class.getDeclaredField(name);
    field.setAccessible(true);
    field.set(mUI, value);
  }

  @SuppressWarnings("unchecked")
  private <T> T getField(String name) throws Exception {
    Field field = UIAnimaX.class.getDeclaredField(name);
    field.setAccessible(true);
    return (T) field.get(mUI);
  }

  private static class TestUIAnimaX extends UIAnimaX {
    TestUIAnimaX(LynxContext context) {
      super(context);
    }

    @Override
    protected View createView(Context context) {
      return new AnimaXContainerView(context);
    }
  }
}
