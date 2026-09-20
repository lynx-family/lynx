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
import com.lynx.tasm.behavior.ui.UIParams;
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
import org.mockito.ArgumentCaptor;
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
      assertFalse(UIThreadUtils.isOnUiThread());
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

    JavaOnlyMap original = JavaOnlyMap.shallowCopy(props);
    doAnswer(invocation -> {
      assertFalse(UIThreadUtils.isOnUiThread());
      assertTrue(ui.getGestureDetectorMap().containsKey(7));
      StylesDiffMap styles = invocation.getArgument(0);
      assertSame(props, styles.mBackingMap);
      assertEquals(original, styles.mBackingMap);
      return invocation.callRealMethod();
    })
        .when(ui)
        .updatePropertiesInterval(any());
    doAnswer(invocation -> {
      assertTrue(UIThreadUtils.isOnUiThread());
      return invocation.callRealMethod();
    })
        .when(ui)
        .afterPropsUpdated(any());
    Runnable result = owner.prepareViewForRenderer(42, "prepared-view", props, events, gestures);
    assertNotNull(result);
    assertNull(owner.getNode(42));
    assertSame(ui, mExclusiveMap.get(42).get());
    assertTrue(ui.getGestureDetectorMap().containsKey(7));
    verify(ui).setAlpha(0.5f);
    verify(ui, times(1)).updatePropertiesInterval(any());
    verify(ui).setAccessibilityExclusiveFocus(true);
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
    ArgumentCaptor<UIParams> params = ArgumentCaptor.forClass(UIParams.class);
    verify(behavior).createUIWithParams(eq(context), params.capture());
    StylesDiffMap styles = params.getValue().mInitialProps;
    verify(ui, times(1)).updatePropertiesInterval(any());
    verify(ui).updatePropertiesInterval(same(styles));
    verify(ui, times(1)).afterPropsUpdated(same(styles));
    assertSame(props, styles.mBackingMap);
    assertEquals(original, props);
    verify(ui, never()).destroy();
  }

  @Test
  public void testNullPropsMatchLegacyAsyncCreation() {
    assertPropsMatchLegacy(null);
  }

  @Test
  public void testEmptyPropsMatchLegacyAsyncCreation() {
    assertPropsMatchLegacy(new JavaOnlyMap());
  }

  private void assertPropsMatchLegacy(JavaOnlyMap props) {
    for (boolean legacy : new boolean[] {false, true}) {
      UIView ui = mock(UIView.class);
      Behavior behavior = mock(Behavior.class);
      BehaviorRegistry registry = mock(BehaviorRegistry.class);
      when(registry.get("prepared-view")).thenReturn(behavior);
      when(behavior.createUIWithParams(eq(mContext), any())).thenReturn(ui);
      LynxUIOwner owner = new LynxUIOwner(mContext, registry, mUIBody.getBodyView());
      mContext.setLynxUIOwner(owner);
      Runnable prepared = legacy
          ? owner.createViewAsyncRunnable(42, "prepared-view", props, null, null, false, 42, null)
          : owner.prepareViewForRenderer(42, "prepared-view", props, null, null);
      ArgumentCaptor<UIParams> params = ArgumentCaptor.forClass(UIParams.class);
      verify(behavior).createUIWithParams(eq(mContext), params.capture());
      StylesDiffMap styles = params.getValue().mInitialProps;
      if (props == null) {
        assertNull(styles);
        verify(ui, never()).updatePropertiesInterval(any());
      } else {
        assertSame(props, styles.mBackingMap);
        verify(ui, times(1)).updatePropertiesInterval(same(styles));
      }
      assertNull(owner.getNode(42));
      verify(ui, never()).afterPropsUpdated(any());
      InstrumentationRegistry.getInstrumentation().runOnMainSync(prepared);
      if (props == null) {
        verify(ui, never()).afterPropsUpdated(any());
      } else {
        verify(ui, times(1)).updatePropertiesInterval(same(styles));
        verify(ui, times(1)).afterPropsUpdated(same(styles));
        assertTrue(props.isEmpty());
      }
      assertSame(ui, owner.getNode(42));
    }
  }

  @Test
  public void testPreparationPropertyErrorCannotPublishAfterOwnerDestruction() {
    UIView ui = spy(new UIView(mContext));
    Behavior behavior = mock(Behavior.class);
    BehaviorRegistry registry = mock(BehaviorRegistry.class);
    when(registry.get("prepared-view")).thenReturn(behavior);
    when(behavior.createUIWithParams(eq(mContext), any())).thenReturn(ui);
    LynxUIOwner owner = new LynxUIOwner(mContext, registry, mUIBody.getBodyView());
    mContext.setLynxUIOwner(owner);
    AtomicInteger errors = new AtomicInteger();
    doAnswer(invocation -> {
      assertFalse(UIThreadUtils.isOnUiThread());
      errors.incrementAndGet();
      return null;
    })
        .when(mContext)
        .handleLynxError(any());
    JavaOnlyMap props = new JavaOnlyMap();
    props.putMap(PropsConstants.ACCESSIBILITY_EXCLUSIVE_FOCUS, new JavaOnlyMap());
    Runnable result = owner.prepareViewForRenderer(42, "prepared-view", props, null, null);
    assertEquals(1, errors.get());
    InstrumentationRegistry.getInstrumentation().runOnMainSync(() -> {
      owner.destroy();
      result.run();
    });
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
  public void testAllLegacyRegistrySettersRunOnceDuringPreparation() {
    UIView ui = spy(new UIView(mContext));
    Behavior behavior = mock(Behavior.class);
    BehaviorRegistry registry = mock(BehaviorRegistry.class);
    when(registry.get("prepared-view")).thenReturn(behavior);
    when(behavior.createUIWithParams(eq(mContext), any())).thenReturn(ui);
    LynxUIOwner owner = new LynxUIOwner(mContext, registry, mUIBody.getBodyView());
    mContext.setLynxUIOwner(owner);
    AtomicInteger calls = new AtomicInteger();
    Answer<Object> onPreparation = invocation -> {
      assertFalse(UIThreadUtils.isOnUiThread());
      calls.incrementAndGet();
      return null;
    };
    doAnswer(onPreparation).when(ui).setAccessibilityElements(any());
    doAnswer(onPreparation).when(ui).setAccessibilityElementsA11y(any());
    doAnswer(onPreparation).when(ui).setAccessibilityExclusiveFocus(anyBoolean());
    doAnswer(onPreparation).when(ui).setExposureID(any());
    doAnswer(onPreparation).when(ui).setExposureScene(any());
    doAnswer(onPreparation).when(ui).setIntersectionObservers(any());
    doAnswer(onPreparation).when(ui).setShareElement(any());
    doAnswer(onPreparation).when(ui).setEnterTransitionName(any());
    doAnswer(onPreparation).when(ui).setExitTransitionName(any());
    doAnswer(onPreparation).when(ui).setPauseTransitionName(any());
    doAnswer(onPreparation).when(ui).setResumeTransitionName(any());
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
    assertEquals(11, calls.get());
    verify(ui, times(1)).updatePropertiesInterval(any());
    InstrumentationRegistry.getInstrumentation().runOnMainSync(result);
    assertEquals(11, calls.get());
    verify(ui, times(1)).updatePropertiesInterval(any());
    verify(ui).setAccessibilityElements(any());
    verify(ui).setAccessibilityElementsA11y(any());
    verify(ui).setAccessibilityExclusiveFocus(anyBoolean());
    verify(ui).setExposureID(any());
    verify(ui).setExposureScene(any());
    verify(ui).setIntersectionObservers(any());
    verify(ui).setShareElement(any());
    verify(ui).setEnterTransitionName(any());
    verify(ui).setExitTransitionName(any());
    verify(ui).setPauseTransitionName(any());
    verify(ui).setResumeTransitionName(any());
    assertSame(ui, owner.getNode(42));
  }

  @Test
  public void testUnpublishedConstructionDoesNotReplaceUIHolderEntry() {
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
    // Like legacy async creation, setters may register during preparation. Their shared
    // registry side effects remain the legacy caller/component responsibility, not publication.
    assertSame(ui, mExclusiveMap.get(42).get());
    verify(ui).setAccessibilityExclusiveFocus(true);
    assertSame(replacement, owner.getNode(42));
    assertEquals(42, ui.getSign());
    verify(ui, never()).destroy();
    verify(ui, never()).afterPropsUpdated(any());
  }
}
