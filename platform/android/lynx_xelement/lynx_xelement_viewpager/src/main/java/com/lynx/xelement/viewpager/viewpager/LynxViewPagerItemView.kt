// Copyright 2024 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.viewpager.childitem

import android.content.Context
import com.lynx.xelement.viewpager.viewpager.BaseCustomViewPager
import com.lynx.tasm.behavior.ui.view.AndroidView

class LynxViewPagerItemView(
  context: Context?,
  private val viewpagerItem: LynxViewpagerItem
) : AndroidView(context) {

  override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
    super.onMeasure(widthMeasureSpec, heightMeasureSpec)
    //  When the child view is not measured on the screen, the widthFactor will be 0, resulting in a width of 0
    if (measuredWidth == 0) {
      setMeasuredDimension((parent as? BaseCustomViewPager)?.measuredWidth ?: viewpagerItem.width, MeasureSpec.getSize(heightMeasureSpec))
      if (mDrawChildHook != null) {
        mDrawChildHook.performMeasureChildrenUI()
      }
    }
  }
}
