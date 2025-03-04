// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.text;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.res.ColorStateList;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Picture;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.RemoteException;
import android.text.Layout;
import android.text.Selection;
import android.text.Spannable;
import android.text.Spanned;
import android.view.ActionMode;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.ViewConfiguration;
import androidx.annotation.ColorInt;
import androidx.annotation.Keep;
import androidx.annotation.Nullable;
import com.lynx.R;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TextHelper;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.view.AndroidView;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.service.ILynxSystemInvokeService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.utils.UIThreadUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;

@Keep
public class AndroidText extends AndroidView implements ActionMode.Callback {
  // integer id for action-menu-item copy
  private static final int ID_COPY = 0xFFFE;
  // integer id for action-menu-item select-all
  private static final int ID_SELECT_ALL = 0xFFFD;
  protected static final String SELECTION_CHANGE_EVENT = "selectionchange";
  private static final float RESPONSE_TOUCH_RADIUS = 50.f;
  private static final int DEFAULT_TEXT_SELECTION_COLOR = 0x6633B5E5;
  private static final int DEFAULT_TEXT_HANDLE_COLOR = 0xFF027AFB;
  private static final int DEFAULT_TEXT_HANDLE_SIZE = 15;

  protected Layout mTextLayout;
  protected TextUpdateBundle mTextUpdateBundle;
  protected PointF mTextTranslateOffset;
  protected boolean mHasImage;
  protected boolean mIsJustify;
  private Picture mOverflowPicture;
  private int mOverflow;
  private boolean mOverflowPictureDirty;
  private CharSequence mOriginText;
  private boolean mNeedDrawStroke = false;

  private boolean mIsBindSelectionChange = false;
  private int mSign = 0;
  // select direction
  private boolean mIsForward = true;
  private boolean mEnableTextSelection = false;
  private boolean mEnableCustomContextMenu = false;
  private boolean mEnableCustomTextSelection = false;
  private Path mHighlightPath;
  private Paint mHighlightPaint;
  private int mTextSelectionColor;
  private int mTextSelectionHandleColor;
  private int mHandleSize;
  private int mDefaultHandlePlatformLength;
  private Drawable mSelectionLeftCursor;
  private Drawable mSelectionRightCursor;
  private ActionMode mActionMode = null;
  private int mSelectStart = -1;
  private int mSelectEnd = -1;
  private int mLastSelectStart = -1;
  private int mLastSelectEnd = -1;
  private final PointF mSelectStartPos = new PointF(-1.f, -1.f);
  private final PointF mSelectEndPos = new PointF(-1.f, -1.f);
  private final PointF mStartHandlerPos = new PointF(-1.f, -1.f);
  private final PointF mEndHandlerPos = new PointF(-1.f, -1.f);
  private boolean mIsInSelection = false;
  private boolean mIsAdjustStartPos = false;
  private boolean mIsAdjustEndPos = false;
  private CheckForLongPress mCheckForLongPress = null;
  private boolean mShouldResponseMove = false;

  // save weak reference of selecting AndroidText to ensure that only one AndroidText is selected at
  // a time.
  private static WeakReference<AndroidText> sWeakSelectingAndroidText;

  private final class CheckForLongPress implements Runnable {
    private final float mX;
    private final float mY;

    public CheckForLongPress(float x, float y) {
      mX = x;
      mY = y;
    }

    @Override
    public void run() {
      mIsInSelection = true;
      mSelectStartPos.set(mX, mY);
      mSelectEndPos.set(mX, mY);
      mSelectEnd = mSelectStart = getOffsetForPosition(mX, mY);
      mIsAdjustEndPos = true;
      requestDisallowInterceptTouchEvent(true);

      // clear self
      mCheckForLongPress = null;
    }
  }

  public AndroidText(Context context) {
    super(context);
    mOverflowPicture = new Picture();
    mOverflow = 0x00;
    mOverflowPictureDirty = true;
    setFocusable(true);
    // can draw
    setWillNotDraw(false);
    mTextSelectionColor = DEFAULT_TEXT_SELECTION_COLOR;
    mTextSelectionHandleColor = DEFAULT_TEXT_HANDLE_COLOR;
    mHandleSize = mDefaultHandlePlatformLength =
        Math.round(((LynxContext) context).getScreenMetrics().density * DEFAULT_TEXT_HANDLE_SIZE);
  }

  public void setTextBundle(TextUpdateBundle bundle) {
    // First detach old image span
    dispatchDetachImageSpan();
    mTextUpdateBundle = bundle;
    mTextLayout = generateTextLayout(bundle);
    mTextTranslateOffset = bundle.getTextTranslateOffset();
    mHasImage = bundle.hasImages();
    mNeedDrawStroke = bundle.getNeedDrawStroke();
    mIsJustify = bundle.isJustify();
    mOriginText = bundle.getOriginText();
    if (mHasImage && getText() instanceof Spanned) {
      Spanned spannable = (Spanned) getText();
      AbsInlineImageSpan.possiblyUpdateInlineImageSpans(spannable, this);
    }
    if (mIsInSelection) {
      clearSelection();
    } else {
      resetSelectionState();
    }
    // Enable layout inspector to collect content
    setContentDescription(mTextLayout.getText());
    invalidate();
    mOverflowPictureDirty = true;
  }

  private void resetSelectionState() {
    mSelectStart = mSelectEnd = mLastSelectStart = mLastSelectEnd = -1;
    mIsInSelection = mIsAdjustStartPos = mIsAdjustEndPos = mShouldResponseMove = false;
    mSelectStartPos.set(-1.f, -1.f);
    mSelectEndPos.set(-1.f, -1.f);
  }

  public CharSequence getOriginText() {
    return mOriginText;
  }

  protected Layout generateTextLayout(TextUpdateBundle bundle) {
    return bundle.getTextLayout();
  }

  @Deprecated
  public void setTextGradient(String gradient) {
    LLog.e(
        "text-gradient", "setTextGradient(String) is deprecated, call this function has no effect");
  }

  public void setTextGradient(ReadableArray gradient) {}

  public void setEnableTextSelection(boolean enable) {
    mEnableTextSelection = enable;
  }

  public void updateSelectionBackgroundColor(int color) {
    mTextSelectionColor = color == Color.TRANSPARENT ? DEFAULT_TEXT_SELECTION_COLOR : color;
  }

  public void updateSelectionHandleColor(int color) {
    mTextSelectionHandleColor = color == Color.TRANSPARENT ? DEFAULT_TEXT_HANDLE_COLOR : color;
  }

  public void updateSelectionHandleSize(int size) {
    mHandleSize = size <= 0 ? mDefaultHandlePlatformLength : size;
  }

  @Keep
  @Override
  protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    setMeasuredDimension(
        MeasureSpec.getSize(widthMeasureSpec), MeasureSpec.getSize(heightMeasureSpec));
  }

  @Keep
  @Override
  protected void onLayout(boolean changed, int l, int t, int r, int b) {}

  @Override
  public boolean onTouchEvent(MotionEvent event) {
    if (mTextLayout == null || !mEnableTextSelection || mEnableCustomTextSelection) {
      return super.onTouchEvent(event);
    }
    float x = event.getX() - getPaddingLeft();
    float y = event.getY() - getPaddingTop();

    if (event.getAction() == MotionEvent.ACTION_DOWN) {
      performBeginSelection(x, y);
    } else if (event.getAction() == MotionEvent.ACTION_MOVE) {
      performMovingSelection(x, y);
    } else if (event.getAction() == MotionEvent.ACTION_UP) {
      performEndSelection(x, y);
    } else {
      if (mShouldResponseMove) {
        performEndSelection(x, y);
      } else {
        // The long press callback needs to be removed when the finger leaves the screen.
        removeCheckLongPressCallback();
      }
    }

    invalidate();
    return true;
  }

  @Keep
  @Override
  protected void onDraw(Canvas canvas) {
    if (mTextLayout != null) {
      canvas.save();
      // since TextRender only build StaticLayout once
      // UIText needs to make translate to make sure content display in correct place
      // If layout not align left, needs to do offset
      canvas.translate(
          getPaddingLeft() + mTextTranslateOffset.x, getPaddingTop() + mTextTranslateOffset.y);

      if (mOverflow != 0) {
        drawOverflowPicture();
        canvas.drawPicture(mOverflowPicture);
      } else {
        drawText(canvas);
      }
      canvas.restore();
    }
  }

  @Override
  public void dispatchDraw(Canvas canvas) {
    super.dispatchDraw(canvas);

    if (!mIsInSelection || mHighlightPath == null || mHighlightPath.isEmpty()) {
      return;
    }
    canvas.save();
    canvas.translate(
        getPaddingLeft() + mTextTranslateOffset.x, getPaddingTop() + mTextTranslateOffset.y);
    drawSelectHandle(canvas);
    canvas.restore();
  }

  private void drawTextOnCanvas(Canvas canvas) {
    if (mIsJustify && Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
      TextHelper.drawText(canvas, mTextLayout, getWidth() - getPaddingLeft() - getPaddingRight());
    } else {
      mTextLayout.draw(canvas);
    }
  }

  private void drawHighlight(Canvas canvas) {
    if (!mIsInSelection || mHighlightPath == null || mHighlightPaint == null) {
      return;
    }
    mHighlightPath.reset();
    int selectStart = Math.min(mSelectStart, mSelectEnd);
    int selectEnd = Math.max(mSelectStart, mSelectEnd);
    mTextLayout.getSelectionPath(selectStart, selectEnd, mHighlightPath);
    if (!mHighlightPath.isEmpty()) {
      // Using draw (Canvas canvas, Path highlight, Paint highlightPaint,int cursorOffsetVertical)
      // method does not draw highlighting on some Android phones.
      canvas.drawPath(mHighlightPath, mHighlightPaint);
    }
  }

  private void drawText(Canvas canvas) {
    drawHighlight(canvas);
    drawTextOnCanvas(canvas);
    if (mNeedDrawStroke) {
      TextHelper.drawTextStroke(mTextLayout, canvas);
    }
    TextHelper.drawLine(canvas, mTextLayout);
  }

  /**
   * Get bounding box of the specified range of text.
   * @param start start index of text
   * @param end end index of text
   * @return
   */
  public ArrayList<RectF> getTextBoundingBoxes(int start, int end) {
    ArrayList<RectF> boxes = new ArrayList<>();
    if (mTextLayout == null || mTextLayout.getText().length() < end || start > end || start < 0) {
      return boxes;
    }

    if (start == end) {
      int lineIndex = mTextLayout.getLineForOffset(start);
      int lineHeight = mTextLayout.getLineBottom(lineIndex) - mTextLayout.getLineTop(lineIndex);
      boxes.add(new RectF(0, 0, 0, lineHeight));
    } else {
      int startLineIndex = mTextLayout.getLineForOffset(start);
      int endLineIndex = mTextLayout.getLineForOffset(end);
      for (int lineIndex = startLineIndex; lineIndex <= endLineIndex; lineIndex++) {
        Rect lineRect = new Rect();
        mTextLayout.getLineBounds(lineIndex, lineRect);
        if (lineIndex == startLineIndex || lineIndex == endLineIndex) {
          lineRect.left = (int) Math.max(lineRect.left, mTextLayout.getSecondaryHorizontal(start));
          lineRect.right = (int) Math.min(lineRect.right, mTextLayout.getSecondaryHorizontal(end));
        }
        lineRect.left -= mTextTranslateOffset.x;
        lineRect.right -= mTextTranslateOffset.x;
        lineRect.top -= mTextTranslateOffset.y;
        lineRect.bottom -= mTextTranslateOffset.y;
        boxes.add(new RectF(lineRect));
      }
    }

    return boxes;
  }

  public void setBindSelectionChange(boolean isBindSelectionChange, int sign) {
    mIsBindSelectionChange = isBindSelectionChange;
    mSign = sign;
  }

  private void onSelectionChange() {
    if (mIsBindSelectionChange && getContext() instanceof LynxContext) {
      LynxDetailEvent event = new LynxDetailEvent(mSign, SELECTION_CHANGE_EVENT);
      event.addDetail("start", mSelectStart);
      event.addDetail("end", mSelectEnd);
      event.addDetail("direction", mIsForward ? "forward" : "backward");
      ((LynxContext) getContext()).getEventEmitter().sendCustomEvent(event);
    }
  }

  public void setCustomContextMenu(boolean enable) {
    mEnableCustomContextMenu = enable;
  }

  public void setCustomTextSelection(boolean enable) {
    mEnableCustomTextSelection = enable;
  }

  /**
   * Set text selection.
   * @param startX The x-coordinate of the start of the selected text relative to the text component
   * @param startY The y-coordinate of the start of the selected text relative to the text component
   * @param endX The x-coordinate of the end of the selected text relative to the text component
   * @param endY The y-coordinate of the end of the selected text relative to the text component
   * @return The bounding boxes of each line
   */
  public ArrayList<RectF> setTextSelection(float startX, float startY, float endX, float endY) {
    invalidate();
    if (startX < 0 || startY < 0 || endX < 0 || endY < 0) {
      clearSelection();
      return new ArrayList<>();
    }
    int startIndex = getOffsetForPosition(startX, startY);
    int endIndex = getOffsetForPosition(endX, endY);
    if (startIndex < 0 || endIndex < 0) {
      clearSelection();
      return new ArrayList<>();
    }
    if (startIndex == endIndex) {
      PointF point = getCenterPositionForOffset(startIndex);
      if (startIndex == mTextLayout.getText().length() || (startIndex > 0 && startX < point.x)) {
        startIndex--;
      } else {
        endIndex++;
      }
    }

    mIsInSelection = true;

    updateSelectionRange(startIndex, endIndex);
    updateSelectStartEnd();
    return getTextBoundingBoxes(mSelectStart, mSelectEnd);
  }

  /**
   * Get position and default response click radius of handles.
   * @return Handles list
   */
  public ArrayList<Float>[] getHandlesInfo() {
    if (!mIsInSelection) {
      return new ArrayList[0];
    }
    ArrayList<Float>[] handlesInfo = new ArrayList[2];
    ArrayList<Float> startHandle = new ArrayList<>();
    startHandle.add(mStartHandlerPos.x);
    startHandle.add(mStartHandlerPos.y);
    startHandle.add(RESPONSE_TOUCH_RADIUS);
    handlesInfo[0] = startHandle;
    ArrayList<Float> endHandle = new ArrayList<>();
    endHandle.add(mEndHandlerPos.x);
    endHandle.add(mEndHandlerPos.y);
    endHandle.add(RESPONSE_TOUCH_RADIUS);
    handlesInfo[1] = endHandle;
    return handlesInfo;
  }

  /**
   * Get selected text content.
   * @return Selected string
   */
  public String getSelectedText() {
    if (mSelectStart >= 0 && mSelectEnd > 0 && mSelectEnd > mSelectStart
        && mSelectEnd <= mTextLayout.getText().length()) {
      return mTextLayout.getText().subSequence(mSelectStart, mSelectEnd).toString();
    }
    return "";
  }

  private void drawSelectHandle(Canvas canvas) {
    drawSelectStartCursor(canvas);
    drawSelectEndCursor(canvas);
  }

  private void drawSelectStartCursor(Canvas canvas) {
    canvas.save();
    canvas.translate(mStartHandlerPos.x - mSelectionLeftCursor.getBounds().width() / 2.f,
        mStartHandlerPos.y - mSelectionLeftCursor.getBounds().height() / 2.f);
    mSelectionLeftCursor.draw(canvas);
    canvas.restore();
  }

  private void drawSelectEndCursor(Canvas canvas) {
    canvas.save();
    canvas.translate(mEndHandlerPos.x - mSelectionRightCursor.getBounds().width() / 2.f,
        mEndHandlerPos.y - mSelectionRightCursor.getBounds().height() / 2.f);
    mSelectionRightCursor.draw(canvas);
    canvas.restore();
  }

  @Keep
  @Override
  protected boolean verifyDrawable(Drawable drawable) {
    if (mHasImage && getText() instanceof Spanned) {
      Spanned text = (Spanned) getText();
      AbsInlineImageSpan[] spans = text.getSpans(0, text.length(), AbsInlineImageSpan.class);
      for (AbsInlineImageSpan span : spans) {
        if (span.getDrawable() == drawable) {
          return true;
        }
      }
    }
    return super.verifyDrawable(drawable);
  }

  @Keep
  @Override
  public void invalidateDrawable(Drawable drawable) {
    if (!UIThreadUtils.isOnUiThread()) {
      // TextLayoutWarmer may invalidate AndroidText
      return;
    }
    if (mHasImage && getText() instanceof Spanned) {
      Spanned text = (Spanned) getText();
      AbsInlineImageSpan[] spans = text.getSpans(0, text.length(), AbsInlineImageSpan.class);
      for (AbsInlineImageSpan span : spans) {
        if (span.getDrawable() == drawable) {
          invalidate();
          mOverflowPictureDirty = true;
        }
      }
    }
    super.invalidateDrawable(drawable);
  }

  @Keep
  @Override
  public void onDetachedFromWindow() {
    super.onDetachedFromWindow();
    dispatchDetachImageSpan();
  }

  private void dispatchDetachImageSpan() {
    if (mHasImage && getText() instanceof Spanned) {
      Spanned text = (Spanned) getText();
      AbsInlineImageSpan[] spans = text.getSpans(0, text.length(), AbsInlineImageSpan.class);
      for (AbsInlineImageSpan span : spans) {
        span.onDetachedFromWindow();
        span.setCallback(null);
      }
    }
  }

  @Keep
  @Override
  public void onStartTemporaryDetach() {
    super.onStartTemporaryDetach();
    if (mHasImage && getText() instanceof Spanned) {
      Spanned text = (Spanned) getText();
      AbsInlineImageSpan[] spans = text.getSpans(0, text.length(), AbsInlineImageSpan.class);
      for (AbsInlineImageSpan span : spans) {
        span.onStartTemporaryDetach();
      }
    }
  }

  @Keep
  @Override
  public void onAttachedToWindow() {
    super.onAttachedToWindow();
    if (mHasImage && getText() instanceof Spanned) {
      Spanned spannable = (Spanned) getText();
      AbsInlineImageSpan.possiblyUpdateInlineImageSpans(spannable, this);
    }
  }

  @Keep
  @Override
  public void onFinishTemporaryDetach() {
    super.onFinishTemporaryDetach();
    if (mHasImage && getText() instanceof Spanned) {
      Spanned text = (Spanned) getText();
      AbsInlineImageSpan[] spans = text.getSpans(0, text.length(), AbsInlineImageSpan.class);
      for (AbsInlineImageSpan span : spans) {
        span.onFinishTemporaryDetach();
      }
    }
  }

  public CharSequence getText() {
    return mTextLayout != null ? mTextLayout.getText() : null;
  }

  @Nullable
  public Layout getTextLayout() {
    return mTextLayout;
  }

  public void setOverflow(int overflow) {
    mOverflow = overflow;
  }

  private void drawOverflowPicture() {
    if (!mOverflowPictureDirty || mTextLayout == null) {
      return;
    }
    if (mOverflowPicture == null) {
      mOverflowPicture = new Picture();
    } else {
      // There are a few picture already recording exceptions, temporary fallback processing.
      mOverflowPicture.endRecording();
    }
    Canvas canvas =
        mOverflowPicture.beginRecording(mTextLayout.getWidth(), mTextLayout.getHeight());
    canvas.save();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
      if (getClipBounds() != null) {
        canvas.clipRect(getClipBounds());
      }
    }
    drawTextOnCanvas(canvas);
    if (mNeedDrawStroke) {
      TextHelper.drawTextStroke(mTextLayout, canvas);
    }
    canvas.restore();
    mOverflowPicture.endRecording();

    mOverflowPictureDirty = false;
  }

  public void release() {
    if (mHasImage && getText() instanceof Spanned) {
      Spanned spannable = (Spanned) getText();
      AbsInlineImageSpan.possiblyUpdateInlineImageSpans(spannable, null);
    }
  }

  private void initSelectionCursor(Context context) {
    mSelectionLeftCursor =
        context.getResources().getDrawable(R.drawable.lynx_text_select_handle_left_material);
    mSelectionRightCursor =
        context.getResources().getDrawable(R.drawable.lynx_text_select_handle_right_material);
    mHighlightPaint = new Paint();
    mHighlightPaint.setStyle(Paint.Style.FILL);
    mHighlightPath = new Path();
  }

  private void updateSelectionStyle() {
    mSelectionLeftCursor.setBounds(0, 0, mHandleSize, mHandleSize);
    mSelectionRightCursor.setBounds(0, 0, mHandleSize, mHandleSize);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      mSelectionLeftCursor.setTint(mTextSelectionHandleColor);
      mSelectionRightCursor.setTint(mTextSelectionHandleColor);
    }

    mHighlightPaint.setColor(mTextSelectionColor);
  }

  private int getLineAtCoordinate(float y) {
    y = Math.max(0.f, y);
    y = Math.min(getHeight() - 1, y);

    return mTextLayout.getLineForVertical((int) y);
  }

  private int getOffsetAtCoordinate(int line, float x) {
    float originX = x;
    x = Math.max(0.f, x);
    x = Math.min(getWidth() - 1, x);

    int offset = mTextLayout.getOffsetForHorizontal(line, x);
    float offsetX = mTextLayout.getSecondaryHorizontal(offset);
    if (originX > offsetX + (mTextLayout.getLineRight(line) - offsetX) / 2.0) {
      // contain last char
      offset = mTextLayout.getLineEnd(line);
    }

    return offset;
  }

  private int getOffsetForPosition(float x, float y) {
    if (mTextLayout == null) {
      return -1;
    }

    int line = getLineAtCoordinate(y);

    return getOffsetAtCoordinate(line, x);
  }

  private PointF getBottomPositionForOffset(int offset, boolean isStart) {
    float x = mTextLayout.getPrimaryHorizontal(offset);
    int line = mTextLayout.getLineForOffset(offset);
    float y = mTextLayout.getLineBottom(line);
    // If end cursor is at the start of line, move to the end of last line.
    if (offset == mTextLayout.getLineStart(line) && line > 0 && !isStart) {
      x = mTextLayout.getWidth();
      y = mTextLayout.getLineBottom(line - 1);
    }

    return new PointF(x, y);
  }

  private void showToolbar() {
    if (mEnableCustomContextMenu) {
      return;
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      startActionMode(this, ActionMode.TYPE_FLOATING);
    } else {
      startActionMode(this);
    }
  }

  private void hideToolbar() {
    if (mEnableCustomContextMenu || mActionMode == null) {
      return;
    }

    mActionMode.finish();

    mActionMode = null;
  }

  private void updateSelectionRange(int selectStart, int selectEnd) {
    // init cursor drawable if needed
    if (mSelectionLeftCursor == null) {
      initSelectionCursor(getContext());
    }
    updateSelectionStyle();

    mIsForward = mLastSelectStart == -1
        ? selectEnd > selectStart
        : (mLastSelectStart < selectStart || mLastSelectEnd < selectEnd);

    mLastSelectStart = mSelectStart;
    mLastSelectEnd = mSelectEnd;
    mSelectStart = selectStart;
    mSelectEnd = selectEnd;

    if (mSelectStart >= 0 && mSelectStart <= mTextLayout.getText().length() && mSelectEnd >= 0
        && mSelectEnd <= mTextLayout.getText().length()) {
      Selection.setSelection((Spannable) mTextLayout.getText(), Math.min(mSelectStart, mSelectEnd),
          Math.max(mSelectStart, mSelectEnd));
      mSelectStartPos.set(getBottomPositionForOffset(Math.min(mSelectStart, mSelectEnd), true));
      mSelectEndPos.set(getBottomPositionForOffset(Math.max(mSelectStart, mSelectEnd), false));
      mStartHandlerPos.set(mSelectStartPos.x - mSelectionLeftCursor.getBounds().width() / 2.f,
          mSelectStartPos.y + mSelectionLeftCursor.getBounds().height() / 2.f);
      mEndHandlerPos.set(mSelectEndPos.x + mSelectionRightCursor.getBounds().width() / 2.f,
          mSelectEndPos.y + mSelectionRightCursor.getBounds().height() / 2.f);
      clearOtherSelection();
    } else {
      Selection.removeSelection((Spannable) mTextLayout.getText());
    }
  }

  /**
   * Clear other AndroidText's selection if this is selected.
   */
  private void clearOtherSelection() {
    if (sWeakSelectingAndroidText != null) {
      AndroidText selectingText = sWeakSelectingAndroidText.get();
      if (selectingText != null && selectingText != this) {
        selectingText.clearSelection();
        selectingText.invalidate();
      }
    }
    sWeakSelectingAndroidText = new WeakReference<>(this);
  }

  /**
   * Exchange select start index and end index if need.
   */
  private void updateSelectStartEnd() {
    int minIndex = Math.min(mSelectStart, mSelectEnd);
    mSelectEnd = Math.max(mSelectStart, mSelectEnd);
    mSelectStart = minIndex;
    onSelectionChange();

    mSelectStartPos.set(getBottomPositionForOffset(mSelectStart, true));
    mSelectEndPos.set(getBottomPositionForOffset(mSelectEnd, false));
  }

  private void performBeginSelection(float x, float y) {
    hideToolbar();

    if (mIsInSelection) {
      mShouldResponseMove = true;
      if (distanceBetweenPoints(mStartHandlerPos, x, y) < RESPONSE_TOUCH_RADIUS) {
        adjustStartPosition(x, y);
        requestDisallowInterceptTouchEvent(true);
      } else if (distanceBetweenPoints(mEndHandlerPos, x, y) < RESPONSE_TOUCH_RADIUS) {
        adjustEndPosition(x, y);
        requestDisallowInterceptTouchEvent(true);
      } else {
        mShouldResponseMove = false;
      }
    }
    if (!mIsAdjustEndPos && !mIsAdjustStartPos) {
      removeCheckLongPressCallback();
      mCheckForLongPress = new CheckForLongPress(x, y);
      postDelayed(mCheckForLongPress, ViewConfiguration.getLongPressTimeout());
    }
  }

  private void adjustStartPosition(float x, float y) {
    mIsAdjustStartPos = true;
    int selectStart = getOffsetForPosition(x, y);

    if (mSelectEnd == selectStart) {
      if (selectStart == mTextLayout.getText().length()
          || (x < mSelectEndPos.x && selectStart > 0)) {
        selectStart--;
      } else {
        selectStart++;
      }
    }

    updateSelectionRange(selectStart, mSelectEnd);
  }

  private void adjustEndPosition(float x, float y) {
    mIsAdjustEndPos = true;
    int selectEnd = getOffsetForPosition(x, y);

    if (selectEnd == mSelectStart) {
      if (selectEnd == mTextLayout.getText().length() || (x < mSelectStartPos.x && selectEnd > 0)) {
        selectEnd--;
      } else {
        selectEnd++;
      }
    }

    updateSelectionRange(mSelectStart, selectEnd);
  }

  private PointF getCenterPositionForOffset(int index) {
    if (index < 0 || index > mTextLayout.getText().length()) {
      return new PointF(0, 0);
    }
    int lineIndex = mTextLayout.getLineForOffset(index);
    return new PointF(mTextLayout.getPrimaryHorizontal(index),
        (mTextLayout.getLineTop(lineIndex) + mTextLayout.getLineBottom(lineIndex)) / 2.f);
  }

  private void performMovingSelection(float x, float y) {
    if (mCheckForLongPress != null) {
      if (Math.abs(x - mCheckForLongPress.mX) > 1.f || Math.abs(y - mCheckForLongPress.mY) > 1.f) {
        // touch move before long-press timeout
        removeCheckLongPressCallback();
      }
    }

    if (mIsAdjustStartPos) {
      adjustStartPosition(x, y);
    } else if (mIsAdjustEndPos) {
      adjustEndPosition(x, y);
    }
  }

  private void performEndSelection(float x, float y) {
    requestDisallowInterceptTouchEvent(false);
    if (!mIsInSelection) {
      // touch move before long-press timeout
      removeCheckLongPressCallback();
      return;
    } else {
      if (!mIsAdjustEndPos && !mIsAdjustStartPos) {
        clearSelection();
        return;
      }
    }

    if (mIsAdjustStartPos) {
      adjustStartPosition(x, y);
      updateSelectStartEnd();
    } else if (mIsAdjustEndPos) {
      adjustEndPosition(x, y);
      updateSelectStartEnd();
    }

    showToolbar();
    mIsAdjustEndPos = mIsAdjustStartPos = false;
  }

  /**
   * Clear selection and reset status.
   */
  private void clearSelection() {
    removeCheckLongPressCallback();

    mIsAdjustStartPos = false;
    mIsAdjustEndPos = false;
    mSelectStartPos.set(-1.f, -1.f);
    mSelectEndPos.set(-1.f, -1.f);
    mSelectStart = -1;
    mSelectEnd = -1;
    mLastSelectStart = -1;
    mLastSelectEnd = -1;
    if (mIsInSelection) {
      onSelectionChange();
    }
    mIsInSelection = false;
    updateSelectionRange(mSelectStart, mSelectEnd);
    hideToolbar();
    mShouldResponseMove = false;
    if (mHighlightPath != null) {
      mHighlightPath.reset();
    }
  }

  private double distanceBetweenPoints(PointF point, float x, float y) {
    return Math.sqrt(Math.pow(point.x - x, 2) + Math.pow(point.y - y, 2));
  }

  private void removeCheckLongPressCallback() {
    if (mCheckForLongPress == null) {
      return;
    }

    removeCallbacks(mCheckForLongPress);
    mCheckForLongPress = null;
  }

  private void performCopy() {
    int selectStart = Selection.getSelectionStart(mTextLayout.getText());
    int selectEnd = Selection.getSelectionEnd(mTextLayout.getText());

    if (selectStart < selectEnd) {
      ClipData clipped = ClipData.newPlainText(
          "Lynx-clipboard", mTextLayout.getText().subSequence(selectStart, selectEnd));

      ILynxSystemInvokeService systemInvokeService =
          LynxServiceCenter.inst().getService(ILynxSystemInvokeService.class);
      if (systemInvokeService != null) {
        try {
          systemInvokeService.setPrimaryClip(clipped);
        } catch (RemoteException e) {
          LLog.e("AndroidText",
              "A RemoteException was encountered while calling systemInvokeService. "
                  + e.getMessage());
        }
      } else {
        ClipboardManager clipManager;
        if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
          clipManager = (ClipboardManager) getContext().getSystemService(ClipboardManager.class);
        } else {
          clipManager = (ClipboardManager) getContext().getSystemService(Context.CLIPBOARD_SERVICE);
        }

        clipManager.setPrimaryClip(clipped);
      }
    }

    clearSelection();
  }

  private void performSelectAll() {
    updateSelectionRange(0, mTextLayout.getText().length());
    updateSelectStartEnd();
  }

  @Override
  public boolean onCreateActionMode(ActionMode mode, Menu menu) {
    menu.add(Menu.NONE, ID_COPY, 0, R.string.copy);
    menu.add(Menu.NONE, ID_SELECT_ALL, 1, R.string.selectAll);

    return true;
  }

  @Override
  public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
    mActionMode = mode;
    return false;
  }

  @Override
  public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
    if (item.getItemId() == ID_COPY) {
      performCopy();
    } else if (item.getItemId() == ID_SELECT_ALL) {
      performSelectAll();
    }
    invalidate();
    return true;
  }

  @Override
  public void onDestroyActionMode(ActionMode mode) {}
}
