// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.helper;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxUIOwner;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class UITreeHelperTest {
  public UITreeHelper mUITreeHelper;
  public LynxUIOwner mUIOwner;

  @Before
  public void setUp() throws Exception {
    mUITreeHelper = new UITreeHelper();
    mUITreeHelper = spy(mUITreeHelper);
    mUIOwner = mock(LynxUIOwner.class);
    mUITreeHelper.attachLynxUIOwner(mUIOwner);
  }

  @After
  public void tearDown() throws Exception {
    mUITreeHelper = null;
    mUIOwner = null;
  }

  @Test
  public void getLynxUITree() {}
}
