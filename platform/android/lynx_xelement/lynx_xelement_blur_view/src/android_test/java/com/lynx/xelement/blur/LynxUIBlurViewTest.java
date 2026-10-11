// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.blur;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.util.DisplayMetrics;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import androidx.test.platform.app.InstrumentationRegistry;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import java.lang.reflect.Field;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
@SdkSuppress(minSdkVersion = 31)
public class LynxUIBlurViewTest {
  private LynxUIBlurView<BlurView> mLynxUIBlurView;
  private BlurView mBlurView;

  @Before
  public void setUp() {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
      @Override
      public void run() {
        Context context =
            InstrumentationRegistry.getInstrumentation().getTargetContext().getApplicationContext();
        LynxContext lynxContext = new LynxContext(context, new DisplayMetrics()) {
          @Override
          public void handleException(Exception exception) {}
        };
        mLynxUIBlurView = new LynxUIBlurView<BlurView>(lynxContext) {
          @Override
          protected BlurView createBlurView(Context viewContext) {
            return new ImmediatePostBlurView(viewContext);
          }
        };
        mBlurView = (BlurView) mLynxUIBlurView.getView();
      }
    });
  }

  // @generated-by: lynx-test-contract
  @Test
  public void enableAutoBlurUpdatesRuntimeState() throws Exception {
    JavaOnlyMap params = new JavaOnlyMap();
    params.putBoolean("enable", false);

    runMethodAndWaitForPostedUpdate(params);
    assertFalse(isAutoBlurEnabled());

    params.putBoolean("enable", true);
    runMethodAndWaitForPostedUpdate(params);
    assertTrue(isAutoBlurEnabled());
  }

  // @generated-by: lynx-test-contract
  @Test
  public void enableAutoBlurDefaultsToDisabledWhenEnableIsMissing() throws Exception {
    runMethodAndWaitForPostedUpdate(new JavaOnlyMap());

    assertFalse(isAutoBlurEnabled());
  }

  // @generated-by: lynx-test-contract
  @Test
  public void enableAutoBlurIgnoresNullParams() throws Exception {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
      @Override
      public void run() {
        mLynxUIBlurView.enableAutoBlur(null);
      }
    });

    assertTrue(isAutoBlurEnabled());
  }

  private void runMethodAndWaitForPostedUpdate(final JavaOnlyMap params) {
    InstrumentationRegistry.getInstrumentation().runOnMainSync(new Runnable() {
      @Override
      public void run() {
        mLynxUIBlurView.enableAutoBlur(params);
      }
    });
  }

  private boolean isAutoBlurEnabled() throws Exception {
    Field field = BlurView.class.getDeclaredField("mEnableBlurAutoUpdate");
    field.setAccessible(true);
    return field.getBoolean(mBlurView);
  }

  private static class ImmediatePostBlurView extends BlurView {
    ImmediatePostBlurView(Context context) {
      super(context);
    }

    @Override
    public boolean post(Runnable action) {
      action.run();
      return true;
    }
  }
}
