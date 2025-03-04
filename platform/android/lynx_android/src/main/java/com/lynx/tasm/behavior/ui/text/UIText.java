// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.text;

import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_TRUE;
import static com.lynx.tasm.behavior.ui.text.AndroidText.SELECTION_CHANGE_EVENT;

import android.content.Context;
import android.graphics.RectF;
import android.text.Layout;
import android.text.TextUtils;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.*;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.accessibility.CustomAccessibilityDelegateCompat;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.tasm.gesture.GestureArenaMember;
import com.lynx.tasm.gesture.LynxNewGestureDelegate;
import com.lynx.tasm.gesture.arena.GestureArenaManager;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.handler.BaseGestureHandler;
import java.util.ArrayList;
import java.util.Map;

public class UIText
    extends UIGroup<AndroidText> implements IUIText, GestureArenaMember, LynxNewGestureDelegate {
  // key is gesture id, value is gesture handler
  private Map<Integer, BaseGestureHandler> mGestureHandlers;

  @Deprecated
  public UIText(Context context) {
    super((LynxContext) context);
  }

  public UIText(LynxContext context) {
    super(context);
    mAccessibilityElementStatus = ACCESSIBILITY_ELEMENT_TRUE;
    if (mContext.isTextOverflowEnabled()) {
      mOverflow = OVERFLOW_XY;
    }
  }

  @Override
  protected AndroidText createView(Context context) {
    return new AndroidText(context);
  }

  @Override
  public EventTarget hitTest(float x, float y) {
    return hitTest(x, y, false);
  }

  @Override
  public EventTarget hitTest(float x, float y, boolean ignoreUserInteraction) {
    if (mView == null) {
      return this;
    }
    x -= mPaddingLeft + mBorderLeftWidth;
    y -= mPaddingTop + mBorderTopWidth;

    return UITextUtils.hitTest(this, x, y, this, mView.mTextLayout, UITextUtils.getSpanned(mView),
        getView().mTextTranslateOffset, ignoreUserInteraction);
  }

  @Override
  public void updateExtraData(Object data) {
    if (data instanceof TextUpdateBundle) {
      mView.setTextBundle((TextUpdateBundle) data);
      if (mEvents != null) {
        mView.setBindSelectionChange(mEvents.containsKey(SELECTION_CHANGE_EVENT), getSign());
      }
    }
  }

  @Override
  public void onNodeReady() {
    super.onNodeReady();
    if (mView.mTextUpdateBundle != null) {
      UITextUtils.HandleInlineViewTruncated(mView.mTextUpdateBundle, this);
    }
  }

  public CharSequence getOriginText() {
    return mView == null ? "" : mView.getOriginText();
  }

  @Override
  protected void initAccessibilityDelegate() {
    super.initAccessibilityDelegate();
    if (mView != null) {
      ViewCompat.setAccessibilityDelegate(mView, new CustomAccessibilityDelegateCompat(this));
    }
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    int paddingLeft = mPaddingLeft + mBorderLeftWidth;
    int paddingRight = mPaddingRight + mBorderRightWidth;
    int paddingTop = mPaddingTop + mBorderTopWidth;
    int paddingBottom = mPaddingBottom + mBorderBottomWidth;
    mView.setPadding(paddingLeft, paddingTop, paddingRight, paddingBottom);
  }

  @Override
  @LynxProp(name = PropsConstants.ACCESSIBILITY_LABEL)
  public void setAccessibilityLabel(@Nullable Dynamic value) {
    super.setAccessibilityLabel(value);
    if (mView != null) {
      mView.setFocusable(true);
      mView.setContentDescription(getAccessibilityLabel());
    }
  }

  @Override
  @Nullable
  public CharSequence getAccessibilityLabel() {
    CharSequence s = super.getAccessibilityLabel();
    if (!TextUtils.isEmpty(s)) {
      return s;
    }
    return mView.getText();
  }

  @Deprecated
  public void setTextGradient(String gradient) {
    LLog.e("UIText", "setTextGradient(String) is deprecated");
  }

  @Deprecated
  public void setTextGradient(ReadableArray gradient) {}

  @Deprecated
  public void setColor(int color) {}

  @Deprecated
  public void setColor(Dynamic color) {}

  @LynxProp(name = "text-selection", defaultBoolean = false)
  public void setEnableTextSelection(boolean enable) {
    mView.setEnableTextSelection(enable);
  }

  @LynxProp(name = "custom-context-menu", defaultBoolean = false)
  public void setCustomContextMenu(boolean enable) {
    mView.setCustomContextMenu(enable);
  }

  @LynxProp(name = "custom-text-selection", defaultBoolean = false)
  public void setCustomTextSelection(boolean enable) {
    mView.setCustomTextSelection(enable);
  }

  @LynxProp(name = "selection-background-color", defaultInt = 0)
  public void setSelectionBackgroundColor(int color) {
    mView.updateSelectionBackgroundColor(color);
  }

  @LynxProp(name = "selection-handle-color", defaultInt = 0)
  public void setSelectionHandleColor(int color) {
    mView.updateSelectionHandleColor(color);
  }

  @LynxProp(name = "selection-handle-size", defaultInt = 0)
  public void setSelectionHandleSize(int size) {
    mView.updateSelectionHandleSize(size);
  }

  @Nullable
  @Override
  public Layout getTextLayout() {
    if (mView == null) {
      return null;
    }
    return mView.getTextLayout();
  }

  @Override
  public void setConsumeHoverEvent(boolean value) {
    super.setConsumeHoverEvent(value);
    if (mView != null) {
      mView.setConsumeHoverEvent(mConsumeHoverEvent);
    }
  }

  @Override
  public void destroy() {
    super.destroy();
    mView.release();
    // remove arena member if destroy
    GestureArenaManager manager = getGestureArenaManager();
    if (manager != null) {
      manager.removeMember(this);
    }
    // clear gesture map if destroy
    if (mGestureHandlers != null) {
      mGestureHandlers.clear();
    }
  }

  @Override
  public void copyPropFromOldUiInUpdateFlatten(LynxBaseUI oldUI) {
    super.copyPropFromOldUiInUpdateFlatten(oldUI);
    if (!(oldUI instanceof FlattenUIText)) {
      return;
    }
    updateExtraData(((FlattenUIText) oldUI).getTextBundle());
  }

  /**
   * Returns the bounding box, boundingRect, of the specified text range, as well as the bounding.
   * boxes, boxes, for each line.
   * @param params
   * start: The start position of the specified range.
   * end: The end position of the specified range.
   * @param callback
   * boundingRect: The bounding box of the selected text
   * boxes: The bounding boxes for each line
   */
  @LynxUIMethod
  public void getTextBoundingRect(ReadableMap params, Callback callback) {
    try {
      int start = params.getInt("start");
      int end = params.getInt("end");
      if (start > end || start < 0 || end < 0) {
        callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "parameter is invalid");
        return;
      }
      if (mView != null) {
        ArrayList<RectF> boxes = mView.getTextBoundingBoxes(start, end);
        if (boxes.size() > 0) {
          RectF textRect = LynxUIHelper.getRelativePositionInfo(this, params);
          JavaOnlyMap result = getTextBoundingRectFromBoxes(boxes, params, textRect);
          callback.invoke(LynxUIMethodConstants.SUCCESS, result);
          return;
        }
      }
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "Can not find text bounding rect.");
    } catch (Exception e) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, e.getMessage());
    }
  }

  /**
   * Control text selection highlighting.
   * @param params
   * startX: The x-coordinate of the start of the selected text relative to the text component
   * startY: The y-coordinate of the start of the selected text relative to the text component
   * endX: The x-coordinate of the end of the selected text relative to the text component
   * endY: The y-coordinate of the end of the selected text relative to the text component
   * @param callback
   * return value
   * boundingRect: The bounding box of the selected text
   * boxes: The bounding boxes of each line
   * handles: The cursor positions and the default radius
   */
  @LynxUIMethod
  public void setTextSelection(ReadableMap params, Callback callback) {
    try {
      float density = getLynxContext().getScreenMetrics().density;
      double startX =
          params.getDouble("startX") * density - getPaddingLeft() - getBorderLeftWidth();
      double startY = params.getDouble("startY") * density - getPaddingTop() - getBorderTopWidth();
      double endX = params.getDouble("endX") * density - getPaddingLeft() - getBorderLeftWidth();
      double endY = params.getDouble("endY") * density - getPaddingTop() - getBorderTopWidth();
      if (mView != null) {
        ArrayList<RectF> boxes =
            mView.setTextSelection((float) startX, (float) startY, (float) endX, (float) endY);
        if (boxes.size() == 0) {
          callback.invoke(LynxUIMethodConstants.SUCCESS);
        } else {
          RectF textRect = LynxUIHelper.getRelativePositionInfo(this, params);
          JavaOnlyMap result = getTextBoundingRectFromBoxes(boxes, params, textRect);
          ArrayList<Float>[] handles = mView.getHandlesInfo();
          JavaOnlyArray handleList = new JavaOnlyArray();
          for (int i = 0; i < handles.length; i++) {
            handleList.add(getHandleMap(handles[i], textRect));
          }
          result.putArray("handles", handleList);
          callback.invoke(LynxUIMethodConstants.SUCCESS, result);
        }
      } else {
        callback.invoke(LynxUIMethodConstants.NO_UI_FOR_NODE);
      }
    } catch (Exception e) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, e.getMessage());
    }
  }

  @LynxUIMethod
  public void getSelectedText(ReadableMap params, Callback callback) {
    if (mView != null) {
      String selectedText = mView.getSelectedText();
      JavaOnlyMap map = new JavaOnlyMap();
      map.put("selectedText", selectedText);
      callback.invoke(LynxUIMethodConstants.SUCCESS, map);
    } else {
      callback.invoke(LynxUIMethodConstants.NO_UI_FOR_NODE);
    }
  }

  /**
   *
   * @param textRect
   * @param lineBox
   * @return
   */
  private JavaOnlyMap getMapFromRect(RectF textRect, RectF lineBox) {
    JavaOnlyMap map = new JavaOnlyMap();
    float density = getLynxContext().getScreenMetrics().density;
    map.putDouble(
        "left", (textRect.left + getPaddingLeft() + getBorderLeftWidth() + lineBox.left) / density);
    map.putDouble(
        "top", (textRect.top + getPaddingTop() + getBorderTopWidth() + lineBox.top) / density);
    map.putDouble("right",
        (textRect.left + getPaddingLeft() + getBorderLeftWidth() + lineBox.right) / density);
    map.putDouble("bottom",
        (textRect.top + getPaddingTop() + getBorderTopWidth() + lineBox.bottom) / density);
    map.putDouble("width", lineBox.width() / density);
    map.putDouble("height", lineBox.height() / density);

    return map;
  }

  private JavaOnlyMap getHandleMap(ArrayList<Float> handle, RectF textRect) {
    JavaOnlyMap map = new JavaOnlyMap();
    float density = getLynxContext().getScreenMetrics().density;
    map.putDouble(
        "x", (textRect.left + getPaddingLeft() + getBorderLeftWidth() + handle.get(0)) / density);
    map.putDouble(
        "y", (textRect.top + getPaddingTop() + getBorderTopWidth() + handle.get(1)) / density);
    map.putDouble("radius", handle.get(2) / density);

    return map;
  }

  private JavaOnlyMap getTextBoundingRectFromBoxes(
      ArrayList<RectF> boxes, ReadableMap params, RectF textRect) {
    RectF boundingRect = new RectF(boxes.get(0));
    for (int i = 1; i < boxes.size(); i++) {
      boundingRect.union(boxes.get(i));
    }

    JavaOnlyMap result = new JavaOnlyMap();
    result.putMap("boundingRect", getMapFromRect(textRect, boundingRect));
    JavaOnlyArray boxList = new JavaOnlyArray();
    for (int i = 0; i < boxes.size(); i++) {
      boxList.add(getMapFromRect(textRect, boxes.get(i)));
    }
    result.putArray("boxes", boxList);

    return result;
  }

  public void onPropsUpdated() {
    super.onPropsUpdated();
    if (mGestureHandlers != null) {
      GestureArenaManager manager = getGestureArenaManager();
      // Check if the current UIList instance is already a member of the gesture arena
      if (manager != null && !manager.isMemberExist(getGestureArenaMemberId())) {
        // If not a member, add the UIList instance as a new member to the gesture arena
        mGestureArenaMemberId = manager.addMember(UIText.this);
      }
    }
  }

  @Override
  public void onGestureScrollBy(float deltaX, float deltaY) {
    // No need to implement if it's not a scrolling container
  }

  @Override
  public boolean canConsumeGesture(float deltaX, float deltaY) {
    // consume gesture by default if it's not a scrolling container
    return true;
  }

  @Override
  public int getMemberScrollX() {
    // always zero if it's not a scrolling container
    return 0;
  }

  @Override
  public boolean isAtBorder(boolean isStart) {
    // always false if it's not a scrolling container
    return false;
  }

  @Override
  public int getMemberScrollY() {
    // always zero if it's not a scrolling container
    return 0;
  }

  @Override
  public void onInvalidate() {
    if (!isEnableNewGesture()) {
      return;
    }
    ViewCompat.postInvalidateOnAnimation(mView);
  }

  @Override
  public void setGestureDetectors(Map<Integer, GestureDetector> gestureDetectors) {
    super.setGestureDetectors(gestureDetectors);
    if (gestureDetectors == null || gestureDetectors.isEmpty()) {
      return;
    }
    GestureArenaManager manager = getGestureArenaManager();
    if (manager == null) {
      return;
    }
    if (manager.isMemberExist(getGestureArenaMemberId())) {
      // when update gesture handlers, need to reset it
      if (mGestureHandlers != null) {
        mGestureHandlers.clear();
        mGestureHandlers = null;
      }
    }
    if (mGestureHandlers == null) {
      mGestureHandlers = BaseGestureHandler.convertToGestureHandler(
          getSign(), getLynxContext(), UIText.this, getGestureDetectorMap());
    }
    mView.setGestureManager(manager);
  }

  @Nullable
  @Override
  public Map<Integer, BaseGestureHandler> getGestureHandlers() {
    if (!isEnableNewGesture()) {
      return null;
    }

    if (mGestureHandlers == null) {
      mGestureHandlers = BaseGestureHandler.convertToGestureHandler(
          getSign(), getLynxContext(), UIText.this, getGestureDetectorMap());
    }
    return mGestureHandlers;
  }
}
