// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
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
public class UIAnimaXInspectorTest {
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
    mListener = getField("mInspectorAnimationListener");
  }

  @Test
  public void resourceBecomesReadyOnlyAfterFirstFrame() {
    assertTrue(mUI.getSnapshot().isNull("frameCount"));
    mListener.onReady(param(10, 100.5));
    assertEquals("loading", mUI.getSnapshot().getString("state"));
    mListener.onFirstFrame(param(10, 100.5));
    assertEquals("ready", mUI.getSnapshot().getString("state"));
    assertEquals(100.5, mUI.getSnapshot().getDouble("frameCount"), 0);
  }

  @Test
  public void sourceReplacementClearsObservationsAndKeepsIdenticalSourceGeneration() {
    mUI.setSrc("first.json");
    mPropsSetter.flush();
    mListener.onReady(param(0, 100));
    mListener.onFirstFrame(param(0, 100));
    mUI.setSrc("first.json");
    mPropsSetter.flush();
    assertEquals(1, mUI.getSnapshot().getLong("resourceGeneration"));
    assertEquals("ready", mUI.getSnapshot().getString("state"));
    mUI.setSrc("second.json");
    mPropsSetter.flush();
    assertEquals(2, mUI.getSnapshot().getLong("resourceGeneration"));
    assertEquals("loading", mUI.getSnapshot().getString("state"));
    assertTrue(mUI.getSnapshot().isNull("frameCount"));
  }

  @Test
  public void failureReportsOnlyErrorCode() {
    AnimaXErrorParam error = mock(AnimaXErrorParam.class);
    when(error.getErrorCode()).thenReturn(17);
    mListener.onError(error);
    assertEquals("error", mUI.getSnapshot().getString("state"));
    assertEquals(17, mUI.getSnapshot().getInt("errorCode"));
    assertTrue(mUI.getSnapshot().isNull("frameCount"));
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
