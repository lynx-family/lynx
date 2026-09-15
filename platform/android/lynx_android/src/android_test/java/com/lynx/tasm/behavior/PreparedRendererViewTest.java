// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIShadowProxy;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityHelper;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.utils.UIThreadUtils;
import com.lynx.testing.base.TestingUtils;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import org.junit.Before;
import org.junit.Test;
import org.mockito.stubbing.Answer;

public class PreparedRendererViewTest {
  private LynxContext mContext;
  private UIBody mUIBody;
  private LynxAccessibilityHelper mHelper;
  private Map<Integer, WeakReference<LynxBaseUI>> mExclusiveMap;

  @Before
  public void setUp() throws Exception {
    LynxEnv.inst().initNativeLibraries(new INativeLibraryLoader() {
      @Override
      public void loadLibrary(String name) {
        System.loadLibrary(name);
      }
    });
    mContext = spy(TestingUtils.getLynxContext());
    mUIBody = TestingUtils.getUIBody(mContext);
    mHelper = new LynxAccessibilityHelper(mUIBody);
    Field field = LynxAccessibilityHelper.class.getDeclaredField("mExclusiveUIMap");
    field.setAccessible(true);
    mExclusiveMap = (Map<Integer, WeakReference<LynxBaseUI>>) field.get(mHelper);
    LynxAccessibilityWrapper wrapper = mock(LynxAccessibilityWrapper.class);
    when(mContext.getLynxAccessibilityWrapper()).thenReturn(wrapper);
    doAnswer(invocation -> {
      assertTrue(UIThreadUtils.isOnUiThread());
      if ((Boolean) invocation.getArgument(1)) {
        mHelper.addUIToExclusiveMap(invocation.getArgument(0));
      } else {
        mHelper.removeUIFromExclusiveMap(invocation.getArgument(0));
      }
      return null;
    })
        .when(wrapper)
        .addOrRemoveUIFromExclusiveMap(any(), anyBoolean());
  }

  @Test
  public void testPreparedRendererPreservesPayloadAndPublishesOnlyOnMain() {
    LynxContext context = mContext;
    UIView ui = spy(new UIView(context));
    Behavior behavior = mock(Behavior.class);
    BehaviorRegistry registry = mock(BehaviorRegistry.class);
    when(registry.get("prepared-view")).thenReturn(behavior);
    when(behavior.createUIWithParams(eq(context), any())).thenReturn(ui);
    LynxUIOwner owner = new LynxUIOwner(context, registry, mUIBody.getBodyView());
    context.setLynxUIOwner(owner);
    JavaOnlyMap props = new JavaOnlyMap();
    props.putDouble("opacity", 0.5);
    doAnswer(invocation -> {
      assertFalse(UIThreadUtils.isOnUiThread());
      return invocation.callRealMethod();
    })
        .when(ui)
        .setAlpha(0.5f);
    props.putBoolean("accessibility-exclusive-focus", true);
    props.putInt("outline-width", 1);
    JavaOnlyMap event = new JavaOnlyMap();
    event.putString("name", "tap");
    event.putString("type", "bind");
    event.putString("function", "onTap");
    JavaOnlyArray events = JavaOnlyArray.of(event);
    JavaOnlyMap gesture = new JavaOnlyMap();
    gesture.putInt("id", 7);
    gesture.putInt("type", 0);
    gesture.putArray("callbackNames", new JavaOnlyArray());
    gesture.putMap("relationMap", new JavaOnlyMap());
    JavaOnlyArray gestures = JavaOnlyArray.of(gesture);

    Runnable result = owner.prepareViewForRenderer(42, "prepared-view", props, events, gestures);
    assertNotNull(result);
    assertNull(owner.getNode(42));
    assertTrue(mExclusiveMap.isEmpty());
    assertNull(ui.getGestureDetectorMap());
    verify(ui).setAlpha(0.5f);
    verify(ui, times(1)).updatePropertiesInterval(any());
    verify(ui, never()).setAccessibilityExclusiveFocus(anyBoolean());
    verify(ui, never()).afterPropsUpdated(any());

    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> { result.run(); });
    assertTrue(owner.getNode(42) instanceof UIShadowProxy);
    assertSame(owner.getNode(42), ui.getParent());
    assertTrue(ui.getGestureDetectorMap().containsKey(7));
    assertEquals("onTap", ui.getEvents().get("tap").functionName);
    verify(ui).setAccessibilityExclusiveFocus(true);
    assertSame(ui, mExclusiveMap.get(42).get());
    assertEquals(42, ui.getSign());
    assertEquals(42, ui.getNodeIndex());
    verify(ui, times(1)).afterPropsUpdated(any());
    verify(ui, never()).destroy();
  }

  @Test
  public void testDeferredPropertyErrorCannotPublishAfterOwnerDestruction() {
    UIView ui = spy(new UIView(mContext));
    Behavior behavior = mock(Behavior.class);
    BehaviorRegistry registry = mock(BehaviorRegistry.class);
    when(registry.get("prepared-view")).thenReturn(behavior);
    when(behavior.createUIWithParams(eq(mContext), any())).thenReturn(ui);
    LynxUIOwner owner = new LynxUIOwner(mContext, registry, mUIBody.getBodyView());
    mContext.setLynxUIOwner(owner);
    AtomicInteger errors = new AtomicInteger();
    doAnswer(invocation -> {
      assertTrue(UIThreadUtils.isOnUiThread());
      errors.incrementAndGet();
      owner.destroy();
      return null;
    })
        .when(mContext)
        .handleLynxError(any());
    JavaOnlyMap props = new JavaOnlyMap();
    props.putMap(PropsConstants.ACCESSIBILITY_EXCLUSIVE_FOCUS, new JavaOnlyMap());
    Runnable result = owner.prepareViewForRenderer(42, "prepared-view", props, null, null);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(result);
    assertEquals(1, errors.get());
    assertNull(owner.getNode(42));
    verify(ui, never()).afterPropsUpdated(any());
  }

  @Test
  public void testOwnerDestructionDoesNotDestroyUnpublishedRenderer() {
    assertUnpublishedRendererLifecycle(false);
  }

  @Test
  public void testOwnerReplacementDoesNotDestroyUnpublishedRenderer() {
    assertUnpublishedRendererLifecycle(true);
  }

  private void assertUnpublishedRendererLifecycle(boolean replaceOwner) {
    UIView ui = mock(UIView.class);
    Behavior behavior = mock(Behavior.class);
    BehaviorRegistry registry = mock(BehaviorRegistry.class);
    when(registry.get("prepared-view")).thenReturn(behavior);
    when(behavior.createUIWithParams(eq(mContext), any())).thenReturn(ui);
    LynxUIOwner owner = new LynxUIOwner(mContext, registry, mUIBody.getBodyView());
    mContext.setLynxUIOwner(owner);
    JavaOnlyMap props = new JavaOnlyMap();
    props.putDouble("opacity", 0.5);
    Runnable prepared = owner.prepareViewForRenderer(42, "prepared-view", props, null, null);
    assertNotNull(prepared);
    assertNull(owner.getNode(42));

    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      if (replaceOwner) {
        LynxUIOwner replacement = new LynxUIOwner(mContext, registry, mUIBody.getBodyView());
        mContext.setLynxUIOwner(replacement);
      } else {
        owner.destroy();
      }
      prepared.run();
      assertNull(owner.getNode(42));
      verify(ui, never()).afterPropsUpdated(any());
      verify(ui, never()).destroy();
    });
  }

  @Test
  public void testPublishedRendererRetainsNormalOwnerDestruction() {
    UIView ui = mock(UIView.class);
    Behavior behavior = mock(Behavior.class);
    BehaviorRegistry registry = mock(BehaviorRegistry.class);
    when(registry.get("prepared-view")).thenReturn(behavior);
    when(behavior.createUIWithParams(eq(mContext), any())).thenReturn(ui);
    LynxUIOwner owner = new LynxUIOwner(mContext, registry, mUIBody.getBodyView());
    mContext.setLynxUIOwner(owner);
    Runnable prepared = owner.prepareViewForRenderer(42, "prepared-view", null, null, null);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      prepared.run();
      assertSame(ui, owner.getNode(42));
      verify(ui, never()).destroy();
      owner.destroy();
      verify(ui, times(1)).destroy();
    });
  }

  @Test
  public void testSharedRegistrySettersWaitForMainThreadFinalization() {
    UIView ui = spy(new UIView(mContext));
    Behavior behavior = mock(Behavior.class);
    BehaviorRegistry registry = mock(BehaviorRegistry.class);
    when(registry.get("prepared-view")).thenReturn(behavior);
    when(behavior.createUIWithParams(eq(mContext), any())).thenReturn(ui);
    LynxUIOwner owner = new LynxUIOwner(mContext, registry, mUIBody.getBodyView());
    mContext.setLynxUIOwner(owner);
    AtomicInteger calls = new AtomicInteger();
    Answer<Object> onMain = invocation -> {
      assertTrue(UIThreadUtils.isOnUiThread());
      calls.incrementAndGet();
      return null;
    };
    doAnswer(onMain).when(ui).setAccessibilityElements(any());
    doAnswer(onMain).when(ui).setAccessibilityElementsA11y(any());
    doAnswer(onMain).when(ui).setAccessibilityExclusiveFocus(anyBoolean());
    doAnswer(onMain).when(ui).setExposureID(any());
    doAnswer(onMain).when(ui).setExposureScene(any());
    doAnswer(onMain).when(ui).setIntersectionObservers(any());
    doAnswer(onMain).when(ui).setShareElement(any());
    doAnswer(onMain).when(ui).setEnterTransitionName(any());
    doAnswer(onMain).when(ui).setExitTransitionName(any());
    doAnswer(onMain).when(ui).setPauseTransitionName(any());
    doAnswer(onMain).when(ui).setResumeTransitionName(any());
    JavaOnlyMap props = new JavaOnlyMap();
    for (String key : new String[] {PropsConstants.ACCESSIBILITY_ELEMENTS,
             PropsConstants.ACCESSIBILITY_ELEMENTS_A11Y,
             PropsConstants.ACCESSIBILITY_EXCLUSIVE_FOCUS, PropsConstants.EXPOSURE_ID,
             PropsConstants.EXPOSURE_SCENE, PropsConstants.INTERSECTION_OBSERVERS,
             PropsConstants.SHARED_ELEMENT, PropsConstants.ENTER_TRANSITION_NAME,
             PropsConstants.EXIT_TRANSITION_NAME, PropsConstants.PAUSE_TRANSITION_NAME,
             PropsConstants.RESUME_TRANSITION_NAME}) {
      props.putNull(key);
    }
    Runnable result = owner.prepareViewForRenderer(42, "prepared-view", props, null, null);
    assertEquals(0, calls.get());
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> assertEquals(0, calls.get()));
    InstrumentationRegistry.getInstrumentation().runOnMainSync(result);
    assertEquals(11, calls.get());
    assertSame(ui, owner.getNode(42));
  }

  @Test
  public void testUnpublishedConstructionCannotTouchPublishedReplacement() {
    LynxContext context = mContext;
    UIView ui = spy(new UIView(context));
    Behavior behavior = mock(Behavior.class);
    BehaviorRegistry registry = mock(BehaviorRegistry.class);
    when(registry.get("prepared-view")).thenReturn(behavior);
    when(behavior.createUIWithParams(eq(context), any())).thenReturn(ui);
    LynxUIOwner owner = new LynxUIOwner(context, registry, mUIBody.getBodyView());
    context.setLynxUIOwner(owner);
    JavaOnlyMap props = new JavaOnlyMap();
    props.putBoolean("accessibility-exclusive-focus", true);
    UIView replacement = new UIView(context);
    replacement.setSign(42, "prepared-view");
    owner.setNode(42, replacement);
    InstrumentationRegistry.getInstrumentation().runOnMainSync(
        () -> mHelper.addUIToExclusiveMap(replacement));
    Runnable result = owner.prepareViewForRenderer(42, "prepared-view", props, null, null);
    assertNotNull(result);
    assertSame(replacement, mExclusiveMap.get(42).get());
    // Abandoned preparation must not alter the existing registration.
    verify(ui, never()).setAccessibilityExclusiveFocus(anyBoolean());
    assertSame(replacement, owner.getNode(42));
    assertEquals(42, ui.getSign());
    verify(ui, never()).destroy();
    verify(ui, never()).afterPropsUpdated(any());
  }
}
