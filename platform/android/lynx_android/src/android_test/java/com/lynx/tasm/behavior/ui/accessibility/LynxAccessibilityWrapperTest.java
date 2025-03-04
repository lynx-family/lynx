// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import static com.lynx.jsbridge.LynxAccessibilityModule.MSG;
import static com.lynx.jsbridge.LynxAccessibilityModule.MSG_MUTATION_STYLES;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.*;

import android.app.Service;
import android.view.MotionEvent;
import android.view.accessibility.AccessibilityManager;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.PageConfig;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StylesDiffMap;
import com.lynx.tasm.behavior.ui.LynxAccessibilityNodeProvider;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;

public class LynxAccessibilityWrapperTest {
  private static final String KEY_ENABLE_ACCESSIBILITY_ELEMENT = "enableAccessibilityElement";
  private static final String KEY_ENABLE_OVERLAP_ACCESSIBILITY_ELEMENT =
      "enableOverlapForAccessibilityElement";
  private static final String KEY_ENABLE_NEW_ACCESSIBILITY = "enableNewAccessibility";
  private static final String KEY_ENABLE_A11Y_ID_MUTATION_OBSERVER = "enableA11yIDMutationObserver";
  private static final String KEY_ENABLE_A11Y = "enableA11y";
  private LynxContext mContext;
  private UIBody mUIBody;
  private LynxAccessibilityWrapper mWrapper;
  private LynxAccessibilityNodeProvider mNodeProvider;
  private LynxAccessibilityDelegate mDelegate;
  private LynxAccessibilityHelper mHelper;
  private LynxAccessibilityMutationHelper mMutationHelper;
  private LynxAccessibilityStateHelper mStatusHelper;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
    mUIBody = new UIBody(mContext, new UIBody.UIBodyView(mContext));
    mWrapper = new LynxAccessibilityWrapper(mUIBody);
    mNodeProvider = new LynxAccessibilityNodeProvider(mUIBody);
    mDelegate = new LynxAccessibilityDelegate(mUIBody);
    mHelper = new LynxAccessibilityHelper(mUIBody);
    AccessibilityManager manager =
        (AccessibilityManager) mContext.getSystemService(Service.ACCESSIBILITY_SERVICE);
    mStatusHelper = new LynxAccessibilityStateHelper(
        manager, new LynxAccessibilityStateHelper.OnStateListener() {
          @Override
          public void onAccessibilityEnable(boolean enable) {}

          @Override
          public void onTouchExplorationEnable(boolean enable) {}
        });
    mMutationHelper = new LynxAccessibilityMutationHelper();
  }

  @Test
  public void onPageConfigDecoded() {
    // init
    LynxAccessibilityNodeProvider spyNodeProvider = spy(mNodeProvider);
    LynxAccessibilityDelegate spyDelegate = spy(mDelegate);
    LynxAccessibilityHelper spyHelper = spy(mHelper);
    // Note: In enableNodeProvider() / enableDelegate() / enableHelper(), will invoke spyWrapper
    // onPageConfigDecoded() (1)
    LynxAccessibilityWrapper spyWrapper1 = spy(mWrapper);
    registerNodeProvider(spyWrapper1, spyNodeProvider);
    enableNodeProvider(spyWrapper1);
    verify(spyNodeProvider).setConfigEnableAccessibilityElement(true);
    verify(spyNodeProvider).setConfigEnableOverlapForAccessibilityElement(true);
    // (2)
    LynxAccessibilityWrapper spyWrapper2 = spy(mWrapper);
    registerDelegate(spyWrapper2, spyDelegate);
    enableDelegate(spyWrapper2);
    assertNotNull(getField(spyWrapper2, "mStateHelper"));
    assertNotNull(getField(spyWrapper2, "mMutationHelper"));
    assertNotNull(getField(spyWrapper2, "mDelegate"));
    verify(spyDelegate).setConfigEnableAccessibilityElement(true);
    // (3)
    LynxAccessibilityWrapper spyWrapper3 = spy(mWrapper);
    registerHelper(spyWrapper3, spyHelper);
    enableHelper(spyWrapper3);
    assertNotNull(getField(spyWrapper3, "mStateHelper"));
    assertNotNull(getField(spyWrapper3, "mMutationHelper"));
    assertNotNull(getField(spyWrapper3, "mHelper"));
    verify(spyHelper).setConfigEnableAccessibilityElement(true);
  }

  @Test
  public void onHoverEvent() {
    // init
    LynxAccessibilityWrapper spyWrapper = spy(mWrapper);
    LynxAccessibilityNodeProvider spyNodeProvider = spy(mNodeProvider);
    registerNodeProvider(spyWrapper, spyNodeProvider);
    LynxAccessibilityDelegate spyDelegate = spy(mDelegate);
    registerDelegate(spyWrapper, spyDelegate);
    // (1)
    enableNodeProvider(spyWrapper);
    MotionEvent motionEvent = MotionEvent.obtain(0, 0, MotionEvent.ACTION_DOWN, 0.f, 0.f, 0);
    spyWrapper.onHoverEvent(mUIBody.getBodyView(), motionEvent);
    verify(spyNodeProvider).onHover(mUIBody.getBodyView(), motionEvent);
    // (2)
    enableDelegate(spyWrapper);
    spyWrapper.onHoverEvent(mUIBody.getBodyView(), motionEvent);
    verify(spyDelegate).dispatchHoverEvent(motionEvent);
    // (3)
    enableHelper(spyWrapper);
    assertFalse(spyWrapper.onHoverEvent(mUIBody.getBodyView(), motionEvent));
  }

  @Test
  public void onDestroy() {
    LynxAccessibilityStateHelper spyStateHelper = spy(mStatusHelper);
    // (1)
    LynxAccessibilityWrapper spyWrapper1 = spy(mWrapper);
    registerStateHelper(spyWrapper1, spyStateHelper);
    enableDelegate(spyWrapper1);
    spyWrapper1.onDestroy();
    verify(spyStateHelper, times(1)).removeAllListeners();
    // (2)
    LynxAccessibilityWrapper spyWrapper2 = spy(mWrapper);
    registerStateHelper(spyWrapper2, spyStateHelper);
    enableHelper(spyWrapper2);
    spyWrapper2.onDestroy();
    verify(spyStateHelper, times(2)).removeAllListeners();
  }

  @Test
  public void shouldHandleA11yMutation() {
    // (1)
    LynxAccessibilityWrapper spyWrapper1 = spy(mWrapper);
    enableNodeProvider(spyWrapper1);
    assertFalse(spyWrapper1.shouldHandleA11yMutation());
    // (2)
    LynxAccessibilityWrapper spyWrapper2 = spy(mWrapper);
    when(spyWrapper2.shouldHandleA11yMutation()).thenReturn(true);
    enableDelegate(spyWrapper2);
    assertTrue(spyWrapper2.shouldHandleA11yMutation());
  }

  @Test
  public void handleMutationStyleUpdate() {
    // init
    LynxAccessibilityWrapper spyWrapper = spy(mWrapper);
    LynxAccessibilityMutationHelper spyMutationHelper = spy(mMutationHelper);
    registerMutationHelper(spyWrapper, spyMutationHelper);
    UIComponent component = new UIComponent(mContext);
    JavaOnlyMap javaOnlyMap = new JavaOnlyMap();
    javaOnlyMap.put(PropsConstants.OPACITY, 0.1);
    javaOnlyMap.put(PropsConstants.OVERFLOW, false);
    StylesDiffMap props = new StylesDiffMap(javaOnlyMap);
    when(spyWrapper.shouldHandleA11yMutation()).thenReturn(true);
    // (1)
    spyWrapper.handleMutationStyleUpdate(component, props);
    verify(spyMutationHelper, times(1))
        .insertA11yMutationEvent(LynxAccessibilityMutationHelper.MUTATION_ACTION_STYLE_UPDATE,
            component, PropsConstants.OPACITY);
    verify(spyMutationHelper, times(1))
        .insertA11yMutationEvent(LynxAccessibilityMutationHelper.MUTATION_ACTION_STYLE_UPDATE,
            component, PropsConstants.OVERFLOW);
  }

  @Test
  public void registerMutationStyleInner() {
    // init
    LynxAccessibilityWrapper spyWrapper = spy(mWrapper);
    LynxAccessibilityMutationHelper spyMutationHelper = spy(mMutationHelper);
    registerMutationHelper(spyWrapper, spyMutationHelper);
    JavaOnlyArray paramsArray = new JavaOnlyArray();
    paramsArray.add(0, PropsConstants.OPACITY);
    paramsArray.add(1, PropsConstants.OVERFLOW);
    JavaOnlyMap params = new JavaOnlyMap();
    params.put(MSG_MUTATION_STYLES, paramsArray);
    JavaOnlyMap res = new JavaOnlyMap();
    // (1)
    when(spyWrapper.enableDelegate()).thenReturn(false);
    when(spyWrapper.enableHelper()).thenReturn(false);
    spyWrapper.registerMutationStyleInner(params, res);
    String msg1 = res.getString(MSG);
    assertTrue(msg1.startsWith("Fail"));
    // (2)
    when(spyWrapper.enableDelegate()).thenReturn(true);
    when(spyWrapper.enableHelper()).thenReturn(true);
    spyWrapper.registerMutationStyleInner(params, res);
    String msg2 = res.getString(MSG);
    verify(spyMutationHelper).registerMutationStyle(paramsArray);
    assertTrue(msg2.startsWith("Success"));
  }

  @Test
  public void onLayoutFinish() {
    // init
    LynxAccessibilityMutationHelper spyMutationHelper = spy(mMutationHelper);
    LynxAccessibilityHelper spyHelper = spy(mHelper);
    // (1)
    LynxAccessibilityWrapper spyWrapper1 = spy(mWrapper);
    registerMutationHelper(spyWrapper1, spyMutationHelper);
    enableNodeProvider(spyWrapper1);
    spyWrapper1.onLayoutFinish();
    verify(spyMutationHelper, times(0)).flushA11yMutationEvents(any(LynxContext.class));
    // (2)
    LynxAccessibilityWrapper spyWrapper2 = spy(mWrapper);
    registerMutationHelper(spyWrapper2, spyMutationHelper);
    enableDelegate(spyWrapper2);
    when(spyWrapper2.shouldHandleA11yMutation()).thenReturn(false);
    spyWrapper2.onLayoutFinish();
    verify(spyMutationHelper, times(0)).flushA11yMutationEvents(any(LynxContext.class));
    when(spyWrapper2.shouldHandleA11yMutation()).thenReturn(true);
    spyWrapper2.onLayoutFinish();
    verify(spyMutationHelper, times(1)).flushA11yMutationEvents(any(LynxContext.class));
    // (3)
    LynxAccessibilityWrapper spyWrapper3 = spy(mWrapper);
    registerMutationHelper(spyWrapper3, spyMutationHelper);
    registerHelper(spyWrapper3, spyHelper);
    enableHelper(spyWrapper3);
    when(spyWrapper3.shouldHandleA11yMutation()).thenReturn(false);
    spyWrapper3.onLayoutFinish();
    verify(spyMutationHelper, times(1)).flushA11yMutationEvents(any(LynxContext.class));
    verify(spyHelper, times(1)).applyAccessibilityElements();
    when(spyWrapper3.shouldHandleA11yMutation()).thenReturn(true);
    spyWrapper3.onLayoutFinish();
    verify(spyMutationHelper, times(2)).flushA11yMutationEvents(any(LynxContext.class));
    verify(spyHelper, times(2)).applyAccessibilityElements();
  }

  private void enableNodeProvider(LynxAccessibilityWrapper wrapper) {
    PageConfig config = generatePageConfig(true, true, false, false, false);
    wrapper.onPageConfigDecoded(config);
  }

  private void enableDelegate(LynxAccessibilityWrapper wrapper) {
    PageConfig config = generatePageConfig(true, true, true, true, false);
    wrapper.onPageConfigDecoded(config);
  }

  private void enableHelper(LynxAccessibilityWrapper wrapper) {
    PageConfig config = generatePageConfig(true, true, false, true, true);
    wrapper.onPageConfigDecoded(config);
  }

  private PageConfig generatePageConfig(final boolean enableAccessibilityElement,
      final boolean enableOverlapForAccessibilityElement, final boolean enableNewAccessibility,
      final boolean enableA11yIDMutationObserver, final boolean enableA11y) {
    JavaOnlyMap map = new JavaOnlyMap();
    map.put(KEY_ENABLE_ACCESSIBILITY_ELEMENT, enableAccessibilityElement);
    map.put(KEY_ENABLE_OVERLAP_ACCESSIBILITY_ELEMENT, enableOverlapForAccessibilityElement);
    map.put(KEY_ENABLE_NEW_ACCESSIBILITY, enableNewAccessibility);
    map.put(KEY_ENABLE_A11Y_ID_MUTATION_OBSERVER, enableA11yIDMutationObserver);
    map.put(KEY_ENABLE_A11Y, enableA11y);
    PageConfig pageConfig = new PageConfig(map);
    return pageConfig;
  }

  private void registerNodeProvider(
      LynxAccessibilityWrapper wrapper, LynxAccessibilityNodeProvider nodeProvider) {
    try {
      Field field = LynxAccessibilityWrapper.class.getDeclaredField("mNodeProvider");
      if (field != null && wrapper != null) {
        field.setAccessible(true);
        field.set(wrapper, nodeProvider);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }

  private void registerDelegate(
      LynxAccessibilityWrapper wrapper, LynxAccessibilityDelegate delegate) {
    try {
      Field field = LynxAccessibilityWrapper.class.getDeclaredField("mDelegate");
      if (field != null && wrapper != null) {
        field.setAccessible(true);
        field.set(wrapper, delegate);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }

  private void registerHelper(LynxAccessibilityWrapper wrapper, LynxAccessibilityHelper helper) {
    try {
      Field field = LynxAccessibilityWrapper.class.getDeclaredField("mHelper");
      if (field != null && wrapper != null) {
        field.setAccessible(true);
        field.set(wrapper, helper);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }

  private void registerStateHelper(
      LynxAccessibilityWrapper wrapper, LynxAccessibilityStateHelper stateHelper) {
    try {
      Field field = LynxAccessibilityWrapper.class.getDeclaredField("mStateHelper");
      if (field != null && wrapper != null) {
        field.setAccessible(true);
        field.set(wrapper, stateHelper);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }

  private void registerMutationHelper(
      LynxAccessibilityWrapper wrapper, LynxAccessibilityMutationHelper mutationHelper) {
    try {
      Field field = LynxAccessibilityWrapper.class.getDeclaredField("mMutationHelper");
      if (field != null && wrapper != null) {
        field.setAccessible(true);
        field.set(wrapper, mutationHelper);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }

  Object getField(LynxAccessibilityWrapper wrapper, String fieldName) {
    Object fieldObject = null;
    try {
      Field field = LynxAccessibilityWrapper.class.getDeclaredField(fieldName);
      if (wrapper != null && field != null) {
        field.setAccessible(true);
        fieldObject = field.get(wrapper);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
    return fieldObject;
  }
}
