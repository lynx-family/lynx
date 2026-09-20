// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

import android.graphics.Rect;
import android.os.Build;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;
import com.lynx.tasm.behavior.shadow.ShadowNodeType;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.event.EventsListener;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxUIOwnerTest {
  private LynxUI mLynxUI = null;
  private LynxContext mContext;
  private LynxView mLynxView;
  private UIBody mUIBody;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
    mUIBody = new UIBody(mContext, new UIBody.UIBodyView(mContext));
    mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);
    mLynxUI = mock(LynxUI.class);
    mLynxUI.updateLayout(
        0, 0, 100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, new Rect(0, 0, 100, 100));

    LynxUI child = mock(LynxUI.class);
    child.updateLayout(0, 0, 50, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, new Rect(0, 0, 50, 50));
    mLynxUI.insertChild(child, 0);
  }

  @After
  public void tearDown() throws Exception {
    mLynxUI = null;
  }

  @Test
  public void testCopyUI() {
    // Avoid reporting it in the lower version of Android.
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
      return;
    }
    try {
      Class lynxEnvCls = LynxEnv.inst().getClass();
      Method method = lynxEnvCls.getDeclaredMethod("getBehaviors");
      List<Behavior> behaviors = (List<Behavior>) method.invoke(LynxEnv.inst());

      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner =
          new LynxUIOwner(contextSpy, new BehaviorRegistry(behaviors), mUIBody.getBodyView());
      LynxBaseUI copiedUI = mLynxUI.clone();
      assertEquals(100, copiedUI.getWidth());
      assertEquals(100, copiedUI.getHeight());

      LynxBaseUI copiedChild = copiedUI.getChildAt(0);
      assertEquals(50, copiedChild.getWidth());
      assertEquals(50, copiedChild.getHeight());

    } catch (Throwable e) {
      e.printStackTrace();
    }
  }

  @Test
  public void testGetTagInfo() {
    try {
      List<Behavior> behaviors = new BuiltInBehavior().create();

      LynxContext contextSpy = spy(mContext);
      LynxUIOwner uiOwner =
          new LynxUIOwner(contextSpy, new BehaviorRegistry(behaviors), mUIBody.getBodyView());

      assertEquals(uiOwner.getTagInfo("list"), ShadowNodeType.COMMON);
      assertEquals(uiOwner.getTagInfo("text"), ShadowNodeType.CUSTOM);
      assertEquals(uiOwner.getTagInfo("view"), ShadowNodeType.COMMON);
      assertEquals(uiOwner.getTagInfo("raw-text"), ShadowNodeType.CUSTOM | ShadowNodeType.VIRTUAL);
      assertEquals(
          uiOwner.getTagInfo("inline-text"), ShadowNodeType.CUSTOM | ShadowNodeType.VIRTUAL);
      assertEquals(uiOwner.getTagInfo("frame"), ShadowNodeType.CUSTOM);
      assertEquals(uiOwner.getTagInfo("xxxx"), 0);
    } catch (Throwable e) {
      e.printStackTrace();
      fail();
    }
  }

  @Test
  public void testPositionChangeListenerRegistration() {
    LynxUIOwner uiOwner = new LynxUIOwner(mContext, new BehaviorRegistry(), mUIBody.getBodyView());
    LynxBaseUI observedUI = mock(LynxBaseUI.class);
    Map<String, EventsListener> events = new HashMap<>();
    events.put("positionchange",
        new EventsListener("positionchange", "bindEvent", "onPositionChange", null, null));
    when(observedUI.getSign()).thenReturn(42);
    when(observedUI.getEvents()).thenReturn(events);

    uiOwner.setNode(42, observedUI);

    assertTrue(uiOwner.hasPositionChangeListenerForTesting(42));

    LynxBaseUI replacementUI = mock(LynxBaseUI.class);
    when(replacementUI.getSign()).thenReturn(42);
    when(replacementUI.getEvents()).thenReturn(new HashMap<>());
    uiOwner.setNode(42, replacementUI);

    assertFalse(uiOwner.hasPositionChangeListenerForTesting(42));

    LynxBaseUI globalBindUI = mock(LynxBaseUI.class);
    Map<String, EventsListener> globalEvents = new HashMap<>();
    globalEvents.put("positionchange",
        new EventsListener("positionchange", "global-bindEvent", "onPositionChange", null, null));
    when(globalBindUI.getSign()).thenReturn(42);
    when(globalBindUI.getEvents()).thenReturn(globalEvents);
    uiOwner.setNode(42, globalBindUI);

    assertFalse(uiOwner.hasPositionChangeListenerForTesting(42));
  }

  @Test
  public void testHasResponseChainEvent() {
    Map<String, EventsListener> events = new HashMap<>();

    events.put("tap", new EventsListener("tap", "", "", null, null));
    assertTrue(LynxUIOwner.hasResponseChainEvent(events, "tap"));

    events.put("tap", new EventsListener("tap", null, null, "", ""));
    assertTrue(LynxUIOwner.hasResponseChainEvent(events, "tap"));

    events.put("tap", new EventsListener("tap", null, null, null, null));
    assertFalse(LynxUIOwner.hasResponseChainEvent(events, "tap"));

    events.put("tap", new EventsListener("tap", "global-bindEvent", "onTap", null, null));
    assertFalse(LynxUIOwner.hasResponseChainEvent(events, "tap"));

    assertFalse(LynxUIOwner.hasResponseChainEvent(events, "missing"));
    assertFalse(LynxUIOwner.hasResponseChainEvent(null, "tap"));
  }
}
