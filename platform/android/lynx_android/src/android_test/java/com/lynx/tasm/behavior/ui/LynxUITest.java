// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.os.IBinder;
import android.view.View;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.stubbing.Answer;

public class LynxUITest {
  private LynxUI mUi;

  @Before
  public void setUp() throws Exception {
    mUi = mock(LynxUI.class);
  }

  @After
  public void tearDown() throws Exception {
    mUi = null;
  }

  @Test
  public void testIsVisible() {
    doCallRealMethod().when(mUi).isVisible();
    View view = mock(View.class);
    // mView is null
    assertFalse(mUi.isVisible());
    mUi.mView = view;
    when(view.getVisibility())
        .thenReturn(View.INVISIBLE)
        .thenReturn(View.GONE)
        .thenReturn(View.VISIBLE);
    assertFalse(mUi.isVisible());
    assertFalse(mUi.isVisible());
    when(view.getAlpha()).thenReturn(0f).thenReturn(1f);
    assertFalse(mUi.isVisible());
    when(view.isAttachedToWindow()).thenReturn(false).thenReturn(true);
    assertFalse(mUi.isVisible());
    assertTrue(mUi.isVisible());
    // check getWindowToken
    when(view.isAttachedToWindow()).thenAnswer(new Answer<Boolean>() {
      @Override
      public Boolean answer(InvocationOnMock invocation) throws Throwable {
        return view.getWindowToken() != null;
      }
    });
    assertFalse(mUi.isVisible());
    when(view.getWindowToken()).thenReturn(mock(IBinder.class));
    assertTrue(mUi.isVisible());
  }
}
