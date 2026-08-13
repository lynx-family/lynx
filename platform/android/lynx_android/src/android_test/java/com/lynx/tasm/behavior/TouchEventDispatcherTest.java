// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyFloat;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.argThat;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.Matrix;
import android.graphics.PointF;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.widget.EditText;
import com.lynx.react.bridge.DynamicFromArray;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.EventEmitter;
import com.lynx.tasm.LynxEventEmitter;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.event.EventTargetBase;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.event.EventsListener;
import com.lynx.tasm.event.LynxEventDetail;
import com.lynx.tasm.event.LynxTouchEvent;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.handler.GestureConstants;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.InOrder;

public class TouchEventDispatcherTest {
  private LynxContext mContext;
  private LynxUIOwner mOwner;
  private TouchEventDispatcher mDispatcher;
  private UIBody mRootUI;

  public static class MockEventTarget implements EventTarget {
    public int mSign;
    private HashMap<String, EventTarget> mChildrenLynxPageUI;

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
    public boolean eventThrough(float x, float y) {
      return false;
    }

    @Override
    public PointerEventsValue pointerEvents() {
      return PointerEventsValue.Auto;
    }

    @Override
    public PanInterceptDirection panInterceptDirection() {
      return PanInterceptDirection.None;
    }

    @Override
    public PanInterceptScope panInterceptScope() {
      return PanInterceptScope.None;
    }

    @Override
    public void setPanInterceptSelf(boolean panInterceptSelf) {}

    @Override
    public void setPanInterceptAncestors(boolean panInterceptAncestors) {}

    @Override
    public void setPanInterceptDescendants(boolean panInterceptDescendants) {}

    @Override
    public EventTargetBase parentResponder() {
      return null;
    }

    @Override
    public ReadableMap getDataset() {
      return null;
    }

    @Override
    public EventTarget getParentLynxPageUI() {
      return null;
    }

    @Override
    public void setParentLynxPageUI(EventTarget parentLynxPageUI) {}

    @Override
    public HashMap<String, EventTarget> getChildrenLynxPageUI() {
      return mChildrenLynxPageUI;
    }

    @Override
    public void setChildrenLynxPageUI(HashMap<String, EventTarget> childrenLynxPageUI) {
      mChildrenLynxPageUI = childrenLynxPageUI;
    }

    @Override
    public EventTarget getRootLynxPageUI() {
      return null;
    }

    @Override
    public void setEventID(long eventID) {}

    @Override
    public void startEventCapture(long eventID) {}

    @Override
    public void onEventCapture(boolean isCapture, long eventID) {}

    @Override
    public void startEventBubble(long eventID) {}

    @Override
    public void onEventBubble(boolean isCapture, long eventID) {}

    @Override
    public void startEventFire(boolean isStop, long eventID) {}

    @Override
    public void onEventFire(boolean isStop, long eventID) {}
  }

  private static class MockGestureArenaManager extends GestureArenaManager {
    private boolean mHasActivePlatformGesture;

    void setHasActivePlatformGesture(boolean hasActive) {
      mHasActivePlatformGesture = hasActive;
    }

    @Override
    public boolean hasActivePlatformGesture() {
      return mHasActivePlatformGesture;
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
  public void testTouchStartIsDispatchedBeforePointerDown() {
    EventEmitter eventEmitter = mock(EventEmitter.class);
    mContext.setEventEmitter(eventEmitter);
    UIGroup root = mock(UIGroup.class);
    LynxBaseUI target = mock(LynxBaseUI.class);
    when(target.getSign()).thenReturn(17);
    when(root.hitTest(anyFloat(), anyFloat())).thenReturn(target);
    MotionEvent event = MotionEvent.obtain(
        SystemClock.uptimeMillis(), SystemClock.uptimeMillis(), MotionEvent.ACTION_DOWN, 10, 20, 0);

    assertTrue(mDispatcher.handleFirstTouchDown(event, root));

    InOrder order = inOrder(eventEmitter);
    order.verify(eventEmitter)
        .sendTouchEvent(argThat(touchEvent -> "touchstart".equals(touchEvent.getName())));
    order.verify(eventEmitter)
        .sendBubbleEvent(eq("pointerdown"), eq(17), any(JavaOnlyMap.class));
    event.recycle();
  }

  @Test
  public void testNativeTextInputDoesNotSynthesizeKeyboardClick() {
    EventEmitter eventEmitter = mock(EventEmitter.class);
    mContext.setEventEmitter(eventEmitter);
    LynxUI target = mock(LynxUI.class);
    when(target.getSign()).thenReturn(17);
    when(target.getView()).thenReturn(mock(EditText.class));
    mDispatcher.setFocusedUI(target);

    mDispatcher.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER));
    mDispatcher.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER));

    verify(eventEmitter)
        .sendBubbleEvent(eq("keydown"), eq(17), any(JavaOnlyMap.class));
    verify(eventEmitter)
        .sendBubbleEvent(eq("keyup"), eq(17), any(JavaOnlyMap.class));
    verify(eventEmitter, never()).sendTouchEvent(any(LynxTouchEvent.class));
  }

  @Test
  public void testCanceledActivationKeyDoesNotSynthesizeClick() {
    EventEmitter eventEmitter = mock(EventEmitter.class);
    mContext.setEventEmitter(eventEmitter);
    LynxBaseUI target = mock(LynxBaseUI.class);
    when(target.getSign()).thenReturn(17);
    mDispatcher.setFocusedUI(target);
    KeyEvent canceledUp = new KeyEvent(0, 1, KeyEvent.ACTION_UP, KeyEvent.KEYCODE_ENTER, 0, 0,
        -1, 0, KeyEvent.FLAG_CANCELED);

    mDispatcher.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_ENTER));
    mDispatcher.dispatchKeyEvent(canceledUp);

    verify(eventEmitter)
        .sendBubbleEvent(eq("keydown"), eq(17), any(JavaOnlyMap.class));
    verify(eventEmitter)
        .sendBubbleEvent(eq("keyup"), eq(17), any(JavaOnlyMap.class));
    verify(eventEmitter, never()).sendTouchEvent(any(LynxTouchEvent.class));
  }

  @Test
  public void testWheelTargetsInnermostChildLynxPage() {
    EventEmitter parentEmitter = mock(EventEmitter.class);
    mContext.setEventEmitter(parentEmitter);
    UIGroup parentRoot = mock(UIGroup.class);
    MockEventTarget frameTarget = new MockEventTarget(17);
    when(parentRoot.hitTest(anyFloat(), anyFloat())).thenReturn(frameTarget);

    LynxContext childContext = TestingUtils.getLynxContext();
    EventEmitter childEmitter = mock(EventEmitter.class);
    childContext.setEventEmitter(childEmitter);
    LynxUIOwner childOwner =
        new LynxUIOwner(childContext, null, new UIBody.UIBodyView(childContext));
    TouchEventDispatcher childDispatcher = new TouchEventDispatcher(childOwner);
    childContext.setTouchEventDispatcher(childDispatcher);
    UIBody childRoot = mock(UIBody.class);
    MockEventTarget childTarget = new MockEventTarget(23);
    when(childRoot.getLynxContext()).thenReturn(childContext);
    when(childRoot.hitTest(anyFloat(), anyFloat())).thenReturn(childTarget);
    HashMap<String, EventTarget> children = new HashMap<>();
    children.put(String.valueOf(System.identityHashCode(frameTarget)), childRoot);
    frameTarget.setChildrenLynxPageUI(children);

    MotionEvent.PointerProperties properties = new MotionEvent.PointerProperties();
    properties.id = 0;
    properties.toolType = MotionEvent.TOOL_TYPE_MOUSE;
    MotionEvent.PointerCoords coordinates = new MotionEvent.PointerCoords();
    coordinates.x = 10;
    coordinates.y = 20;
    coordinates.setAxisValue(MotionEvent.AXIS_VSCROLL, 1);
    MotionEvent event = MotionEvent.obtain(0, 1, MotionEvent.ACTION_SCROLL, 1,
        new MotionEvent.PointerProperties[] {properties},
        new MotionEvent.PointerCoords[] {coordinates}, 0, 0, 1, 1, 0, 0,
        InputDevice.SOURCE_MOUSE, 0);

    mDispatcher.onGenericMotionEvent(event, parentRoot);

    verify(childEmitter)
        .sendBubbleEvent(eq("wheel"), eq(23), any(JavaOnlyMap.class));
    verify(parentEmitter, never())
        .sendBubbleEvent(eq("wheel"), anyInt(), any(JavaOnlyMap.class));
    event.recycle();
  }

  @Test
  public void testStylusHoverHasNoPressedButtons() {
    EventEmitter eventEmitter = mock(EventEmitter.class);
    mContext.setEventEmitter(eventEmitter);
    UIGroup root = mock(UIGroup.class);
    MockEventTarget target = new MockEventTarget(17);
    when(root.hitTest(anyFloat(), anyFloat())).thenReturn(target);
    MotionEvent.PointerProperties properties = new MotionEvent.PointerProperties();
    properties.id = 0;
    properties.toolType = MotionEvent.TOOL_TYPE_STYLUS;
    MotionEvent.PointerCoords coordinates = new MotionEvent.PointerCoords();
    coordinates.x = 10;
    coordinates.y = 20;
    MotionEvent event = MotionEvent.obtain(0, 1, MotionEvent.ACTION_HOVER_MOVE, 1,
        new MotionEvent.PointerProperties[] {properties},
        new MotionEvent.PointerCoords[] {coordinates}, 0, 0, 1, 1, 0, 0,
        InputDevice.SOURCE_STYLUS, 0);

    mDispatcher.onGenericMotionEvent(event, root);

    ArgumentCaptor<JavaOnlyMap> params = ArgumentCaptor.forClass(JavaOnlyMap.class);
    verify(eventEmitter).sendBubbleEvent(eq("pointermove"), eq(17), params.capture());
    assertEquals(0, params.getValue().getInt("buttons"));
    event.recycle();
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
  public void testPlatformGestureBeginDoesNotProtectNativeParent() {
    try {
      UIBody.UIBodyView rootView = mOwner.getRootUI().getView();
      final boolean[] disallowIntercept = {false};
      AndroidView parentView = new AndroidView(mContext) {
        @Override
        public void requestDisallowInterceptTouchEvent(boolean disallowInterceptValue) {
          disallowIntercept[0] = disallowInterceptValue;
          super.requestDisallowInterceptTouchEvent(disallowInterceptValue);
        }
      };
      parentView.addView(rootView);

      mDispatcher.setEnablePlatformGesture(true);
      assertFalse(disallowIntercept[0]);
      mDispatcher.onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_BEGIN);
      assertFalse(disallowIntercept[0]);
    } catch (Throwable e) {
      e.printStackTrace();
      fail();
    }
  }

  @Test
  public void testPlatformGestureActiveProtectsNativeParent() {
    try {
      UIBody.UIBodyView rootView = mOwner.getRootUI().getView();
      final boolean[] disallowIntercept = {false};
      AndroidView parentView = new AndroidView(mContext) {
        @Override
        public void requestDisallowInterceptTouchEvent(boolean disallowInterceptValue) {
          disallowIntercept[0] = disallowInterceptValue;
          super.requestDisallowInterceptTouchEvent(disallowInterceptValue);
        }
      };
      parentView.addView(rootView);

      mDispatcher.setEnablePlatformGesture(true);
      assertFalse(disallowIntercept[0]);
      mDispatcher.onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_ACTIVE);
      assertTrue(disallowIntercept[0]);

      mDispatcher.resetPlatformGestureProtection();
      assertFalse(disallowIntercept[0]);
    } catch (Throwable e) {
      e.printStackTrace();
      fail();
    }
  }

  @Test
  public void testPlatformGestureFailReleasesNativeParentWhenArenaHasNoActiveGesture() {
    try {
      UIBody.UIBodyView rootView = mOwner.getRootUI().getView();
      final boolean[] disallowIntercept = {false};
      AndroidView parentView = new AndroidView(mContext) {
        @Override
        public void requestDisallowInterceptTouchEvent(boolean disallowInterceptValue) {
          disallowIntercept[0] = disallowInterceptValue;
          super.requestDisallowInterceptTouchEvent(disallowInterceptValue);
        }
      };
      parentView.addView(rootView);

      MockGestureArenaManager manager = new MockGestureArenaManager();
      manager.setHasActivePlatformGesture(false);
      mDispatcher.setGestureArenaManager(manager);
      mDispatcher.setEnablePlatformGesture(true);

      mDispatcher.onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_ACTIVE);
      assertTrue(disallowIntercept[0]);

      mDispatcher.onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_CANCELLED);
      assertFalse(disallowIntercept[0]);
    } catch (Throwable e) {
      e.printStackTrace();
      fail();
    }
  }

  @Test
  public void testPlatformGestureFailDoesNotReleaseNativeParentWhenArenaStillActive() {
    try {
      UIBody.UIBodyView rootView = mOwner.getRootUI().getView();
      final boolean[] disallowIntercept = {false};
      AndroidView parentView = new AndroidView(mContext) {
        @Override
        public void requestDisallowInterceptTouchEvent(boolean disallowInterceptValue) {
          disallowIntercept[0] = disallowInterceptValue;
          super.requestDisallowInterceptTouchEvent(disallowInterceptValue);
        }
      };
      parentView.addView(rootView);

      MockGestureArenaManager manager = new MockGestureArenaManager();
      manager.setHasActivePlatformGesture(true);
      mDispatcher.setGestureArenaManager(manager);
      mDispatcher.setEnablePlatformGesture(true);

      mDispatcher.onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_ACTIVE);
      assertTrue(disallowIntercept[0]);

      mDispatcher.onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_FAIL);
      assertTrue(disallowIntercept[0]);
      MotionEvent ev = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
          MotionEvent.ACTION_MOVE, 100, 100, 0);
      assertTrue(mDispatcher.onInterceptTouchEvent(ev));
      ev.recycle();
    } catch (Throwable e) {
      e.printStackTrace();
      fail();
    }
  }

  @Test
  public void testPlatformGestureReleaseDoesNotClearConsumeSlideEventProtection() {
    try {
      LynxBaseUI rootUI = mOwner.getRootUI();
      UIBody.UIBodyView rootView = mOwner.getRootUI().getView();
      ArrayList<Boolean> disallowInterceptRequests = new ArrayList<>();
      AndroidView parentView = new AndroidView(mContext) {
        @Override
        public void requestDisallowInterceptTouchEvent(boolean disallowInterceptValue) {
          disallowInterceptRequests.add(disallowInterceptValue);
          super.requestDisallowInterceptTouchEvent(disallowInterceptValue);
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

      field = mDispatcher.getClass().getDeclaredField("mDownPoint");
      field.setAccessible(true);
      field.set(mDispatcher, new PointF(100, 100));

      MotionEvent down = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
          MotionEvent.ACTION_DOWN, 100, 100, 0);
      MotionEvent move = MotionEvent.obtain(SystemClock.uptimeMillis(), SystemClock.uptimeMillis(),
          MotionEvent.ACTION_MOVE, 200, 100, 0);
      assertFalse(mDispatcher.consumeSlideEvent(down));
      assertTrue(mDispatcher.consumeSlideEvent(move));
      assertTrue(disallowInterceptRequests.get(disallowInterceptRequests.size() - 1));

      MockGestureArenaManager manager = new MockGestureArenaManager();
      manager.setHasActivePlatformGesture(false);
      mDispatcher.setGestureArenaManager(manager);
      mDispatcher.setEnablePlatformGesture(true);
      mDispatcher.onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_ACTIVE);
      int requestCountBeforeRelease = disallowInterceptRequests.size();

      mDispatcher.onPlatformGestureStatusChanged(GestureConstants.LYNX_STATE_CANCELLED);
      assertEquals(requestCountBeforeRelease, disallowInterceptRequests.size());
      assertTrue(disallowInterceptRequests.get(disallowInterceptRequests.size() - 1));
      down.recycle();
      move.recycle();
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
    assertFalse(childUI.eventThrough(0, 0));
    JavaOnlyArray array = new JavaOnlyArray();
    array.pushBoolean(true);
    DynamicFromArray param = new DynamicFromArray(array, 0);
    parentUI.setEventThrough(param);
    assertTrue(childUI.eventThrough(0, 0));
  }

  @Test
  public void testEventThroughActiveRegions() {
    float density = mContext.getResources().getDisplayMetrics().density;
    mRootUI.getView().setLeft(0);
    mRootUI.getView().setRight(300);
    mRootUI.getView().setTop(0);
    mRootUI.getView().setBottom(300);
    mRootUI.setWidth(300);
    mRootUI.setHeight(300);
    UIView parentUI = new UIView(mContext);
    parentUI.getView().setLeft(0);
    parentUI.getView().setRight(300);
    parentUI.getView().setTop(0);
    parentUI.getView().setBottom(300);
    parentUI.setWidth(300);
    parentUI.setHeight(300);
    mRootUI.insertChild(parentUI, 0);
    UIView childUI = new UIView(mContext);
    childUI.getView().setLeft(0);
    childUI.getView().setRight(300);
    childUI.getView().setTop(0);
    childUI.getView().setBottom(300);
    childUI.setWidth(300);
    childUI.setHeight(300);
    parentUI.insertChild(childUI, 0);

    assertFalse(childUI.eventThrough(0, 0));
    JavaOnlyArray region = new JavaOnlyArray();
    region.pushString("0px");
    region.pushString("0px");
    region.pushString("300px");
    region.pushString("150px");
    JavaOnlyArray regions = new JavaOnlyArray();
    regions.pushArray(region);
    JavaOnlyArray value = new JavaOnlyArray();
    value.pushArray(regions);
    DynamicFromArray regionsValue = new DynamicFromArray(value, 0);
    childUI.setEventThroughActiveRegions(regionsValue);
    assertFalse(childUI.eventThrough(150 * density, 50 * density));
    assertTrue(childUI.eventThrough(150 * density, 200 * density));

    JavaOnlyArray array = new JavaOnlyArray();
    array.pushBoolean(true);
    DynamicFromArray param = new DynamicFromArray(array, 0);
    parentUI.setEventThrough(param);
    assertTrue(childUI.eventThrough(150 * density, 50 * density));
    assertFalse(childUI.eventThrough(150 * density, 200 * density));

    JavaOnlyArray region1 = new JavaOnlyArray();
    region1.pushString("0px");
    region1.pushString("150px");
    region1.pushString("300px");
    region1.pushString("150px");
    JavaOnlyArray regions1 = new JavaOnlyArray();
    regions1.pushArray(region1);
    JavaOnlyArray value1 = new JavaOnlyArray();
    value1.pushArray(regions1);
    DynamicFromArray regionsValue1 = new DynamicFromArray(value1, 0);
    parentUI.setEventThroughActiveRegions(regionsValue1);
    assertFalse(childUI.eventThrough(150 * density, 50 * density));
    assertFalse(childUI.eventThrough(150 * density, 200 * density));
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

  @Test
  public void testPointerEvents() {
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
