// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.text;

import static com.lynx.tasm.behavior.ui.accessibility.LynxAccessibilityWrapper.ACCESSIBILITY_ELEMENT_TRUE;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.text.Layout;
import android.text.Spanned;
import android.text.TextUtils;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.event.EventTarget;
import com.lynx.tasm.behavior.shadow.text.TextHelper;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.LynxFlattenUI;
import com.lynx.tasm.utils.UIThreadUtils;

public class FlattenUIText extends LynxFlattenUI implements IUIText {
  private Layout mTextLayout;
  private PointF mTextTranslateOffset;
  private boolean mHasImage;
  private boolean mNeedDrawStroke;
  private boolean mIsJustify;
  private CharSequence mOriginText;
  private TextUpdateBundle mTextBundle;
  private Drawable.Callback mCallback = new DrawableCallback();

  @Deprecated
  public FlattenUIText(Context context) {
    super((LynxContext) context);
  }

  public FlattenUIText(LynxContext context) {
    super(context);
    mAccessibilityElementStatus = ACCESSIBILITY_ELEMENT_TRUE;
    if (mContext.isTextOverflowEnabled()) {
      mOverflow = OVERFLOW_XY;
    }
  }

  public void updateExtraData(Object data) {
    if (data instanceof TextUpdateBundle) {
      setTextBundle((TextUpdateBundle) data);
    }
  }

  @Override
  public void onNodeReady() {
    super.onNodeReady();
    if (mTextBundle != null) {
      UITextUtils.HandleInlineViewTruncated(mTextBundle, this);
    }
  }

  protected TextUpdateBundle getTextBundle() {
    return mTextBundle;
  }

  public void setTextBundle(final TextUpdateBundle bundle) {
    mTextBundle = bundle;
    // First detach old image span
    dispatchDetachImageSpan();
    mTextLayout = bundle.getTextLayout();
    mTextTranslateOffset = bundle.getTextTranslateOffset();
    mHasImage = bundle.hasImages();
    mNeedDrawStroke = bundle.getNeedDrawStroke();
    mIsJustify = bundle.isJustify();
    mOriginText = bundle.getOriginText();
    if (mHasImage && getText() instanceof Spanned) {
      Spanned spannable = (Spanned) getText();
      AbsInlineImageSpan.possiblyUpdateInlineImageSpans(spannable, mCallback);
    }
    invalidate();
  }

  public CharSequence getOriginText() {
    return mOriginText;
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

  @Override
  public void onLayoutUpdated() {
    super.onLayoutUpdated();
    invalidate();
  }

  public CharSequence getText() {
    return mTextLayout != null ? mTextLayout.getText() : null;
  }

  @Deprecated
  public void setTextGradient(String gradient) {
    LLog.e("FlattenUIText", "setTextGradient(String) is deprecated");
  }

  @Deprecated
  public void setTextGradient(ReadableArray gradient) {}

  @Deprecated
  public void setColor(int color) {}

  @Deprecated
  public void setColor(Dynamic color) {}

  @Override
  public void onDraw(final Canvas canvas) {
    TraceEvent.beginSection("text.FlattenUIText.onDraw");
    super.onDraw(canvas);
    if (mTextLayout == null) {
      TraceEvent.endSection("text.FlattenUIText.onDraw");
      return;
    }

    int paddingLeft = mPaddingLeft + mBorderLeftWidth;
    int paddingRight = mPaddingRight + mBorderRightWidth;
    int paddingTop = mPaddingTop + mBorderTopWidth;
    int paddingBottom = mPaddingBottom + mBorderBottomWidth;
    canvas.save();
    if (getOverflow() != 0) {
      Rect clipRect = getBoundRectForOverflow();
      if (clipRect != null) {
        // not null
        canvas.clipRect(clipRect);
      }
    } else if (!mContext.isTextOverflowEnabled()) {
      canvas.clipRect(
          paddingLeft, paddingTop, getWidth() - paddingRight, getHeight() - paddingBottom);
    }
    canvas.translate(paddingLeft + mTextTranslateOffset.x, paddingTop + mTextTranslateOffset.y);

    if (mIsJustify && Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
      TextHelper.drawText(canvas, mTextLayout, getWidth() - paddingLeft - paddingRight);
    } else {
      mTextLayout.draw(canvas);
    }
    if (mNeedDrawStroke) {
      TextHelper.drawTextStroke(mTextLayout, canvas);
    }

    canvas.restore();
    TextHelper.drawLine(canvas, mTextLayout);
    TraceEvent.endSection("text.FlattenUIText.onDraw");
  }

  @Override
  @Nullable
  public CharSequence getAccessibilityLabel() {
    CharSequence s = super.getAccessibilityLabel();
    if (!TextUtils.isEmpty(s)) {
      return s;
    }
    return getText();
  }

  @Nullable
  public Layout getTextLayout() {
    return mTextLayout;
  }

  @Override
  public EventTarget hitTest(float x, float y) {
    return hitTest(x, y, false);
  }

  @Override
  public EventTarget hitTest(float x, float y, boolean ignoreUserInteraction) {
    x -= mPaddingLeft + mBorderLeftWidth;
    y -= mPaddingTop + mBorderTopWidth;

    return UITextUtils.hitTest(this, x, y, this, mTextLayout, UITextUtils.getSpanned(mTextLayout),
        mTextTranslateOffset, ignoreUserInteraction);
  }

  private class DrawableCallback implements Drawable.Callback {
    @Override
    public void invalidateDrawable(@NonNull Drawable who) {
      if (!UIThreadUtils.isOnUiThread()) {
        // TextLayoutWarmer may invalidate on another thread
        return;
      }
      FlattenUIText.this.invalidate();
    }

    @Override
    public void scheduleDrawable(@NonNull Drawable who, @NonNull Runnable what, long when) {
      UIThreadUtils.runOnUiThreadAtTime(what, who, when);
    }

    @Override
    public void unscheduleDrawable(@NonNull Drawable who, @NonNull Runnable what) {
      UIThreadUtils.removeCallbacks(what, who);
    }
  }

  private void release() {
    if (mHasImage && getText() instanceof Spanned) {
      Spanned spannable = (Spanned) getText();
      AbsInlineImageSpan.possiblyUpdateInlineImageSpans(spannable, null);
    }
  }

  @Override
  public void destroy() {
    super.destroy();
    release();
  }
}
