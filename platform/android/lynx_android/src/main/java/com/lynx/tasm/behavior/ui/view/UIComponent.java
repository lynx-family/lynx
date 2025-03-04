// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.view;

import android.content.Context;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.PropsConstants;

public class UIComponent extends UIView {
  private OnUpdateListener mOnUpdateListener;
  private String mType;
  private String mItemKey;
  private int mZIndex = 0;

  public UIComponent(LynxContext context) {
    super(context);
    if (context.getDefaultOverflowVisible()) {
      mOverflow = OVERFLOW_XY;
    }
  }

  @Override
  protected AndroidView onCreateView(Context context) {
    return new ComponentView(context);
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    if (mOnUpdateListener != null) {
      mOnUpdateListener.onLayoutUpdated(this);
    }
  }

  public void setOnUpdateListener(OnUpdateListener listener) {
    mOnUpdateListener = listener;
  }

  public OnUpdateListener getOnUpdateListener() {
    return mOnUpdateListener;
  }

  public String getType() {
    return mType;
  }

  public void setType(String type) {
    this.mType = type;
  }

  @Override
  public void destroy() {
    super.destroy();
    mOnUpdateListener = null;
  }

  public interface OnUpdateListener {
    void onLayoutUpdated(UIComponent ui);
  }

  @Override
  public boolean isAccessibilityHostUI() {
    return true;
  }

  @LynxProp(name = PropsConstants.ITEM_KEY)
  public void setItemKey(String key) {
    mItemKey = key;
  }

  public String getItemKey() {
    return mItemKey;
  }

  @LynxProp(name = PropsConstants.Z_INDEX, defaultInt = 0)
  public void setZIndex(int zIndex) {
    mZIndex = zIndex;
  }

  public int getZIndex() {
    return mZIndex;
  }
}
