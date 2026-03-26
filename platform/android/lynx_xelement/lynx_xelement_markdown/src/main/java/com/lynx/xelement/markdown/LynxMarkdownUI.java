// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.xelement.markdown;

import android.content.Context;
import android.graphics.RectF;
import com.lynx.markdown.Constants;
import com.lynx.markdown.MarkdownValuePack;
import com.lynx.react.bridge.Callback;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.behavior.LynxBehavior;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIMethod;
import com.lynx.tasm.behavior.LynxUIMethodConstants;
import com.lynx.tasm.behavior.ui.UIGroup;
import com.lynx.tasm.behavior.ui.utils.LynxUIHelper;
import com.lynx.xelement.markdown.adaptor.LynxMarkdownBundle;
import com.lynx.xelement.markdown.adaptor.LynxMarkdownView;
import com.lynx.xelement.markdown.adaptor.LynxServalViewWrapper;
import java.util.ArrayList;

@LynxBehavior(tagName = "x-markdown", isCreateAsync = true)
public class LynxMarkdownUI extends UIGroup<LynxMarkdownView> {
  private LynxMarkdownBundle mBundle;
  private LynxServalViewWrapper mMarkdown;

  public LynxMarkdownUI(LynxContext context) {
    this(context, null);
  }

  public LynxMarkdownUI(LynxContext context, Object param) {
    super(context, param);
  }

  @Override
  protected LynxMarkdownView createView(Context context) {
    return new LynxMarkdownView(context);
  }

  @Override
  public void updateExtraData(Object extraData) {
    super.updateExtraData(extraData);
    if (extraData instanceof LynxMarkdownBundle) {
      mBundle = (LynxMarkdownBundle) extraData;
      mMarkdown = mBundle.mMarkdownView;
      mView.setBundle(mBundle);
    }
  }

  private boolean ensureMarkdownReady(Callback callback) {
    if (mMarkdown == null) {
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

  private int[] getSelectionRange(
      float startX, float startY, float endX, float endY, int selectionType) {
    if (mMarkdown == null) {
      return null;
    }
    if (selectionType == Constants.CHAR_RANGE_TYPE_CHAR) {
      int start = mMarkdown.getCharIndexByPoint(startX, startY, Constants.INDEX_TYPE_CHAR);
      int end = mMarkdown.getCharIndexByPoint(endX, endY, Constants.INDEX_TYPE_CHAR);
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
      if (start >= end) {
        callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "start >= end");
        return;
      }
      int indexType = toIndexType(params.getString("indexType", ""));
      content = mMarkdown.getContent(start, end, indexType);
    } else {
      content = mMarkdown.getContent();
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
    mMarkdown.pauseAnimation();
    JavaOnlyMap result = new JavaOnlyMap();
    result.put("animationStep", mMarkdown.getAnimationStep());
    callback.invoke(LynxUIMethodConstants.SUCCESS, result);
  }

  @LynxUIMethod
  public void resumeAnimation(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    int animationStep = params == null ? -1 : params.getInt("animationStep", -1);
    if (animationStep >= 0) {
      mMarkdown.resumeAnimation(animationStep);
    } else {
      mMarkdown.resumeAnimation();
    }
    callback.invoke(LynxUIMethodConstants.SUCCESS);
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
  public void setTextSelection(ReadableMap params, Callback callback) {
    if (!ensureMarkdownReady(callback)) {
      return;
    }
    if (params == null) {
      callback.invoke(LynxUIMethodConstants.PARAM_INVALID, "parameter is invalid");
      return;
    }
    float density = getLynxContext().getScreenMetrics().density;
    float startX =
        (float) (params.getDouble("startX", 0) * density - getPaddingLeft() - getBorderLeftWidth());
    float startY =
        (float) (params.getDouble("startY", 0) * density - getPaddingTop() - getBorderTopWidth());
    float endX =
        (float) (params.getDouble("endX", 0) * density - getPaddingLeft() - getBorderLeftWidth());
    float endY =
        (float) (params.getDouble("endY", 0) * density - getPaddingTop() - getBorderTopWidth());
    int selectionType = toSelectionRangeType(params.getString("selectionTextType", ""));
    int[] range = getSelectionRange(startX, startY, endX, endY, selectionType);
    if (range == null) {
      callback.invoke(LynxUIMethodConstants.UNKNOWN, "Can not set text selection.");
      return;
    }
    mMarkdown.setTextSelection(range[0], range[1]);
    ArrayList<RectF> boxes = mMarkdown.getSelectedLineBoundingRect();
    if (boxes.isEmpty()) {
      callback.invoke(LynxUIMethodConstants.SUCCESS);
      return;
    }
    RectF textRect = LynxUIHelper.getRelativePositionInfo(this, params);
    JavaOnlyMap result = getTextBoundingRectFromBoxes(boxes, textRect);

    long handlePosition = mMarkdown.getSelectionHandlePosition();
    int handleX = MarkdownValuePack.unpackPairFirst(handlePosition);
    int handleY = MarkdownValuePack.unpackPairSecond(handlePosition);
    if (handleX >= 0 && handleY >= 0) {
      JavaOnlyArray handles = new JavaOnlyArray();
      handles.pushMap(
          getHandleMap(handleX, handleY, mMarkdown.getSelectionHandleRadius(), textRect));
      result.putArray("handles", handles);
    }
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
    result.put("id", mMarkdown.getContentID());
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
    map.putDouble("left",
        (safeTextRect.left + getPaddingLeft() + getBorderLeftWidth() + lineBox.left) / density);
    map.putDouble(
        "top", (safeTextRect.top + getPaddingTop() + getBorderTopWidth() + lineBox.top) / density);
    map.putDouble("right",
        (safeTextRect.left + getPaddingLeft() + getBorderLeftWidth() + lineBox.right) / density);
    map.putDouble("bottom",
        (safeTextRect.top + getPaddingTop() + getBorderTopWidth() + lineBox.bottom) / density);
    map.putDouble("width", lineBox.width() / density);
    map.putDouble("height", lineBox.height() / density);
    return map;
  }

  private JavaOnlyMap getHandleMap(float x, float y, float radius, RectF textRect) {
    RectF safeTextRect = textRect == null ? new RectF() : textRect;
    JavaOnlyMap map = new JavaOnlyMap();
    float density = getLynxContext().getScreenMetrics().density;
    map.putDouble("x", (safeTextRect.left + getPaddingLeft() + getBorderLeftWidth() + x) / density);
    map.putDouble("y", (safeTextRect.top + getPaddingTop() + getBorderTopWidth() + y) / density);
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
}
