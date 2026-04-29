// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.blur;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RenderNode;
import android.os.Build;
import android.renderscript.Allocation;
import android.renderscript.Element;
import android.renderscript.RSRuntimeException;
import android.renderscript.RenderScript;
import android.renderscript.ScriptIntrinsicBlur;
import android.view.View;
import android.view.ViewTreeObserver;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.utils.BackgroundDrawable;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.core.LynxThreadPool;
import com.lynx.tasm.utils.BlurUtils;

public class BlurView extends AndroidView implements ViewTreeObserver.OnPreDrawListener {
  private static final float DEFAULT_SCALE_FACTOR = 6f;
  private static final int ROUNDING_VALUE = 64;
  private RenderNode mRenderNode;
  private static final String TAG = BlurView.class.getSimpleName();
  private float mBlurRadius;
  private boolean mEnableBlurAutoUpdate = true;
  private boolean mInitialized;
  private float mBlurSampling = DEFAULT_SCALE_FACTOR;
  private Bitmap mBlurBitmap;
  private BlurViewCanvas mBlurCanvas;
  private boolean setHasRadiusIfRadiusChanged = false;
  private int[] mRootLocation;
  private int[] mBlurViewLocation;
  private LynxUIBlurView mLynxUI;
  private View mTargetView;
  private final Rect mCaptureClipBounds = new Rect();
  private boolean mHasCaptureClipBounds;
  private boolean mPreDrawRegistered;
  private boolean mExperimentalUpdateBlurRadius = true;
  private boolean mPreDrawListenerDetached;

  private volatile RenderScript mRenderScript;

  private volatile ScriptIntrinsicBlur mScriptRenderBlur;

  private volatile boolean mIsDestroy = false;

  private Allocation mOutAllocation;

  private int mLastBitmapWidth;

  private int mLastBitmapHeight;

  private boolean mNeedReBlur = false;

  private volatile boolean mIsAttachToWindow = false;
  private boolean mNeedsBlurUpdate = false;

  public BlurView(Context context) {
    super(context);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      // RenderNode has better rendering performance, but it only supported after Android 29
      mRenderNode = new RenderNode("BlurView");
    }
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
      // it is not supported by default due to poor performance of RenderScript before Android 23
      mEnableBlurAutoUpdate = false;
    } else if (Build.VERSION.SDK_INT < 31) {
      // TODO(chengjunnan): upgrade compileSdkVersion and use VERSION_CODES.S
      // before Android 31 we use RenderScript to blur backdrop
      LynxThreadPool.getBriefIOExecutor().execute(new Runnable() {
        @Override
        public void run() {
          initRenderScript();
        }
      });
    }
  }

  private synchronized void initRenderScript() {
    TraceEvent.beginSection(TraceEventDef.BLUR_VIEW_CREATE_RENDER_SCRIPT);
    mRenderScript = RenderScript.create(getContext());
    mScriptRenderBlur = ScriptIntrinsicBlur.create(mRenderScript, Element.U8_4(mRenderScript));
    TraceEvent.endSection(TraceEventDef.BLUR_VIEW_CREATE_RENDER_SCRIPT);
    // the script may not have been created yet when the view is destroyed
    if (mIsDestroy) {
      destroyScript();
      return;
    }
    if (!mIsAttachToWindow) {
      return;
    }
    // make sure drawing is handled on the main thread
    post(new Runnable() {
      @Override
      public void run() {
        TraceEvent.beginSection(TraceEventDef.BLUR_VIEW_RE_BLUR);
        if (mNeedReBlur) {
          renderScriptBlur();
          if (mRenderNode != null) {
            Canvas canvas = mRenderNode.beginRecording();
            canvas.drawBitmap(mBlurBitmap, 0, 0, null);
            mRenderNode.endRecording();
          }
          mNeedReBlur = false;
          invalidate();
        }
        TraceEvent.endSection(TraceEventDef.BLUR_VIEW_RE_BLUR);
      }
    });
  }

  @Override
  public void draw(Canvas canvas) {
    boolean shouldDraw = innerDraw(canvas);
    if (shouldDraw) {
      super.draw(canvas);
    }
  }

  // the bitmap's size will reduce from the view's size to the corresponding multiple value of
  // mBlurSampling to improve the blur performance
  @Override
  public void setBlurSampling(int sampling) {
    super.setBlurSampling(sampling);
    mBlurSampling = sampling;
  }

  void setLynxBaseUI(LynxUIBlurView lynxBaseUI) {
    this.mLynxUI = lynxBaseUI;
  }

  // draw backdrop
  private boolean innerDraw(Canvas canvas) {
    if (mBlurRadius == 0 || !mInitialized || mNeedReBlur) {
      return true;
    }

    // use custom canvas to avoid draw self when draw backdrop
    if (canvas instanceof BlurViewCanvas) {
      return false;
    }

    float scaleFactorH = (float) getHeight() / mBlurBitmap.getHeight();
    float scaleFactorW = (float) getWidth() / mBlurBitmap.getWidth();

    canvas.save();
    clipRadius(canvas);
    canvas.scale(scaleFactorW, scaleFactorH);
    if (mBlurBitmap != null) {
      TraceEvent.beginSection(TraceEventDef.BLUR_VIEW_INNER_DRAW);
      if (mRenderNode != null && canvas.isHardwareAccelerated()) {
        canvas.drawRenderNode(mRenderNode);
      } else {
        canvas.drawBitmap(mBlurBitmap, 0, 0, null);
      }
      TraceEvent.endSection(TraceEventDef.BLUR_VIEW_INNER_DRAW);
    }
    canvas.restore();
    return true;
  }

  void setHasRadiusIfRadiusChanged(boolean hasRadius) {
    if (setHasRadiusIfRadiusChanged != hasRadius) {
      setHasRadiusIfRadiusChanged = hasRadius;
    }
  }

  // get the view where the backdrop needs to be drawn
  private void getTargetParent() {
    TraceEvent.beginSection(TraceEventDef.BLUR_VIEW_GET_TARGET_PARENT);
    if (mLynxUI == null) {
      TraceEvent.endSection(TraceEventDef.BLUR_VIEW_GET_TARGET_PARENT);
      return;
    }
    View captureTargetView = mLynxUI.getCaptureTargetView();
    if (captureTargetView != null) {
      mTargetView = captureTargetView;
      TraceEvent.endSection(TraceEventDef.BLUR_VIEW_GET_TARGET_PARENT);
      return;
    }
    clearCaptureClipBounds();
    LynxBaseUI lynxBaseUI = mLynxUI.getTargetParent();
    setTargetBlurParent(lynxBaseUI);
    TraceEvent.endSection(TraceEventDef.BLUR_VIEW_GET_TARGET_PARENT);
  }

  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b) {
    super.onLayout(changed, l, t, r, b);
    if (mEnableBlurAutoUpdate) {
      getTargetParent();
    }
  }

  // when view's size changed,we need update bitmap's size
  @Override
  protected void onSizeChanged(int width, int height, int oldWidth, int oleHeight) {
    super.onSizeChanged(width, height, oldWidth, oleHeight);
    if (mEnableBlurAutoUpdate) {
      if (setHasRadiusIfRadiusChanged && getBackground() != null) {
        getBackground().setBounds(0, 0, width, height);
      }
    }
    if (!mExperimentalUpdateBlurRadius) {
      updateBlurViewSize(width, height);
    }
  }

  @Override
  protected void onDetachedFromWindow() {
    super.onDetachedFromWindow();
    mIsAttachToWindow = false;
    if (mPreDrawRegistered) {
      getViewTreeObserver().removeOnPreDrawListener(this);
      mPreDrawListenerDetached = true;
    }
  }

  @Override
  protected void onAttachedToWindow() {
    super.onAttachedToWindow();
    mIsAttachToWindow = true;
    if (mEnableBlurAutoUpdate) {
      if (!isHardwareAccelerated()) {
        // if view set to LAYER_TYPE_SOFTWARE,may cause stackoverflow error
        LLog.e(TAG, "BlurView can't be used in not hardware-accelerated window!");
      } else {
        if (mPreDrawListenerDetached) {
          getViewTreeObserver().addOnPreDrawListener(this);
          mPreDrawListenerDetached = false;
        }
      }
    }
  }

  private void setTargetBlurParent(@Nullable LynxBaseUI rootBaseUI) {
    if (rootBaseUI == null) {
      return;
    }
    if (rootBaseUI.isFlatten() && rootBaseUI.getDrawParent() instanceof LynxUI) {
      mTargetView = ((LynxUI<?>) rootBaseUI.getDrawParent()).getView();
    } else if (rootBaseUI instanceof LynxUI) {
      mTargetView = ((LynxUI<?>) rootBaseUI).getView();
    }
  }

  public void updateBlurViewSize(int width, int height) {
    registerForPreDraw();
    if (downSampleSize(width) == 0 || downSampleSize(height) == 0) {
      setWillNotDraw(true);
      return;
    }
    int scaledWidth = roundSize(downSampleSize(width));
    float roundingScaleFactor = (float) width / scaledWidth;
    int scaledHeight = (int) Math.ceil(height / roundingScaleFactor);
    if (scaledWidth <= 0 || scaledHeight <= 0) {
      setWillNotDraw(true);
      return;
    }
    setWillNotDraw(false);
    if (mBlurBitmap == null || mBlurBitmap.getHeight() != scaledHeight
        || mBlurBitmap.getWidth() != scaledWidth) {
      TraceEvent.beginSection(TraceEventDef.BLUR_VIEW_CREATE_BITMAP);
      mBlurBitmap = Bitmap.createBitmap(scaledWidth, scaledHeight, Bitmap.Config.ARGB_8888);
      TraceEvent.endSection(TraceEventDef.BLUR_VIEW_CREATE_BITMAP);
      mBlurCanvas = new BlurViewCanvas(mBlurBitmap);
      if (mRenderNode != null) {
        mRenderNode.setPosition(0, 0, mBlurBitmap.getWidth(), mBlurBitmap.getHeight());
      }
    }
    mInitialized = true;
  }

  private void clipRadius(Canvas canvas) {
    if (setHasRadiusIfRadiusChanged && mLynxUI != null) {
      BackgroundDrawable drawable =
          mLynxUI.getLynxBackground() != null ? mLynxUI.getLynxBackground().getDrawable() : null;
      if (drawable != null) {
        Path path = drawable.getInnerClipPathForBorderRadius();
        if (path != null) {
          canvas.clipPath(path);
        }
      }
    }
  }

  // make the bitmap's size mod ROUNDING_VALUE is 0,
  // it may help avoiding an extra bitmap allocation when blur the bitmap.
  private int roundSize(int size) {
    if (size % ROUNDING_VALUE == 0) {
      return size;
    }
    return size - (size % ROUNDING_VALUE) + ROUNDING_VALUE;
  }

  private int downSampleSize(float value) {
    return (int) Math.ceil(value / mBlurSampling);
  }

  void destroy() {
    mIsDestroy = true;
    TraceEvent.beginSection(TraceEventDef.BLUR_VIEW_DESTORY);
    unregisterForPreDraw();
    if (mRenderNode != null) {
      mRenderNode.discardDisplayList();
      mRenderNode = null;
    }
    destroyScript();
    if (mOutAllocation != null) {
      mOutAllocation.destroy();
      mOutAllocation = null;
    }
    mTargetView = null;
    clearCaptureClipBounds();
    mBlurBitmap = null;
    mLynxUI = null;
    mBlurCanvas = null;
    mInitialized = false;
    TraceEvent.endSection(TraceEventDef.BLUR_VIEW_DESTORY);
  }

  // we only use renderscript on android 23 and above, in these versions the renderscript's
  // destory() is no-op
  private synchronized void destroyScript() {
    if (mRenderScript != null) {
      mRenderScript.destroy();
      mRenderScript = null;
    }
    if (mScriptRenderBlur != null) {
      mScriptRenderBlur.destroy();
      mScriptRenderBlur = null;
    }
  }

  // blur backdrop
  private void updateBlur() {
    if (!mEnableBlurAutoUpdate || !mInitialized) {
      return;
    }
    if (mHasCaptureClipBounds) {
      getTargetParent();
    }
    mBlurBitmap.eraseColor(Color.TRANSPARENT);
    mBlurCanvas.save();
    if (clipCaptureTarget()) {
      setupInternalCanvasMatrix();
      TraceEvent.beginSection(TraceEventDef.BLUR_VIEW_DRAW);
      if (mTargetView != null) {
        mTargetView.draw(mBlurCanvas);
      }
      TraceEvent.endSection(TraceEventDef.BLUR_VIEW_DRAW);
    }
    mBlurCanvas.restore();
    TraceEvent.beginSection(TraceEventDef.BLUR_VIEW_BLUR);
    if (!BlurUtils.createEffect(mRenderNode, mBlurRadius)) {
      renderScriptBlur();
    }
    if (mRenderNode != null) {
      Canvas canvas = mRenderNode.beginRecording();
      canvas.drawBitmap(mBlurBitmap, 0, 0, null);
      mRenderNode.endRecording();
    }
    TraceEvent.endSection(TraceEventDef.BLUR_VIEW_BLUR);
  }

  private void renderScriptBlur() {
    if (mRenderScript == null || mScriptRenderBlur == null) {
      if (Build.VERSION.SDK_INT < 31) {
        // TODO(chengjunnan): upgrade compileSdkVersion and use VERSION_CODES.S
        // mNeedReBlur only used before Android 31
        mNeedReBlur = true;
      }
      return;
    }
    if (mBlurBitmap == null || mBlurRadius == 0) {
      return;
    }
    Allocation inAllocation = Allocation.createFromBitmap(mRenderScript, mBlurBitmap);
    try {
      if (!canReuseAllocation(mBlurBitmap)) {
        if (mOutAllocation != null) {
          mOutAllocation.destroy();
        }
        mOutAllocation = Allocation.createTyped(mRenderScript, inAllocation.getType());
        mLastBitmapWidth = mBlurBitmap.getWidth();
        mLastBitmapHeight = mBlurBitmap.getHeight();
      }

      mScriptRenderBlur.setRadius(mBlurRadius);
      mScriptRenderBlur.setInput(inAllocation);
      mScriptRenderBlur.forEach(mOutAllocation);
      mOutAllocation.copyTo(mBlurBitmap);
    } catch (RSRuntimeException e) {
      BlurUtils.iterativeBoxBlur(mBlurBitmap, (int) mBlurRadius);
    } finally {
      inAllocation.destroy();
    }
  }

  private boolean canReuseAllocation(@NonNull Bitmap bitmap) {
    return bitmap.getHeight() == mLastBitmapHeight && bitmap.getWidth() == mLastBitmapWidth;
  }

  void setRootLocation(int[] rootLocation) {
    this.mRootLocation = rootLocation;
  }

  void setBlurViewLocation(int[] blurViewLocation) {
    this.mBlurViewLocation = blurViewLocation;
  }

  void setCaptureClipBounds(int left, int top, int right, int bottom) {
    mCaptureClipBounds.set(left, top, right, bottom);
    mHasCaptureClipBounds = true;
  }

  void clearCaptureClipBounds() {
    mCaptureClipBounds.setEmpty();
    mHasCaptureClipBounds = false;
  }

  private boolean clipCaptureTarget() {
    if (!mHasCaptureClipBounds) {
      return true;
    }
    if (mCaptureClipBounds.isEmpty()) {
      return false;
    }
    float scaleFactorH = (float) getHeight() / mBlurBitmap.getHeight();
    float scaleFactorW = (float) getWidth() / mBlurBitmap.getWidth();
    return mBlurCanvas.clipRect(mCaptureClipBounds.left / scaleFactorW,
        mCaptureClipBounds.top / scaleFactorH, mCaptureClipBounds.right / scaleFactorW,
        mCaptureClipBounds.bottom / scaleFactorH);
  }

  private void setupInternalCanvasMatrix() {
    if (mRootLocation == null || mBlurViewLocation == null || mRootLocation.length < 2
        || mBlurViewLocation.length < 2) {
      return;
    }
    int left = mBlurViewLocation[0] - mRootLocation[0];
    int top = mBlurViewLocation[1] - mRootLocation[1];

    float scaleFactorH = (float) getHeight() / mBlurBitmap.getHeight();
    float scaleFactorW = (float) getWidth() / mBlurBitmap.getWidth();

    float scaledLeftPosition = -left / scaleFactorW;
    float scaledTopPosition = -top / scaleFactorH;

    mBlurCanvas.translate(scaledLeftPosition, scaledTopPosition);
    mBlurCanvas.scale(1 / scaleFactorW, 1 / scaleFactorH);
  }

  void setBlurRadius(float radius) {
    if (radius < 0) {
      radius = 0;
    }
    if (this.mBlurRadius != radius) {
      this.mBlurRadius = radius;
      postInvalidate();
    }
  }

  void setEnableBlurAutoUpdate(boolean enabled) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
      enabled = false;
    }
    final boolean finalEnabled = enabled;
    if (mEnableBlurAutoUpdate != finalEnabled) {
      // post it to avoid drawing problems in the previous frame,if not,backdrop may be not right
      post(new Runnable() {
        @Override
        public void run() {
          mEnableBlurAutoUpdate = finalEnabled;
          if (finalEnabled) {
            getTargetParent();
            updateBlurViewSize(getWidth(), getHeight());
          } else {
            unregisterForPreDraw();
          }
          invalidate();
        }
      });
    }
  }

  void setExperimentalUpdateBlurRadius(boolean experimentalUpdateBlurRadius) {
    mExperimentalUpdateBlurRadius = experimentalUpdateBlurRadius;
  }

  void updateBlurTarget() {
    if (!mEnableBlurAutoUpdate) {
      return;
    }
    post(new Runnable() {
      @Override
      public void run() {
        getTargetParent();
        updateBlurViewSize(getWidth(), getHeight());
        updateBlur();
        invalidate();
      }
    });
  }

  private void registerForPreDraw() {
    if (!mPreDrawRegistered && mEnableBlurAutoUpdate) {
      getViewTreeObserver().addOnPreDrawListener(this);
      mPreDrawRegistered = true;
    }
  }

  private void unregisterForPreDraw() {
    getViewTreeObserver().removeOnPreDrawListener(this);
    mPreDrawRegistered = false;
    mPreDrawListenerDetached = false;
  }

  @Override
  public boolean onPreDraw() {
    TraceEvent.beginSection(TraceEventDef.BLUR_VIEW_UPDATE_BLUR);
    if (mTargetView != null && mTargetView.isDirty()) {
      mNeedsBlurUpdate = true;
      post(new Runnable() {
        @Override
        public void run() {
          if (mNeedsBlurUpdate) {
            updateBlur();
            mNeedsBlurUpdate = false;
            invalidate();
          }
        }
      });
    }
    TraceEvent.endSection(TraceEventDef.BLUR_VIEW_UPDATE_BLUR);
    return true;
  }
}
