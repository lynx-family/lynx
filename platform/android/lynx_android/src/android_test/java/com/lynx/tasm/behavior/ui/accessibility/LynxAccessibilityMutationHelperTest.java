// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityMutationHelper.*;
import static org.mockito.Mockito.*;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.view.UIComponent;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Set;
import org.junit.Before;
import org.junit.Test;

public class LynxAccessibilityMutationHelperTest {
  private LynxContext mContext;
  private UIBody mUIBody;
  private UIComponent mUIComponent;
  private LynxAccessibilityMutationHelper mMutationHelper;
  private ArrayList<JavaOnlyMap> mMutationEventList;
  private Set<String> mMutationStyles;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
    mUIBody = TestingUtils.getUIBody(mContext);
    mMutationHelper = new LynxAccessibilityMutationHelper();
    mMutationEventList = mMutationHelper.mMutationEventList;
    mMutationStyles = mMutationHelper.mMutationStyles;
    mUIComponent = new UIComponent(mContext);
  }

  @Test
  public void registerMutationStyle() {
    // init
    LynxAccessibilityMutationHelper spyMutationHelper = spy(mMutationHelper);
    Set<String> spyMutationStyles = spy(mMutationStyles);
    registerMutationStyles(spyMutationHelper, spyMutationStyles);
    // (1) test invalid param
    spyMutationHelper.registerMutationStyle(null);
    verify(spyMutationStyles, times(0)).add(any(String.class));
    // (2) test valid param
    ArrayList<String> propsArray = new ArrayList<>();
    propsArray.add(PropsConstants.BACKGROUND_COLOR);
    propsArray.add(PropsConstants.OPACITY);
    JavaOnlyArray param = JavaOnlyArray.from(propsArray);
    spyMutationHelper.registerMutationStyle(param);
    verify(spyMutationStyles, times(1)).clear();
    verify(spyMutationStyles, times(2)).add(any(String.class));
  }

  @Test
  public void insertA11yMutationEvent() {
    // init
    LynxAccessibilityMutationHelper spyMutationHelper = spy(mMutationHelper);
    ArrayList<JavaOnlyMap> spyMutationEventList = spy(mMutationEventList);
    Set<String> spyMutationStyles = spy(mMutationStyles);
    registerMutationEventList(spyMutationHelper, spyMutationEventList);
    registerMutationStyles(spyMutationHelper, spyMutationStyles);
    UIComponent spyUIComponent = spy(mUIComponent);
    when(spyUIComponent.getSign()).thenReturn(0);
    when(spyUIComponent.getAccessibilityId()).thenReturn("a11y_0");
    // (1) Test for MUTATION_ACTION_INSERT / MUTATION_ACTION_REMOVE / MUTATION_ACTION_DETACH /
    // MUTATION_ACTION_UPDATE
    spyMutationHelper.insertA11yMutationEvent(MUTATION_ACTION_INSERT, spyUIComponent);
    spyMutationHelper.insertA11yMutationEvent(MUTATION_ACTION_REMOVE, spyUIComponent);
    spyMutationHelper.insertA11yMutationEvent(MUTATION_ACTION_DETACH, spyUIComponent);
    spyMutationHelper.insertA11yMutationEvent(MUTATION_ACTION_UPDATE, spyUIComponent);
    verify(spyMutationEventList, times(4)).add(any(JavaOnlyMap.class));
    // (2) Test for MUTATION_ACTION_STYLE_UPDATE
    // Prepare mMutationStyles with props: BACKGROUND_COLOR / OPACITY
    mMutationStyles.add(PropsConstants.BACKGROUND_COLOR);
    mMutationStyles.add(PropsConstants.OPACITY);
    // BACKGROUND_COLOR and OPACITY can be inserted
    spyMutationHelper.insertA11yMutationEvent(
        MUTATION_ACTION_STYLE_UPDATE, spyUIComponent, PropsConstants.BACKGROUND_COLOR);
    spyMutationHelper.insertA11yMutationEvent(
        MUTATION_ACTION_STYLE_UPDATE, spyUIComponent, PropsConstants.OPACITY);
    // BACKGROUND_IMAGE and OVERFLOW can not be inserted
    spyMutationHelper.insertA11yMutationEvent(
        MUTATION_ACTION_STYLE_UPDATE, spyUIComponent, PropsConstants.BACKGROUND_IMAGE);
    spyMutationHelper.insertA11yMutationEvent(
        MUTATION_ACTION_STYLE_UPDATE, spyUIComponent, PropsConstants.OVERFLOW);
    // So spyMutationEventList will invoke add() 6 times
    verify(spyMutationEventList, times(6)).add(any(JavaOnlyMap.class));
  }

  @Test
  public void flushA11yMutationEvents() {
    // init
    LynxContext spyContext = spy(mContext);
    LynxAccessibilityMutationHelper spyMutationHelper = spy(mMutationHelper);
    ArrayList<JavaOnlyMap> spyMutationEventList = spy(mMutationEventList);
    registerMutationEventList(spyMutationHelper, spyMutationEventList);
    UIComponent spyUIComponent = spy(mUIComponent);
    when(spyUIComponent.getSign()).thenReturn(0);
    when(spyUIComponent.getAccessibilityId()).thenReturn("a11y_0");
    spyMutationHelper.insertA11yMutationEvent(MUTATION_ACTION_INSERT, spyUIComponent);
    spyMutationHelper.insertA11yMutationEvent(MUTATION_ACTION_REMOVE, spyUIComponent);
    spyMutationHelper.insertA11yMutationEvent(MUTATION_ACTION_DETACH, spyUIComponent);
    spyMutationHelper.insertA11yMutationEvent(MUTATION_ACTION_UPDATE, spyUIComponent);
    spyMutationHelper.flushA11yMutationEvents(spyContext);
    // Note: here we should use eq() because when using matchers, all arguments have to be provided
    // by matchers.
    verify(spyContext).sendGlobalEvent(eq("a11y-mutations"), any(JavaOnlyArray.class));
  }

  private void registerMutationStyles(
      LynxAccessibilityMutationHelper spyMutationHelper, Set<String> spyMutationStyles) {
    try {
      Field field = LynxAccessibilityMutationHelper.class.getDeclaredField("mMutationStyles");
      if (field != null) {
        field.setAccessible(true);
        field.set(spyMutationHelper, spyMutationStyles);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }

  private void registerMutationEventList(LynxAccessibilityMutationHelper spyMutationHelper,
      ArrayList<JavaOnlyMap> spyMutationEventList) {
    try {
      Field field = LynxAccessibilityMutationHelper.class.getDeclaredField("mMutationEventList");
      if (field != null) {
        field.setAccessible(true);
        field.set(spyMutationHelper, spyMutationEventList);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }
}
