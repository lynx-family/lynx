// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.blur;

import android.content.Context;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxGeneratorName;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.view.UIView;
import com.lynx.tasm.utils.UnitUtils;

@LynxGeneratorName(packageName = "com.lynx.xelement.blur")
@LynxBehavior(tagName = "blur-view", isCreateAsync = false)
public class LynxUIBlurView<T extends BlurView> extends UIView {
  private static final String ANDROID_CAPTURE_TARGET = "android-capture-target";

  private int[] mChildLocation = new int[2];

  private int[] mParentLocation = new int[2];

  private int[] mCaptureTargetLocation = new int[2];

  private View mPreDrawingParent;

  private boolean mExperimentalUpdateBlurRadius = true;

  @Nullable private String mCaptureTargetId;

  public LynxUIBlurView(LynxContext context) {
    this(context, null);
  }

  public LynxUIBlurView(LynxContext context, Object params) {
    super(context, params);
  }

  @Override
  protected T createView(Context context) {
    T blurView = createBlurView(context);
    blurView.setLynxBaseUI(this);
    return blurView;
  }

  protected T createBlurView(Context context) {
    return (T) new BlurView(context);
  }

  @LynxProp(name = PropsConstants.BLUR_RADIUS, defaultInt = 0)
  public void setBlurRadius(String blurRadius) {
    if (mView != null) {
      UIBody uiBody = mContext.getUIBody();
      int radius = Math.round(UnitUtils.toPxWithDisplayMetrics(blurRadius, uiBody.getFontSize(),
          mFontSize, uiBody.getWidth(), uiBody.getHeight(), mContext.getScreenMetrics()));
      ((BlurView) mView).setBlurRadius(radius);
    }
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    if (mExperimentalUpdateBlurRadius) {
      ((BlurView) mView).updateBlurViewSize(getWidth(), getHeight());
    }
  }

  @LynxProp(name = "enable-auto-blur", defaultBoolean = true)
  public void setEnableAutoBlur(final boolean enable) {
    if (mView != null) {
      ((BlurView) mView).setEnableBlurAutoUpdate(enable);
    }
  }

  @LynxProp(name = ANDROID_CAPTURE_TARGET)
  public void setCaptureTarget(@Nullable String captureTargetId) {
    mCaptureTargetId = captureTargetId;
    if (mView != null) {
      ((BlurView) mView).updateBlurTarget();
    }
  }

  /**
   * @name: experimental-update-blur-radius
   * @description:
   * @category: experimental
   * @standardAction: remove in 3.5
   * @supportVersion: 3.4
   **/
  @LynxProp(name = "experimental-update-blur-radius")
  public void setExperimentalUpdateBlurRadius(boolean updateBlurRadius) {
    mExperimentalUpdateBlurRadius = updateBlurRadius;
    if (mView != null) {
      ((BlurView) mView).setExperimentalUpdateBlurRadius(updateBlurRadius);
    }
  }

  @LynxUIMethod
  public void enableAutoBlur(ReadableMap params) {
    if (params == null) {
      return;
    }
    boolean enable = params.hasKey("enable") ? params.getBoolean("enable") : false;

    if (mView != null) {
      ((BlurView) mView).setEnableBlurAutoUpdate(enable);
    }
  }

  private boolean isInTargetParent(@NonNull LynxBaseUI parent) {
    int childLeft = mChildLocation[0];
    int childTop = mChildLocation[1];
    int childRight = childLeft + getWidth();
    int childBottom = childTop + getHeight();
    getUIViewLocation(parent, mParentLocation);
    int parentLeft = mParentLocation[0];
    int parentTop = mParentLocation[1];
    if (parent.isFlatten()) {
      parentLeft += parent.getLeft();
      parentTop += parent.getTop();
    }
    int parentRight = parentLeft + parent.getWidth();
    int parentBottom = parentTop + parent.getHeight();
    ((BlurView) mView).setRootLocation(mParentLocation);
    return childLeft >= parentLeft && childTop >= parentTop && childRight <= parentRight
        && childBottom <= parentBottom;
  }

  LynxBaseUI getTargetParent() {
    LynxBaseUI parentBaseUI = getParentBaseUI();
    getUIViewLocation(this, mChildLocation);
    ((BlurView) mView).setBlurViewLocation(mChildLocation);
    ((BlurView) mView).clearCaptureClipBounds();
    while (!(parentBaseUI instanceof UIBody)) {
      if (parentBaseUI == null) {
        break;
      }
      if (parentBaseUI.getOverflow() == OVERFLOW_HIDDEN) {
        getUIViewLocation(parentBaseUI, mParentLocation);
        ((BlurView) mView).setRootLocation(mParentLocation);
        break;
      }
      if (isInTargetParent(parentBaseUI)) {
        break;
      }
      parentBaseUI = parentBaseUI.getParentBaseUI();
    }
    return parentBaseUI;
  }

  @Nullable
  View getCaptureTargetView() {
    if (mView == null || mCaptureTargetId == null || mCaptureTargetId.isEmpty()
        || mContext.getLynxView() == null) {
      return null;
    }
    View targetView = mContext.getLynxView().findViewByIdSelector(mCaptureTargetId);
    if (targetView == null) {
      return null;
    }
    getUIViewLocation(this, mChildLocation);
    targetView.getLocationOnScreen(mCaptureTargetLocation);
    ((BlurView) mView).setBlurViewLocation(mChildLocation);
    ((BlurView) mView).setRootLocation(mCaptureTargetLocation);
    updateCaptureTargetClipBounds(targetView);
    return targetView;
  }

  private void updateCaptureTargetClipBounds(@NonNull View targetView) {
    int blurLeft = mChildLocation[0];
    int blurTop = mChildLocation[1];
    int blurRight = blurLeft + getWidth();
    int blurBottom = blurTop + getHeight();

    int targetLeft = mCaptureTargetLocation[0];
    int targetTop = mCaptureTargetLocation[1];
    int targetRight = targetLeft + targetView.getWidth();
    int targetBottom = targetTop + targetView.getHeight();

    int intersectionLeft = Math.max(blurLeft, targetLeft);
    int intersectionTop = Math.max(blurTop, targetTop);
    int intersectionRight = Math.min(blurRight, targetRight);
    int intersectionBottom = Math.min(blurBottom, targetBottom);

    ((BlurView) mView)
        .setCaptureClipBounds(intersectionLeft - blurLeft, intersectionTop - blurTop,
            intersectionRight - blurLeft, intersectionBottom - blurTop);
  }

  private void getUIViewLocation(@NonNull LynxBaseUI ui, int[] outLocation) {
    View view;
    if (ui.isFlatten()) {
      view = ((LynxUI<?>) ui.getDrawParent()).getView();
      if (view != null && mPreDrawingParent != view) {
        mPreDrawingParent = view;
        view.getLocationOnScreen(outLocation);
      }
    } else {
      view = ((LynxUI<?>) ui).getView();
      if (view != null) {
        view.getLocationOnScreen(outLocation);
      }
    }
  }

  @Override
  public void onBorderRadiusUpdated(int index) {
    super.onBorderRadiusUpdated(index);
    if (mView != null) {
      ((BlurView) mView).setHasRadiusIfRadiusChanged(getHasRadius());
    }
  }

  @Override
  public void destroy() {
    super.destroy();
    if (mView != null) {
      ((BlurView) mView).destroy();
    }
  }
}
