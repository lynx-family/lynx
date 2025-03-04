// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.jsbridge;

import static com.lynx.jsbridge.LynxAccessibilityModule.MSG_MUTATION_STYLES;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.spy;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityDelegate;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityMutationHelper;
import com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper;
import com.lynx.testing.base.TestingUtils;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;

@RunWith(AndroidJUnit4.class)
public class LynxAccessibilityModuleTest {
  public static final String MSG_SUCCESS = "Success";
  public static final String MSG_FAIL = "Fail";
  private LynxContext mLynxContext;
  private LynxAccessibilityModule mLynxAccessibilityModule;
  private UIBody mUIBody;
  private LynxAccessibilityWrapper mA11yWrapper;
  private LynxAccessibilityDelegate mDelegate;
  private LynxAccessibilityMutationHelper mMutationHelper;

  @Before
  public void setUp() throws Exception {
    mLynxContext = TestingUtils.getLynxContext();
    mUIBody = new UIBody(mLynxContext, new UIBody.UIBodyView(mLynxContext));
    mLynxContext.setUIBody(mUIBody);
    mA11yWrapper = new LynxAccessibilityWrapper(mUIBody);
    mDelegate = new LynxAccessibilityDelegate(mUIBody);
    registerDelegate(mDelegate);
    mLynxAccessibilityModule = new LynxAccessibilityModule(mLynxContext);
    mMutationHelper = new LynxAccessibilityMutationHelper();
  }

  @Test
  public void testRegisterMutationStyleInner() {
    JavaOnlyMap res = new JavaOnlyMap();
    LynxAccessibilityModule spyModule = spy(mLynxAccessibilityModule);
    // (1) invalid env
    JavaOnlyArray styles = new JavaOnlyArray();
    styles.pushString(PropsConstants.BACKGROUND_COLOR);
    styles.pushString(PropsConstants.BACKGROUND_IMAGE);
    styles.pushString(PropsConstants.OPACITY);
    JavaOnlyMap params = new JavaOnlyMap();
    params.putArray(MSG_MUTATION_STYLES, styles);
    spyModule.registerMutationStyleInner(params, res);
    assertTrue(res.getString(LynxAccessibilityModule.MSG).contains(MSG_FAIL));
    // (2) valid env and parameters
    LynxAccessibilityWrapper spyWrapper = spy(mA11yWrapper);
    registerWrapper(mUIBody, spyWrapper);
    registerMutationHelper(spyWrapper, mMutationHelper);
    spyModule.mLynxContext = mLynxContext;
    // (2.1)
    Mockito.when(spyWrapper.enableDelegate()).thenReturn(false);
    spyModule.registerMutationStyleInner(params, res);
    assertTrue(res.getString(LynxAccessibilityModule.MSG).contains(MSG_FAIL));
    // (2.2)
    Mockito.when(spyWrapper.enableDelegate()).thenReturn(true);
    spyModule.registerMutationStyleInner(params, res);
    assertTrue(res.getString(LynxAccessibilityModule.MSG).contains(MSG_SUCCESS));
    // (3) invalid parameters
    params.putArray(MSG_MUTATION_STYLES, null);
    Mockito.when(spyWrapper.enableDelegate()).thenReturn(true);
    spyModule.registerMutationStyleInner(params, res);
    assertTrue(res.getString(LynxAccessibilityModule.MSG).contains(MSG_FAIL));
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

  private void registerWrapper(UIBody uiBody, LynxAccessibilityWrapper a11yWrapper) {
    try {
      Field field = UIBody.class.getDeclaredField("mA11yWrapper");
      if (field != null) {
        field.setAccessible(true);
        field.set(uiBody, a11yWrapper);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }

  private void registerDelegate(LynxAccessibilityDelegate delegate) {
    try {
      Field field = LynxAccessibilityWrapper.class.getDeclaredField("mDelegate");
      if (field != null && mA11yWrapper != null) {
        field.setAccessible(true);
        field.set(mA11yWrapper, delegate);
      }
    } catch (NoSuchFieldException e) {
      e.printStackTrace();
    } catch (IllegalAccessException e) {
      e.printStackTrace();
    }
  }
}
