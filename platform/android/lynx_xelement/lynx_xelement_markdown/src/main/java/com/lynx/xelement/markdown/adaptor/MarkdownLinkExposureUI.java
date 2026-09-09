// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown.adaptor;

import android.graphics.RectF;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;

public class MarkdownLinkExposureUI extends LynxFlattenUI {
  private String mUniqueID;
  private final String mUrl;
  private final String mContent;
  public MarkdownLinkExposureUI(
      LynxContext context, RectF rect, String url, String content, String uniqueID) {
    super(context);
    mUniqueID = uniqueID;
    mUrl = url;
    mContent = content;
    updateUIRect(rect);
  }

  public String getUniqueID() {
    return mUniqueID;
  }

  private void updateUIRect(RectF rect) {
    setWidth((int) rect.width());
    setHeight((int) rect.height());
    setTop((int) rect.top);
    setLeft((int) rect.left);
  }

  public JavaOnlyMap getData() {
    JavaOnlyMap data = new JavaOnlyMap();
    data.put("url", mUrl);
    data.put("content", mContent);
    return data;
  }

  public JavaOnlyMap getOption() {
    JavaOnlyMap customOption = new JavaOnlyMap();
    customOption.put("sendCustom", true);
    customOption.put("specifyTarget", true);
    customOption.put("bindEventName", "childrenexpose");

    return customOption;
  }

  public LynxBaseUI getExposeReceiveTarget() {
    return getParentBaseUI();
  }

  public String getExposeUniqueID() {
    return mUniqueID;
  }

  public float getExposureScreenMarginLeft() {
    return getExposeReceiveTarget().getExposureScreenMarginLeft();
  }

  public float getExposureScreenMarginRight() {
    return getExposeReceiveTarget().getExposureScreenMarginRight();
  }

  public float getExposureScreenMarginTop() {
    return getExposeReceiveTarget().getExposureScreenMarginTop();
  }

  public float getExposureScreenMarginBottom() {
    return getExposeReceiveTarget().getExposureScreenMarginBottom();
  }

  public boolean getEnableExposureUIMargin() {
    return getExposeReceiveTarget().getEnableExposureUIMargin();
  }

  @Override
  public EventTarget hitTest(float x, float y, boolean ignoreUserInteraction) {
    return getExposeReceiveTarget();
  }
}
