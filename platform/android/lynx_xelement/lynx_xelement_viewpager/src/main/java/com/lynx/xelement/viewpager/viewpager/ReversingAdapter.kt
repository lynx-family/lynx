// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.viewpager

import android.database.DataSetObserver
import androidx.viewpager.widget.PagerAdapter
import android.util.Log
import android.view.View
import android.view.ViewGroup

class ReversingAdapter(private val mDelegete: androidx.viewpager.widget.PagerAdapter) : androidx.viewpager.widget.PagerAdapter() {
    fun getmDelegete(): androidx.viewpager.widget.PagerAdapter {
        return mDelegete
    }

    override fun destroyItem(container: ViewGroup, position: Int, `object`: Any) {
        var position = position
        if (isRtl) {
            position = count - position - 1
        }
        mDelegete.destroyItem(container, position, `object`)
    }

    private fun superNotifyDataSetChanged() {
        super.notifyDataSetChanged()
    }

    @Deprecated("")
    override fun destroyItem(container: View, position: Int, `object`: Any) {
        var position = position
        if (isRtl) {
            position = count - position - 1
        }
        mDelegete.destroyItem(container, position, `object`)
    }

    override fun getItemPosition(`object`: Any): Int {
        var position = mDelegete.getItemPosition(`object`)
        if (isRtl) {
            position = if (position == POSITION_UNCHANGED || position == POSITION_NONE) {
                // We can't accept POSITION_UNCHANGED when in RTL mode because adding items to the end of the collection adds them to the beginning of the
                // ViewPager.  Items whose positions do not change from the perspective of the wrapped adapter actually do change from the perspective of
                // the ViewPager.
                POSITION_NONE
            } else {
                count - position - 1
            }
        }
        return position
    }

    override fun getPageTitle(position: Int): CharSequence? {
        var position = position
        if (isRtl) {
            position = count - position - 1
        }
        return mDelegete.getPageTitle(position)
    }

    override fun getPageWidth(position: Int): Float {
        var position = position
        if (isRtl) {
            position = count - position - 1
        }
        return mDelegete.getPageWidth(position)
    }

    override fun getCount(): Int {
        return mDelegete.count
    }

    override fun instantiateItem(container: ViewGroup, position: Int): Any {
        var position = position
        if (isRtl) {
            position = count - position - 1
        }
        return mDelegete.instantiateItem(container, position)
    }

    @Deprecated("")
    override fun instantiateItem(container: View, position: Int): Any {
        var position = position
        if (isRtl) {
            position = count - position - 1
        }
        return mDelegete.instantiateItem(container, position)
    }

    override fun setPrimaryItem(container: ViewGroup, position: Int, `object`: Any) {
        var position = position
        if (isRtl) {
            position = count - position - 1
        }
        mDelegete.setPrimaryItem(container, position, `object`)
    }

    @Deprecated("")
    override fun setPrimaryItem(container: View, position: Int, `object`: Any) {
        var position = position
        if (isRtl) {
            position = count - position - 1
        }
        mDelegete.setPrimaryItem(container, position, `object`)
    }

    override fun isViewFromObject(view: View, o: Any): Boolean {
        return mDelegete.isViewFromObject(view, o)
    }

    var isRtl = false

    fun setmIsRtl(mIsRtl: Boolean) {
        isRtl = mIsRtl
        notifyDataSetChanged()
    }

    private class MyDataSetObserver(mParent: ReversingAdapter) : DataSetObserver() {
        val mParent: ReversingAdapter?
        override fun onChanged() {
            mParent?.superNotifyDataSetChanged()
        }

        override fun onInvalidated() {
            onChanged()
        }

        init {
            this.mParent = mParent
        }
    }

    init {
        mDelegete.registerDataSetObserver(MyDataSetObserver(this))
    }
}
