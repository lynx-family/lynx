// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.atMost;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.verify;

import android.os.Build;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.testing.base.TestingUtils;
import java.util.concurrent.TimeUnit;
import org.junit.Before;
import org.junit.Test;

public class ComponentStatisticTest {
  private LynxContext mContext;
  private LynxView mLynxView;
  private UIBody mUIBody;

  @Before
  public void setUp() throws Exception {
    mContext = TestingUtils.getLynxContext();
    mUIBody = new UIBody(mContext, new UIBody.UIBodyView(mContext));
    mLynxView = new LynxView(mContext, new LynxViewBuilder());
    mContext.setLynxView(mLynxView);
  }

  @Test
  public void componentStatistic() throws Exception {
    // Avoid reporting it in the lower version of Android.
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N) {
      return;
    }

    LynxContext contextSpy = spy(mContext);
    LynxUIOwner uiOwner =
        new LynxUIOwner(contextSpy, new BehaviorRegistry(), mUIBody.getBodyView());

    uiOwner.componentStatistic("view");
    TimeUnit.SECONDS.sleep(1);
    // Reported only when the view is first created.
    verify(contextSpy, atLeast(1)).getInstanceId();
    reset(contextSpy);

    uiOwner.componentStatistic("text");
    TimeUnit.SECONDS.sleep(1);
    // Reported only when the text is first created.
    verify(contextSpy, atLeast(1)).getInstanceId();
    reset(contextSpy);

    uiOwner.componentStatistic("text");
    TimeUnit.SECONDS.sleep(1);
    // Because the text has already been created, it will not be reported.
    verify(contextSpy, atMost(0)).getInstanceId();
  }
}
