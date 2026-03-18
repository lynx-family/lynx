// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.viewpager.childitem

import android.content.Context
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxProp
import com.lynx.tasm.behavior.ui.UIGroup
import com.lynx.tasm.event.EventsListener
import com.lynx.tasm.event.LynxDetailEvent

/**
 *@author chenkai.cq
 *@date 11/13/20 10:36
 */
open class LynxViewpagerItem(context: LynxContext, params: Any?) : UIGroup<LynxViewPagerItemView>(context, params) {

    private var tag: String? = null

    constructor(context: LynxContext) : this(context, null)

    companion object {
        const val X_ELEMENT_TAG = "viewpager-item"
        const val BIND_ON_ATTACH = "attach"
    }

    private var mPropChaneListener: IPropChaneListener? = null
    private var mEnableIsAttached: Boolean = false
    override fun createView(context: Context?): LynxViewPagerItemView = LynxViewPagerItemView(context, this)

    @LynxProp(name = "tag")
    fun setTag(tag: String) {
        this.tag = tag
        mPropChaneListener?.onTagChange(tag)
    }

    fun getTag(): String {
        return tag.toString()
    }


    override fun setEvents(events: MutableMap<String, EventsListener>?) {
        super.setEvents(events)
        if (events != null) {
            mEnableIsAttached = events.containsKey(BIND_ON_ATTACH)
        }
    }

    fun setPropChaneListener(mPropChaneListener: IPropChaneListener) {
        this.mPropChaneListener = mPropChaneListener
    }

    interface IPropChaneListener {
        fun onTagChange(tag: String)
    }

    //通知前端，当前这个页面被销毁，不可见，移除window等情况
    fun sendIsAttachedEvent(attach: Boolean, index: Int) {

        if (!mEnableIsAttached) return

        lynxContext.eventEmitter.sendCustomEvent(
            LynxDetailEvent(
                sign,
                BIND_ON_ATTACH
            ).apply {
                addDetail("attach", attach)
                addDetail("tag", this@LynxViewpagerItem.getTag())
                addDetail("index", index)
            })

    }
}
