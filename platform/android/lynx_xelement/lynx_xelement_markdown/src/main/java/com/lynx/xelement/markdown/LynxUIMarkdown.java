// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown;

import android.content.Context;
import android.graphics.RectF;
import android.view.View;
import com.lynx.markdown.Constants;
import com.lynx.markdown.MarkdownValuePack;
import com.lynx.markdown.ServalMarkdownView;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.behavior.ui.LynxUI;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.xelement.markdown.adaptor.LynxMarkdownBundle;
import com.lynx.xelement.markdown.adaptor.LynxMarkdownView;
import com.lynx.xelement.markdown.adaptor.MarkdownLinkExposureUI;
import com.lynx.xelement.markdown.adaptor.MarkdownResourceContext;
import java.util.ArrayList;

public class LynxUIMarkdown extends UIGroup<LynxMarkdownView> {
  private ServalMarkdownView mMarkdown;
  private LynxUIMarkdownShadowNode mShadowNode;
  private MarkdownResourceContext mResourceContext;
  private String mContentID = "";
  private boolean mExposeLinks;

  public LynxUIMarkdown(LynxContext context) {
    this(context, null);
  }

  public LynxUIMarkdown(LynxContext context, Object param) {
    super(context, param);
  }

  @Override
  protected LynxMarkdownView createView(Context context) {
    LynxMarkdownView view = new LynxMarkdownView(context);
    view.setExposureUpdater(this::updateLinkExposure);
    return view;
  }

  @Override
  public void updateExtraData(Object extraData) {
    super.updateExtraData(extraData);
    if (extraData instanceof LynxMarkdownBundle) {
      LynxMarkdownBundle bundle = (LynxMarkdownBundle) extraData;
      mShadowNode = bundle.mShadowNode;
      mContentID = bundle.mContentID;
      mExposeLinks = bundle.mExposeLinks;
      mResourceContext = bundle.mResourceContext;
      updateContentOffset();
      mMarkdown = mView.setBundle(bundle);
    }
  }

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    updateContentOffset();
  }

  @Override
  public void onInsertChild(LynxBaseUI child, int index) {
    super.onInsertChild(child, index);
    if (child instanceof LynxUI) {
      ((LynxUI) child).setVisibilityForView(View.INVISIBLE);
    }
  }

  private boolean ensureMarkdownReady(Callback callback) {
    if (mMarkdown == null || mShadowNode == null) {
      callback.invoke(LynxUIMethodConstants.NO_UI_FOR_NODE);
      return false;
    }
    return true;
  }

  private int toIndexType(String type) {
    return "source".equals(type) ? Constants.INDEX_TYPE_SOURCE : Constants.INDEX_TYPE_CHAR;
  }

  private int toSelectionRangeType(String type) {
    if ("word".equals(type)) {
      return Constants.CHAR_RANGE_TYPE_WORD;
    }
    if ("sentence".equals(type)) {
      return Constants.CHAR_RANGE_TYPE_SENTENCE;
    }
    if ("paragraph".equals(type)) {
      return Constants.CHAR_RANGE_TYPE_PARAGRAPH;
    }
    return Constants.CHAR_RANGE_TYPE_CHAR;
  }

  private int getContentLeftOffset() {
    return getPaddingLeft() + getBorderLeftWidth();
  }

  private int getContentTopOffset() {
    return getPaddingTop() + getBorderTopWidth();
  }

  private void updateContentOffset() {
    int left = getContentLeftOffset();
    int top = getContentTopOffset();
    if (mResourceContext != null) {
      mResourceContext.setContentOffset(left, top);
    }
    mView.setContentOffset(left, top);
  }

  private int[] getSelectionRange(
      float startX, float startY, float endX, float endY, int selectionType) {
    if (mMarkdown == null || startX < 0 || startY < 0 || endX < 0 || endY < 0) {
      return null;
    }
    if (startX != endX || startY != endY) {
      int start = mMarkdown.getCharIndexByPoint(startX, startY, Constants.INDEX_TYPE_CHAR);
      int end = mMarkdown.getCharIndexByPoint(endX, endY, Constants.INDEX_TYPE_CHAR);
      if (start < 0 || end < 0) {
        return null;
      }
      if (start == end) {
        return getSelectionRange(startX, startY, startX, startY, Constants.CHAR_RANGE_TYPE_CHAR);
      }
      if (start > end) {
        int tmp = start;
        start = end;
        end = tmp;
      }
      return new int[] {start, end};
    }
    long startRange =
        mMarkdown.getCharRangeByPoint(startX, startY, Constants.INDEX_TYPE_CHAR, selectionType);
    long endRange =
        mMarkdown.getCharRangeByPoint(endX, endY, Constants.INDEX_TYPE_CHAR, selectionType);
    int start = MarkdownValuePack.unpackPairFirst(startRange);
    int end = MarkdownValuePack.unpackPairSecond(endRange);
    if (start < 0 || end < 0) {
      return null;
    }
    if (start > end) {
      int tmp = start;
      start = end;
      end = tmp;
    }
    return new int[] {start, end};
  }

  @LynxUIMethod
  public void getContent(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    boolean hasRange = params != null && (params.hasKey("start") || params.hasKey("end"));
    String content;
    if (hasRange) {
      int start = params.getInt("start", 0);
      int end = params.getInt("end", Integer.MAX_VALUE);
      if (start > end) {
        callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "start > end");
        return;
      }
      int indexType = toIndexType(params.getString("indexType", ""));
      content = mMarkdown.getContent(start, end, indexType);
    } else {
      content = mMarkdown.getContent(0, Integer.MAX_VALUE, Constants.INDEX_TYPE_CHAR);
    }
    JavaOnlyMap result = new JavaOnlyMap();
    result.put("content", content);
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  @LynxUIMethod
  public void pauseAnimation(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    mMarkdown.pauseRenderUpdate();
    int animationStep = mMarkdown.getRenderedAnimationStep();
    if (!mShadowNode.pauseAnimation()) {
      mMarkdown.resumeRenderUpdate();
      callback.invoke(LynxUIMethodConstants.NO_UI_FOR_NODE);
      return;
    }
    JavaOnlyMap result = new JavaOnlyMap();
    result.put("animationStep", animationStep);
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  @LynxUIMethod
  public void resumeAnimation(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    int animationStep = params == null ? -1 : params.getInt("animationStep", -1);
    boolean success = mShadowNode.resumeAnimation(animationStep);
    mMarkdown.resumeRenderUpdate();
    callback.invoke(success ? LynxUIMethodConstants.SUCCESS : LynxUIMethodConstants.NO_UI_FOR_NODE);
  }

  @LynxUIMethod
  public void clearStatus(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    callback.invoke(LynxUIMethodConstants.SUCCESS);
  }

  @LynxUIMethod
  public void getTextBoundingRect(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    if (params == null) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "parameter is invalid");
      return;
    }
    int start = params.getInt("start", -1);
    int end = params.getInt("end", -1);
    if (start < 0 || end < 0 || start > end) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "parameter is invalid");
      return;
    }
    int indexType = toIndexType(params.getString("indexType", ""));
    ArrayList<RectF> boxes = mMarkdown.getTextBoundingRect(start, end, indexType);
    if (boxes.isEmpty()) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "Can not find text bounding rect.");
      return;
    }
    RectF textRect = LynxUIHelper.getRelativePositionInfo(this, params);
    JavaOnlyMap result = getTextBoundingRectFromBoxes(boxes, textRect);
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  @LynxUIMethod
  public void getCharIndexByPoint(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    Object xValue = params == null ? null : params.asHashMap().get("x");
    Object yValue = params == null ? null : params.asHashMap().get("y");
    if (!(xValue instanceof Number) || !(yValue instanceof Number)) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "parameter is invalid");
      return;
    }
    float density = getLynxContext().getScreenMetrics().density;
    float x = (float) (((Number) xValue).doubleValue() * density - getContentLeftOffset());
    float y = (float) (((Number) yValue).doubleValue() * density - getContentTopOffset());
    int indexType = toIndexType(params.getString("indexType", ""));
    int index = mMarkdown.getCharIndexByPoint(x, y, indexType);
    if (index < 0) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "can not find char index");
      return;
    }
    JavaOnlyMap result = new JavaOnlyMap();
    result.put("index", index);
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  @LynxUIMethod
  public void setTextSelection(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    if (params == null) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "parameter is invalid");
      return;
    }
    float density = getLynxContext().getScreenMetrics().density;
    float startX = (float) (params.getDouble("startX", 0) * density - getContentLeftOffset());
    float startY = (float) (params.getDouble("startY", 0) * density - getContentTopOffset());
    float endX = (float) (params.getDouble("endX", 0) * density - getContentLeftOffset());
    float endY = (float) (params.getDouble("endY", 0) * density - getContentTopOffset());
    int selectionType = toSelectionRangeType(params.getString("selectionTextType", ""));
    int[] range = getSelectionRange(startX, startY, endX, endY, selectionType);
    if (range == null) {
      mMarkdown.setTextSelection(-1, -1);
      JavaOnlyMap result = new JavaOnlyMap();
      result.putArray("boxes", new JavaOnlyArray());
      result.putArray("handles", new JavaOnlyArray());
      callback.invoke(LynxUIMethodConstants.SUCCESS, result);
      return;
    }
    mMarkdown.setTextSelection(range[0], range[1]);
    ArrayList<RectF> boxes = mMarkdown.getSelectedLineBoundingRect();
    if (boxes.isEmpty()) {
      JavaOnlyMap result = new JavaOnlyMap();
      result.putArray("boxes", new JavaOnlyArray());
      result.putArray("handles", new JavaOnlyArray());
      callback.invoke(LynxUIMethodConstants.SUCCESS, result);
      return;
    }
    RectF textRect = LynxUIHelper.getRelativePositionInfo(this, params);
    JavaOnlyMap result = getTextBoundingRectFromBoxes(boxes, textRect);

    RectF first = boxes.get(0);
    RectF last = boxes.get(boxes.size() - 1);
    JavaOnlyArray handles = new JavaOnlyArray();
    handles.pushMap(getHandleMap(first.left, first.bottom, 50.f, textRect));
    handles.pushMap(getHandleMap(last.right, last.bottom, 50.f, textRect));
    result.putArray("handles", handles);
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  @LynxUIMethod
  public void getSelectedText(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    JavaOnlyMap result = new JavaOnlyMap();
    result.put("selectedText", mMarkdown.getSelectedText());
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  @LynxUIMethod
  public void getParseResult(ReadableMap param, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    ReadableArray tags = param == null ? null : param.getArray("tags", null);
    if (tags == null || tags.size() == 0) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "param invalid: no tags");
      return;
    }

    JavaOnlyMap result = new JavaOnlyMap();
    result.put("id", mContentID);
    JavaOnlyMap rangeResult = new JavaOnlyMap();
    for (int i = 0; i < tags.size(); i++) {
      String tag = tags.getString(i);
      if (tag == null) {
        continue;
      }
      long[] ranges = mMarkdown.getSyntaxSourceRanges(tag);
      if (ranges == null || ranges.length == 0) {
        continue;
      }
      JavaOnlyArray array = new JavaOnlyArray();
      for (long packedRange : ranges) {
        JavaOnlyMap rangeMap = new JavaOnlyMap();
        rangeMap.put("start", MarkdownValuePack.unpackPairFirst(packedRange));
        rangeMap.put("end", MarkdownValuePack.unpackPairSecond(packedRange));
        array.pushMap(rangeMap);
      }
      rangeResult.put(tag, array);
    }
    result.put("result", rangeResult);
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  @LynxUIMethod
  public void getImages(ReadableMap param, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    String[] images = mMarkdown.getAllImageUrl();
    JavaOnlyArray array = new JavaOnlyArray();
    for (String image : images) {
      array.pushString(image);
    }
    JavaOnlyMap result = new JavaOnlyMap();
    result.put("images", array);
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  private JavaOnlyMap getMapFromRect(RectF textRect, RectF lineBox) {
    RectF safeTextRect = textRect == null ? new RectF() : textRect;
    JavaOnlyMap map = new JavaOnlyMap();
    float density = getLynxContext().getScreenMetrics().density;
    map.putDouble("left", (safeTextRect.left + getContentLeftOffset() + lineBox.left) / density);
    map.putDouble("top", (safeTextRect.top + getContentTopOffset() + lineBox.top) / density);
    map.putDouble("right", (safeTextRect.left + getContentLeftOffset() + lineBox.right) / density);
    map.putDouble("bottom", (safeTextRect.top + getContentTopOffset() + lineBox.bottom) / density);
    map.putDouble("width", lineBox.width() / density);
    map.putDouble("height", lineBox.height() / density);
    return map;
  }

  private JavaOnlyMap getHandleMap(float x, float y, float radius, RectF textRect) {
    RectF safeTextRect = textRect == null ? new RectF() : textRect;
    JavaOnlyMap map = new JavaOnlyMap();
    float density = getLynxContext().getScreenMetrics().density;
    map.putDouble("x", (safeTextRect.left + getContentLeftOffset() + x) / density);
    map.putDouble("y", (safeTextRect.top + getContentTopOffset() + y) / density);
    map.putDouble("radius", radius / density);
    return map;
  }

  private JavaOnlyMap getTextBoundingRectFromBoxes(ArrayList<RectF> boxes, RectF textRect) {
    JavaOnlyMap result = new JavaOnlyMap();
    if (boxes.isEmpty()) {
      return result;
    }
    RectF boundingRect = new RectF(boxes.get(0));
    for (int i = 1; i < boxes.size(); i++) {
      boundingRect.union(boxes.get(i));
    }
    result.putMap("boundingRect", getMapFromRect(textRect, boundingRect));
    JavaOnlyArray boxList = new JavaOnlyArray();
    for (RectF box : boxes) {
      boxList.pushMap(getMapFromRect(textRect, box));
    }
    result.putArray("boxes", boxList);
    return result;
  }

  private void updateLinkExposure() {
    removeChildrenExposureUI();
    if (mMarkdown == null || !mExposeLinks || mEvents == null
        || !mEvents.containsKey("childrenexpose")) {
      return;
    }
    String[] urls = mMarkdown.getLinkUrl();
    String[] contents = mMarkdown.getLinkContent();
    ArrayList<RectF> rects = mMarkdown.getLinkBoundingRect();
    for (int index = 0; index < urls.length; index++) {
      RectF rect = new RectF(rects.get(index));
      rect.offset(getContentLeftOffset(), getContentTopOffset());
      String id = getSign() + "_link_" + index;
      MarkdownLinkExposureUI child =
          new MarkdownLinkExposureUI(getLynxContext(), rect, urls[index], contents[index], id);
      insertChild(child, getChildCount());
      getLynxContext().addUIToExposedMap(child, id, child.getData(), child.getOption());
    }
  }

  @Override
  public void removeChildrenExposureUI() {
    for (int index = getChildCount() - 1; index >= 0; index--) {
      LynxBaseUI child = getChildAt(index);
      if (child instanceof MarkdownLinkExposureUI) {
        MarkdownLinkExposureUI exposure = (MarkdownLinkExposureUI) child;
        getLynxContext().removeUIFromExposedMap(exposure, exposure.getUniqueID());
        removeChild(exposure);
      }
    }
  }

  @Override
  public void destroy() {
    removeChildrenExposureUI();
    super.destroy();
    mView.destroy();
    mMarkdown = null;
    mShadowNode = null;
    mResourceContext = null;
  }
}
