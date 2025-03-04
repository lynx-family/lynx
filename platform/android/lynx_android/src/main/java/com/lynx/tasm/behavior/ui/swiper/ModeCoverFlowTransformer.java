// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.swiper;

import android.view.View;
import androidx.core.math.MathUtils;

public class ModeCoverFlowTransformer implements ViewPager.PageTransformer {
  @Override
  public void transformPage(ViewPager viewPager, View page, boolean isVertical, int offset) {
    if (viewPager != null && page != null) {
      int expectSize = viewPager.getChildExpectSize();
      float rotationValue = 0;
      if (expectSize != 0) {
        rotationValue = ((float) offset) / expectSize;
      }
      rotationValue = MathUtils.clamp(rotationValue * 9f, -9f, 9f);
      page.setCameraDistance(1280);
      if (isVertical) {
        page.setRotationX(rotationValue);
      } else {
        page.setRotationY(-rotationValue);
      }
    }
  }

  @Override
  public void reset(View page) {
    if (page != null) {
      page.setRotationX(0);
      page.setRotationY(0);
    }
  }
}
