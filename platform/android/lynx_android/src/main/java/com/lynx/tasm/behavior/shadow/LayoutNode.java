// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow;

import com.lynx.tasm.base.CalledByNative;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LayoutNodeManager;

public class LayoutNode {
  protected LayoutNodeManager layoutNodeManager;

  private int mSignature;

  private boolean mHasMeasureFucSet = false;
  private boolean mHasCustomMeasureFuncSet = false;
  private boolean mIsDirty = false;
  private MeasureFunc mMeasureFunc;
  private CustomMeasureFunc mCustomMeasureFunc = null;
  private Style mStyle;

  public long mBaseline = 0;

  public LayoutNode() {}

  public void attachNativePtr(long ptr) {
    mStyle = new Style(this);
    if (!mHasMeasureFucSet && mMeasureFunc != null) {
      setMeasureFunc(mMeasureFunc);
    } else if (!mHasCustomMeasureFuncSet && mCustomMeasureFunc != null) {
      setCustomMeasureFunc(mCustomMeasureFunc);
    }
  }

  public final int getSignature() {
    return mSignature;
  }

  public void setSignature(int signature) {
    mSignature = signature;
  }

  public void setLayoutNodeManager(LayoutNodeManager manager) {
    layoutNodeManager = manager;
  }

  public void destroy() {
    layoutNodeManager = null;
  }

  /**
   * This will be called once when node is dirty during one layout loop. Only nodes that
   * set measure function can receive this callback.
   *
   * You can override this method for preparing variable that measurement needs!
   */
  public void onLayoutBefore() {}
  public void onLayout(int left, int top, int width, int height) {
    mIsDirty = false;
  }

  public void setMeasureFunc(MeasureFunc measureFunc) {
    mMeasureFunc = measureFunc;
    if (layoutNodeManager != null) {
      mHasMeasureFucSet = true;
      layoutNodeManager.setMeasureFunc(mSignature, this);
    }
  }

  public void setCustomMeasureFunc(CustomMeasureFunc customMeasureFunc) {
    mCustomMeasureFunc = customMeasureFunc;
    if (layoutNodeManager != null) {
      mHasCustomMeasureFuncSet = true;
      layoutNodeManager.setMeasureFunc(mSignature, this);
    }
  }

  /**
   * Dirty node will be layout
   * @return
   */
  public boolean isDirty() {
    if (mIsDirty) {
      return mIsDirty;
    } else if (layoutNodeManager.isDirty(mSignature)) {
      mIsDirty = true;
    }
    return mIsDirty;
  }

  public void markDirty() {
    if (!mIsDirty) {
      mIsDirty = true;
      layoutNodeManager.markDirty(mSignature);
    }
  }

  public void resetIsDirty() {
    mIsDirty = false;
  }

  /**
   * Style is not thread safe, don't use it outside the thread that LayoutNode lives in.
   * @return
   */
  public Style getStyle() {
    return mStyle;
  }

  /* package */ int getFlexDirection() {
    return layoutNodeManager.getFlexDirection(mSignature);
  }

  /* package */ float getWidth() {
    return layoutNodeManager.getWidth(mSignature);
  }

  /* package */ float getHeight() {
    return layoutNodeManager.getHeight(mSignature);
  }

  /* package */ int[] getPadding() {
    return layoutNodeManager.getPadding(mSignature);
  }

  /* package */ int[] getMargins() {
    return layoutNodeManager.getMargin(mSignature);
  }

  @CalledByNative
  public float[] measure(
      float width, int widthMode, float height, int heightMode, boolean finalMeasure) {
    float[] ret = new float[3];
    if (mMeasureFunc != null) {
      long result = mMeasureFunc.measure(
          this, width, MeasureMode.fromInt(widthMode), height, MeasureMode.fromInt(heightMode));
      ret[0] = MeasureOutput.getWidth(result);
      ret[1] = MeasureOutput.getHeight(result);
      ret[2] = mBaseline;
    } else if (mCustomMeasureFunc != null) {
      MeasureContext context = new MeasureContext(finalMeasure);
      MeasureParam param = new MeasureParam();
      param.updateConstraints(
          width, MeasureMode.fromInt(widthMode), height, MeasureMode.fromInt(heightMode));
      MeasureResult measureResult = mCustomMeasureFunc.measure(param, context);
      ret[0] = measureResult.getWidthResult();
      ret[1] = measureResult.getHeightResult();
      ret[2] = measureResult.getBaselineResult();
    }
    return ret;
  }

  @CalledByNative
  public void align() {
    if (mCustomMeasureFunc != null) {
      AlignParam param = new AlignParam();
      AlignContext context = new AlignContext();
      mCustomMeasureFunc.align(param, context);
    }
  }
}
