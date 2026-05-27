// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.svg

import android.content.Context
import android.graphics.Bitmap
import android.graphics.Picture
import android.graphics.Rect
import android.text.TextUtils
import android.util.Log
import android.util.Base64
import com.lynx.serval.svg.SVGDrawable
import com.lynx.serval.svg.SVGRender
import com.lynx.serval.svg.SVGRender.BitmapRequestCallBack
import com.lynx.serval.svg.SVGRender.ResourceManager
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.PropsConstants
import com.lynx.tasm.behavior.ui.LynxUI
import com.lynx.tasm.core.LynxThreadPool
import com.lynx.tasm.event.LynxDetailEvent
import com.lynx.tasm.resourceprovider.LynxResourceRequest
import com.lynx.tasm.resourceprovider.LynxResourceCallback
import com.lynx.tasm.resourceprovider.LynxResourceResponse
import com.lynx.tasm.utils.UIThreadUtils

import com.lynx.tasm.resourceprovider.LynxResourceRequest.LynxResourceType.LynxResourceTypeSVG
import java.nio.charset.StandardCharsets
import java.nio.charset.Charset
import com.lynx.tasm.behavior.LynxProp


@LynxGeneratorName(packageName = "com.lynx.xelement.svg")
@LynxBehavior(tagName = ["svg"], isCreateAsync = false)
open class LynxUISVG(context: LynxContext, params: Any?) : LynxUI<SVGImageView>(context, params) {

  companion object Companion { 
    const val TAG: String = "LynxUISVG";
    private const val sSvgBase64Scheme: String = "data:image/svg+xml;base64";
    const val SVG_LOAD_EVENT: String = "load";
    private const val SVG_CURRENT_COLOR_PROP: String = "current-color";
   }

  private var mSrc: String? = null

  @Volatile
  private var mContent: String? = null
  @Volatile
  private var mColor: String? = null
  private var mSvgResourceManager: SvgDefaultResourceManager? = null

  @Volatile
  private var mNeedRender = false

  @Volatile
  private var mSVGRender: SVGRender? = null

  @Volatile
  private var mIsDestroyed = false

  constructor(context: LynxContext) : this(context, null)

  init {
    mSvgResourceManager = SvgDefaultResourceManager(context, this)
    if (mSVGRender == null) {
      initSVGRender()
    } 
  }

    override fun createView(context: Context?): SVGImageView {
        return SVGImageView(context!!)
    }

      @LynxProp(name = PropsConstants.SRC)
      fun setSource(sources: String) {
        mSrc = sources
        mNeedRender = true
      }

      @LynxProp(name = "content")
      fun setContent(content: String?) {
        mContent = content
        mNeedRender = true
      }

      @LynxProp(name = SVG_CURRENT_COLOR_PROP)
      fun setColor(color: String?) {
        val normalizedColor = color?.trim()?.takeIf { it.isNotEmpty() }
        if (normalizedColor == mColor) {
          return
        }
        mColor = normalizedColor
        mNeedRender = true
      }


      override fun onNodeReady() {
        super.onNodeReady()
        if (mNeedRender) {
          mNeedRender = false
          if (TextUtils.isEmpty(mContent) && TextUtils.isEmpty(mSrc)) {
            mView.setImageDrawable(null)
            return
          }else if(!TextUtils.isEmpty(mSrc)){
            if (tryDecodeBase64AndUpdate()) {
              return
            }
            if (mContext.getGenericResourceFetcher() != null) {
              val resourceRequest: LynxResourceRequest = LynxResourceRequest(mSrc, LynxResourceTypeSVG)
              mContext.getGenericResourceFetcher()!!.fetchResource(
                resourceRequest,
                object : LynxResourceCallback<ByteArray?> {
                  override fun onResponse(response: LynxResourceResponse<ByteArray?>) {
                    if (response.state == LynxResourceResponse.ResponseState.SUCCESS) {
                      val data = response.data
                      if (data == null) {
                        Log.e(TAG, "data is empty!")
                      } else {
                        setServalSVGDrawable(String(data, StandardCharsets.UTF_8), true)
                      }
                    } else {
                      Log.e(TAG, "requestSrc error! the src is " + mSrc + " error message is" + response.error)
                    }
                  }
                }
              )
            }else {
              Log.e(TAG,"getGenericResourceFetcher is null, svg fetch src failed! " + mSrc)
            }
          }else if(!TextUtils.isEmpty(mContent)){
            setServalSVGDrawable(mContent, true)
          }
        }
      }


      override fun onLayoutUpdated() {
        super.onLayoutUpdated()
        mNeedRender = true
      }

      /**
       * Check if the src is a base64 svg data begin with "data:image/svg+xml;base64", and
       * set the decoded data as content.
       * @return Whether the mSrc string is a base64 svg data.
       */
      private fun tryDecodeBase64AndUpdate(): Boolean {
        if (mSrc!!.startsWith(sSvgBase64Scheme)) {
          val base64Data: String = mSrc!!.substring(sSvgBase64Scheme.length + 1)
          val svgData: String = String(
            Base64.decode(
              base64Data.toByteArray(Charset.defaultCharset()),
              Base64.DEFAULT
            ),
            Charset.defaultCharset()
          )
          if (TextUtils.isEmpty(svgData)) {
            mView.setImageDrawable(null)
            mContent = null
            return true
          }
          setServalSVGDrawable(svgData, true)
          return true
        }
        return false
      }

      private fun initSVGRender() {
        LynxThreadPool.getSvgRenderExecutor().execute(object : java.lang.Runnable {
          override fun run() {
            if (mIsDestroyed) { //
              return
            }
            // step 1: set local variable
            val localRender: SVGRender = SVGRender()
            // step 2: keep the local variable safe
            localRender.setResourceManager(object : ResourceManager {
              override fun requestBitMap(url: String?, callBack: BitmapRequestCallBack) {
                mSvgResourceManager?.requestBitmapSync(url!!, object :
                  SvgDefaultResourceManager.BitmapLoadCallback {
                  public override fun onSuccess(bitmap: Bitmap?) {
                    callBack.onSuccess(bitmap)
                  }

                  public override fun onFailed() {
                    callBack.onFailed()
                  }
                })
              }
            })
            // step 3: set the 'mSVGRender' variable
            mSVGRender = localRender
          }
        })
      }

      public fun setServalSVGDrawable(content: String?, needCallBack: Boolean) {
        if (TextUtils.isEmpty(content)) {
          mView.setImageDrawable(null)
          return
        }
        LynxThreadPool.getSvgRenderExecutor().execute(object : java.lang.Runnable {
          override fun run() {
            try {
              mSVGRender?.let {
                mSVGRender!!.setColor(mColor)
                val picture: Picture? = mSVGRender!!.renderPicture(
                  content,
                  Rect(0, 0, getWidth(), getHeight())
                )
                val svgDrawable: SVGDrawable = SVGDrawable(picture)
                UIThreadUtils.runOnUiThread(object : java.lang.Runnable {
                  override fun run() {
                    mView.setImageDrawable(svgDrawable)
                    invalidate()
                    if (needCallBack) {
                      onLoadSuccess()
                    }
                  }
                })
              }

            } catch (e: java.lang.Exception) {
              Log.e(
                TAG, "setDrawable: the content: " + mContent +
                  ", the error message: " + e.message
              )
            }
          }
        })
      }

      public fun invalidateSVG() {
        setServalSVGDrawable(mContent, false)
      }

      override fun onDetach() {
        super.onDetach()
        recycleResources()
      }

      private fun onLoadSuccess() {
    if (mEvents != null && mEvents.containsKey(SVG_LOAD_EVENT)) {
      val event: LynxDetailEvent = LynxDetailEvent(sign, SVG_LOAD_EVENT)
      event.addDetail("height", height.toFloat())
      event.addDetail("width", width.toFloat())
      lynxContext.eventEmitter.sendCustomEvent(event)
    }
  }

      override fun destroy() {
        super.destroy()
        mIsDestroyed = true // set 'mIsDestroyed' flag to true
        mSVGRender = null
        recycleResources()
      }


      fun recycleResources() {
          mSvgResourceManager?.destroy()
      }
    }
