// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.list;
import static org.junit.Assert.assertEquals;

import android.view.View;
import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.testing.base.TestingUtils;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxSnapHelperTest {
  private LynxContext lynxContext;
  @Before
  public void setUp() throws Exception {
    lynxContext = TestingUtils.getLynxContext();
  }

  @Test
  public void testSnap() {
    ScrollView scrollView = new ScrollView(lynxContext);
    scrollView.setLayoutParams(new ViewGroup.LayoutParams(300, 500));
    LinearLayout linearLayout = new LinearLayout(lynxContext);
    linearLayout.setOrientation(LinearLayout.VERTICAL);
    scrollView.addView(linearLayout,
        new FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT, FrameLayout.LayoutParams.WRAP_CONTENT));
    for (int i = 0; i < 10; i++) {
      AndroidView view = new AndroidView(lynxContext);
      UIView ui = new UIView(lynxContext);
      view.bindDrawChildHook(ui);
      ui.setTop(500 * i);
      ui.setHeight(500);
      view.setLayoutParams(new ViewGroup.LayoutParams(300, 500));
      linearLayout.addView(view);
    }

    double snapAlignmentMillisecondsPerPx = 100f / lynxContext.getScreenMetrics().densityDpi;

    LynxSnapHelper snapHelper = new LynxSnapHelper(
        0, 20, snapAlignmentMillisecondsPerPx, new LynxSnapHelper.LynxSnapHooks() {
          @Override
          public int getScrollX() {
            return scrollView.getScrollX();
          }

          @Override
          public int getScrollY() {
            return scrollView.getScrollY();
          }

          @Override
          public int getScrollHeight() {
            return scrollView.getHeight();
          }

          @Override
          public int getScrollWidth() {
            return scrollView.getWidth();
          }

          @Override
          public int getVirtualChildrenCount() {
            return 3;
          }

          @Override
          public int getChildrenCount() {
            return 3;
          }

          @Override
          public View getChildAtIndex(int index) {
            return linearLayout.getChildAt(index);
          }

          @Override
          public View getViewAtPosition(int position) {
            return linearLayout.getChildAt(position);
          }

          @Override
          public int getIndexFromView(View view) {
            return linearLayout.indexOfChild(view);
          }

          @Override
          public void willSnapTo(int position, int currentOffsetX, int currentOffsetY,
              int targetOffsetX, int targetOffsetY) {}
        });
    scrollView.scrollTo(0, 20);
    int[] out = snapHelper.findTargetSnapOffset(0, 100, true, false);
    assertEquals(out[0], 0);
    assertEquals(out[1], 20);
  }
}
