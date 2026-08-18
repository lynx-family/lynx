// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.content.Context;
import android.view.View;
import android.widget.EditText;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.devtool.helper.UITreeHelper;
import com.lynx.devtoolwrapper.IDevToolDelegate;
import com.lynx.tasm.LynxDevToolDelegateImpl;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.LynxTemplateRender;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.behavior.ILynxUIRenderer;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class DevToolPlatformAndroidDelegateTest {
  private Context mContext;
  private LynxView mLynxView;
  private DevToolPlatformAndroidDelegate mPlatformDelegate;

  @Before
  public void setUp() {
    mContext =
        InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
    LynxEnv.inst().init((Application) mContext, null, null, null, null);
    LynxDevtoolEnv.inst().init(mContext);

    mLynxView = mock(LynxView.class);
    mPlatformDelegate = new DevToolPlatformAndroidDelegate(mLynxView);
  }

  @Test
  public void insertTextCommitsToFocusedEditText() {
    EditText editText = new EditText(mContext);
    editText.setText("ac");
    editText.setSelection(1);
    when(mLynxView.findFocus()).thenReturn(editText);

    mPlatformDelegate.insertText("b");

    assertEquals("abc", editText.getText().toString());
  }

  @Test
  public void insertTextFallsBackToRootFocusedEditText() {
    EditText editText = new EditText(mContext);
    editText.setText("acd");
    editText.setSelection(1, 2);
    View rootView = mock(View.class);
    when(rootView.findFocus()).thenReturn(editText);
    when(mLynxView.findFocus()).thenReturn(null);
    when(mLynxView.getRootView()).thenReturn(rootView);

    mPlatformDelegate.insertText("b");

    assertEquals("abd", editText.getText().toString());
  }

  @Test
  public void uiTreeMethodsUseClayRendererDelegate() {
    IDevToolDelegate devToolDelegate = mock(IDevToolDelegate.class);
    when(devToolDelegate.isClayRenderer()).thenReturn(true);
    when(devToolDelegate.getLynxUITree()).thenReturn("{\"name\":\"page\"}");
    when(devToolDelegate.getUINodeInfo(7)).thenReturn("{\"id\":7}");
    when(devToolDelegate.setUIStyle(7, "visible", "false")).thenReturn(0);
    mPlatformDelegate.setDevToolDelegate(devToolDelegate);

    assertEquals("{\"name\":\"page\"}", mPlatformDelegate.getLynxUITree());
    assertEquals("{\"id\":7}", mPlatformDelegate.getUINodeInfo(7));
    assertEquals(0, mPlatformDelegate.setUIStyle(7, "visible", "false"));
    verify(devToolDelegate).getLynxUITree();
    verify(devToolDelegate).getUINodeInfo(7);
    verify(devToolDelegate).setUIStyle(7, "visible", "false");
  }

  @Test
  public void isClayRendererDefaultsToFalse() {
    IDevToolDelegate devToolDelegate = mock(IDevToolDelegate.class, CALLS_REAL_METHODS);
    assertFalse(devToolDelegate.isClayRenderer());
  }

  @Test
  public void isClayRendererUsesCurrentRendererType() {
    LynxTemplateRender render = mock(LynxTemplateRender.class);
    ILynxUIRenderer renderer = mock(ILynxUIRenderer.class);
    when(render.lynxUIRenderer()).thenReturn(renderer);
    LynxDevToolDelegateImpl devToolDelegate = new LynxDevToolDelegateImpl(render);

    when(renderer.getRenderType()).thenReturn(ILynxUIRenderer.RenderType.CLAY);
    assertTrue(devToolDelegate.isClayRenderer());
    when(renderer.getRenderType()).thenReturn(ILynxUIRenderer.RenderType.LYNX_UI);
    assertFalse(devToolDelegate.isClayRenderer());
    when(render.lynxUIRenderer()).thenReturn(null);
    assertFalse(devToolDelegate.isClayRenderer());
    assertFalse(new LynxDevToolDelegateImpl(null).isClayRenderer());
  }

  @Test
  public void uiTreeMethodsUseNativeHelperForNonClayRenderer() throws Exception {
    IDevToolDelegate devToolDelegate = mock(IDevToolDelegate.class);
    when(devToolDelegate.isClayRenderer()).thenReturn(false);
    mPlatformDelegate.setDevToolDelegate(devToolDelegate);
    UITreeHelper helper = mock(UITreeHelper.class);
    Field helperField = DevToolPlatformAndroidDelegate.class.getDeclaredField("mUITreeHelper");
    helperField.setAccessible(true);
    helperField.set(mPlatformDelegate, helper);
    when(helper.getLynxUITree()).thenReturn("{\"name\":\"LynxUI\"}");
    when(helper.getUINodeInfo(7)).thenReturn("{\"id\":7}");
    when(helper.setUIStyle(7, "visible", "false")).thenReturn(0);

    assertEquals("{\"name\":\"LynxUI\"}", mPlatformDelegate.getLynxUITree());
    assertEquals("{\"id\":7}", mPlatformDelegate.getUINodeInfo(7));
    assertEquals(0, mPlatformDelegate.setUIStyle(7, "visible", "false"));
    verify(helper).getLynxUITree();
    verify(helper).getUINodeInfo(7);
    verify(helper).setUIStyle(7, "visible", "false");
    verify(devToolDelegate, never()).getLynxUITree();
    verify(devToolDelegate, never()).getUINodeInfo(7);
    verify(devToolDelegate, never()).setUIStyle(7, "visible", "false");
  }
}
