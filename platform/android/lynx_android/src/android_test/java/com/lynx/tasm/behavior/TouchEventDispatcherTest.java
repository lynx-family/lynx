// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;

import android.graphics.Matrix;
import android.os.SystemClock;
import android.view.MotionEvent;
import com.lynx.react.bridge.DynamicFromArray;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.LynxEventEmitter;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.event.EventTargetBase;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.Map;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class TouchEventDispatcherTest {
  private LynxContext mContext;
  private LynxUIOwner mOwner;
  private TouchEventDispatcher mDispatcher;
  private UIBody mRootUI;

  public static class MockEventTarget implements EventTarget {
    public int mSign;

    MockEventTarget(int sign) {
      mSign = sign;
    }

    @Override
    public int getSign() {
      return mSign;
    }

    @Override
    public int getPseudoStatus() {
      return 0;
    }

    @Override
    public int getGestureArenaMemberId() {
      return 0;
    }

    @Override
    public EventTarget parent() {
      return null;
    }

    @Override
    public EventTarget hitTest(float x, float y) {
      return null;
    }

    @Override
    public EventTarget hitTest(float x, float y, boolean ignoreUserInteraction) {
      return null;
    }

    @Override
    public boolean containsPoint(float x, float y) {
      return true;
    }
    @Override
    public boolean containsPoint(float x, float y, boolean ignoreUserInteraction) {
      return true;
    }

    @Override
    public Map<String, EventsListener> getEvents() {
      return null;
    }

    @Override
    public Map<Integer, GestureDetector> getGestureDetectorMap() {
      return null;
    }

    @Override
    public Matrix getTransformMatrix() {
      return null;
    }

    @Override
    public boolean isUserInteractionEnabled() {
      return true;
    }

    @Override
    public boolean ignoreFocus() {
      return false;
    }

    @Override
    public boolean isFocusable() {
      return false;
    }

    @Override
    public boolean isScrollable() {
      return false;
    }

    @Override
    public boolean isClickable() {
      return true;
    }

    @Override
    public boolean isLongClickable() {
      return true;
    }

    @Override
    public boolean enableTouchPseudoPropagation() {
      return true;
    }

    @Override
    public void onPseudoStatusChanged(int preStatus, int currentStatus) {}

    @Override
    public void onFocusChanged(boolean hasFocus, boolean isFocusTransition) {}

    @Override
    public void onResponseChain() {}

    @Override
    public void offResponseChain() {}

    @Override
    public boolean isOnResponseChain() {
      return true;
    }

    @Override
    public boolean consumeSlideEvent(float angle) {
      return false;
    }

    @Override
    public boolean hasConsumeSlideEventAngles() {
      return false;
    }

    @Override
    public boolean blockNativeEvent(MotionEvent ev) {
      return false;
    }

    @Override
    public boolean dispatchEvent(LynxEventDetail event) {
      return false;
    }

    @Override
    public boolean dispatchTouch(MotionEvent ev) {
      return true;
    }

    @Override
    public boolean eventThrough() {
      return true;
    }

    @Override
    public EventTargetBase parentResponder() {
      return null;
    }

    @Override
    public ReadableMap getDataset() {
      return null;
    }
  }

  @Before
  public void setUp() throws Exception {
    try {
      mContext = TestingUtils.getLynxContext();
      UIBody.UIBodyView view = new UIBody.UIBodyView(mContext);
      LynxTemplateRender renderer = mock(LynxTemplateRender.class);
      mContext.setEventEmitter(new LynxEventEmitter(renderer.getEngineProxy()));
      mOwner = new LynxUIOwner(mContext, null, view);
      mRootUI = new UIBody(mContext, new UIBody.UIBodyView(mContext));
      mDispatcher = new TouchEventDispatcher(mOwner);
    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  @After
  public void tearDown() throws Exception {
    try {
      mContext = null;
      mOwner = null;
      mDispatcher = null;
    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  @Test
  public void testOnActionMove() {
    try {
      LinkedList<EventTarget> pre = new LinkedList<>();
      for (int i = 0; i < 10; ++i) {
        pre.add(new MockEventTarget(i));
      }

      Field field = mDispatcher.getClass().getDeclaredField("mActiveUIList");
      field.setAccessible(true);
      field.set(mDispatcher, pre);

      MockEventTarget newTarget = new MockEventTarget(11);

      MotionEvent ev = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
          MotionEvent.ACTION_MOVE, 100, 100, 0);
      mDispatcher.onActionMove(ev, newTarget);

      assertEquals(0, ((LinkedList) field.get(mDispatcher)).size(), 0);
    } catch (Throwable e) {
      e.printStackTrace();
      assertEquals(1, 0, 0);
    }
  }

  @Test
  public void testConsumeSlideEvent() {
    try {
      MotionEvent ev = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
          MotionEvent.ACTION_DOWN, 100, 100, 0);

      LynxBaseUI rootUI = mOwner.getRootUI();
      UIBody.UIBodyView rootView = mOwner.getRootUI().getView();

      final boolean[] mCalled = {false};
      AndroidView parentView = new AndroidView(mContext) {
        @Override
        public void requestDisallowInterceptTouchEvent(boolean disallowIntercept) {
          mCalled[0] = true;
          super.requestDisallowInterceptTouchEvent(disallowIntercept);
        }
      };
      parentView.addView(rootView);

      Field field =
          rootUI.getClass().getSuperclass().getSuperclass().getSuperclass().getDeclaredField(
              "mConsumeSlideEventAngles");
      field.setAccessible(true);
      ArrayList<ArrayList<Float>> angles = new ArrayList<>();
      ArrayList<Float> array = new ArrayList<>();
      array.add(-180.0f);
      array.add(180.0f);
      angles.add(array);
      field.set(rootUI, angles);

      field = mDispatcher.getClass().getDeclaredField("mActiveUI");
      field.setAccessible(true);
      field.set(mDispatcher, rootUI);

      assertFalse(mCalled[0]);
      assertFalse(mDispatcher.consumeSlideEvent(ev));
      assertTrue(mCalled[0]);
    } catch (Throwable e) {
      e.printStackTrace();
      fail();
    }
  }

  @Test
  public void testConsumeSlideEvent1() {
    try {
      MotionEvent ev = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
          MotionEvent.ACTION_DOWN, 100, 100, 0);

      UIBody.UIBodyView view = mOwner.getRootUI().getView();

      final boolean[] mCalled = {false};
      AndroidView parentView = new AndroidView(mContext) {
        @Override
        public void requestDisallowInterceptTouchEvent(boolean disallowIntercept) {
          mCalled[0] = true;
          super.requestDisallowInterceptTouchEvent(disallowIntercept);
        }
      };
      parentView.addView(view);

      assertFalse(mCalled[0]);
      assertFalse(mDispatcher.consumeSlideEvent(ev));
      assertFalse(mCalled[0]);
    } catch (Throwable e) {
      e.printStackTrace();
      fail();
    }
  }

  @Test
  public void testEventThrough() {
    UIView parentUI = new UIView(mContext);
    mRootUI.insertChild(parentUI, 0);
    UIView childUI = new UIView(mContext);
    parentUI.insertChild(childUI, 0);
    assertFalse(childUI.eventThrough());
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushBoolean(true);
    DynamicFromArray param = new DynamicFromArray(array, 0);
    parentUI.setEventThrough(param);
    assertTrue(childUI.eventThrough());
  }

  @Test
  public void testIgnoreFocus() {
    UIView parentUI = new UIView(mContext);
    mRootUI.insertChild(parentUI, 0);
    UIView childUI = new UIView(mContext);
    parentUI.insertChild(childUI, 0);
    assertFalse(childUI.ignoreFocus());
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushBoolean(true);
    DynamicFromArray param = new DynamicFromArray(array, 0);
    parentUI.setIgnoreFocus(param);
    assertTrue(childUI.ignoreFocus());
  }
}
