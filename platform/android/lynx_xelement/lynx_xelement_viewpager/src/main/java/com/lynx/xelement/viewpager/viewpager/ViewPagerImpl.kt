// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.viewpager.viewpager

import android.content.Context
import com.lynx.xelement.viewpager.Pager
import com.lynx.xelement.viewpager.childitem.LynxViewpagerItem

// Copyright 2020 The Lynx Authors. All rights reserved.


open class ViewPagerImpl(context: Context) : Pager<CustomViewPager>(
      CustomViewPager(context), context) {
  fun addChildItem(child: LynxViewpagerItem, index: Int) {
    mChanged = true
    if (index < 0 || index > mPendingChildren.size) {
      mPendingChildren.add(child)
    } else {
      mPendingChildren.add(index, child)
    }
  }

  fun setPagerChangeAnimation(enable:Boolean){
    mViewPager.mPagerChangeAnimation = enable
  }

  fun setForceCanScroll(enable: Boolean) {
    mViewPager.mForceCanScroll = enable
  }


  fun setViewPagerOverScrollMode(isAlwaysOverScroll: Boolean) {
    if (isAlwaysOverScroll) {
      mViewPager.overScrollMode = OVER_SCROLL_ALWAYS
    } else {
      mViewPager.overScrollMode = OVER_SCROLL_NEVER
    }
  }
}
