// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.accessibility;

import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;

public class LynxAccessibilityDelegateTest {
  private LynxContext mContext;
  private UIBody mUIBody;
  private LynxAccessibilityDelegate mDelegate;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
    mUIBody = new UIBody(mContext, new UIBody.UIBodyView(mContext));
    mDelegate = new LynxAccessibilityDelegate(mUIBody);
  }
}
