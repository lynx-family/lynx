// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.text;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.view.View;
import androidx.annotation.ColorInt;
import androidx.annotation.Nullable;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.Style;
import com.lynx.tasm.behavior.shadow.text.AbsBaselineShiftCalculatorSpan;
import com.lynx.tasm.behavior.shadow.text.TextAttributes;
import com.lynx.tasm.behavior.ui.utils.LynxBackground;

/**
 * Base class for inline image spans.
 *
 * Default value of vertical align is center
 */
public abstract class AbsInlineImageSpan extends AbsBaselineShiftCalculatorSpan {
  private int mHeight;
  private int mWidth;
  private int mMarginLeft, mMarginTop;
  private int mHorizontalMargin, mVerticalMargin;
  private int mBackgroundColor = 0;
  private float mVerticalShift = 0.f;
  private LynxBackground mComplexBackground;

  private @Nullable Drawable.Callback mCallback;

  public AbsInlineImageSpan(int width, int height, int[] margins) {
    mWidth = width;
    mHeight = height;
    mMarginLeft = margins[Style.EDGE_LEFT];
    mMarginTop = margins[Style.EDGE_TOP];
    mHorizontalMargin = margins[Style.EDGE_LEFT] + margins[Style.EDGE_RIGHT];
    mVerticalMargin = margins[Style.EDGE_TOP] + margins[Style.EDGE_BOTTOM];
    mValign = StyleConstants.VERTICAL_ALIGN_DEFAULT;
    mValignLength = 0.0f;
  }

  public void setBackgroundColor(@ColorInt int color) {
    mBackgroundColor = color;
  }

  public void setComplexBackground(LynxBackground background) {
    mComplexBackground = background;
  }

  public void setVerticalShift(float shift) {
    mVerticalShift = shift;
  }

  @Override
  public void draw(Canvas canvas, CharSequence text, int start, int end, float x, int top, int y,
      int bottom, Paint paint) {
    if (isTruncated(text, start, end)) {
      return;
    }
    if (getDrawable() == null)
      return;

    Drawable b = getDrawable();
    canvas.save();

    Paint.FontMetricsInt fm = paint.getFontMetricsInt();
    int yPos;

    switch (mValign) {
      case StyleConstants.VERTICAL_ALIGN_TOP:
        yPos = top;
        break;
      case StyleConstants.VERTICAL_ALIGN_BOTTOM:
        yPos = bottom - mHeight - mVerticalMargin;
        break;
      case StyleConstants.VERTICAL_ALIGN_CENTER:
        yPos = (bottom + top - mHeight - mVerticalMargin) / 2;
        break;
      default:
        if (mEnableTextRefactor) {
          yPos = y + mCalcAscent;
        } else {
          // middle is same as center when mEnableTextRefactor is false
          if (mValign == StyleConstants.VERTICAL_ALIGN_MIDDLE) {
            yPos = (bottom + top - mHeight - mVerticalMargin) / 2;
          } else {
            yPos = y + caYOffset(fm);
          }
        }
        break;
    }

    if (mBackgroundColor != 0) {
      Rect bounds = new Rect(Math.round(x), Math.round(top + mVerticalShift),
          Math.round(x + b.getBounds().width()), Math.round(bottom + mVerticalShift));
      Paint bg = new Paint();
      bg.setStyle(Paint.Style.FILL);
      bg.setColor(mBackgroundColor);
      canvas.drawRect(bounds, bg);
    }
    canvas.translate(x + mMarginLeft, yPos + mMarginTop + mVerticalShift);
    if (mComplexBackground != null && mComplexBackground.getDrawable() != null) {
      mComplexBackground.getDrawable().draw(canvas);
      if (mComplexBackground.getDrawable().getInnerClipPathForBorderRadius() != null) {
        canvas.clipPath(mComplexBackground.getDrawable().getInnerClipPathForBorderRadius());
      }
    }
    b.draw(canvas);
    canvas.restore();
  }

  @Override
  public int getSize(Paint paint, CharSequence text, int start, int end, Paint.FontMetricsInt fm) {
    if (isTruncated(text, start, end)) {
      return 0;
    }
    if (fm != null) {
      if (fm.descent == fm.ascent) {
        fm.ascent = paint.getFontMetricsInt().ascent;
        fm.descent = paint.getFontMetricsInt().descent;
      }

      if (mEnableTextRefactor) {
        mCalcAscent = (int) calcBaselineShiftAscender(-(mHeight + mVerticalMargin), 0);
      } else {
        mCalcAscent = caYOffset(fm);
      }
      if (fm.ascent > mCalcAscent) {
        fm.ascent = mCalcAscent;
      }
      if (fm.descent < mCalcAscent + mHeight + mVerticalMargin) {
        fm.descent = mCalcAscent + mHeight + mVerticalMargin;
      }
      if (fm.top > fm.ascent) {
        fm.top = fm.ascent;
      }
      if (fm.bottom < fm.descent) {
        fm.bottom = fm.descent;
      }
    }
    return mWidth + mHorizontalMargin;
  }

  private int caYOffset(Paint.FontMetricsInt fm) {
    final int lineHeight = fm.descent - fm.ascent;
    final int height = mHeight + mVerticalMargin;
    int yOffset = 0;
    switch (mValign) {
      case StyleConstants.VERTICAL_ALIGN_BASELINE:
        yOffset = -height;
        break;
      case StyleConstants.VERTICAL_ALIGN_SUPER:
        yOffset = fm.ascent + (int) (lineHeight * 0.1f);
        break;
      case StyleConstants.VERTICAL_ALIGN_TOP:
      case StyleConstants.VERTICAL_ALIGN_TEXT_TOP:
        yOffset = fm.ascent;
        break;
      case StyleConstants.VERTICAL_ALIGN_SUB:
        yOffset = fm.descent - height - (int) (lineHeight * 0.1f);
        break;
      case StyleConstants.VERTICAL_ALIGN_BOTTOM:
      case StyleConstants.VERTICAL_ALIGN_TEXT_BOTTOM:
        yOffset = fm.descent - height;
        break;
      case StyleConstants.VERTICAL_ALIGN_PERCENT:
        yOffset = -height - (int) (mValignLength * lineHeight / 100.f);
        break;
      case StyleConstants.VERTICAL_ALIGN_LENGTH:
        yOffset = -height - (int) mValignLength;
        break;
      case StyleConstants.VERTICAL_ALIGN_MIDDLE:
      default:
        yOffset = fm.ascent + (lineHeight - height) / 2;
        break;
    }
    return yOffset;
  }

  @Override
  protected int getIncludeMarginHeight() {
    return mHeight + mVerticalMargin;
  }

  /**
   * For TextInlineImageSpan we need to update the Span to know that the window is attached and
   * the TextView that we will set as the callback on the Drawable.
   *
   * @param spannable The spannable that may contain TextInlineImageSpans
   * @param callback the callback for the Drawable
   */
  public static void possiblyUpdateInlineImageSpans(Spanned spannable, Drawable.Callback callback) {
    AbsInlineImageSpan[] spans =
        spannable.getSpans(0, spannable.length(), AbsInlineImageSpan.class);
    for (AbsInlineImageSpan span : spans) {
      span.onAttachedToWindow();
      span.setCallback(callback);
    }
  }

  /**
   * Get the drawable that is span represents.
   */
  public abstract @Nullable Drawable getDrawable();

  /**
   * Called by the text view from {@link View#onDetachedFromWindow()},
   */
  public abstract void onDetachedFromWindow();

  /**
   * Called by the text view from {@link View#onStartTemporaryDetach()}.
   */
  public abstract void onStartTemporaryDetach();

  /**
   * Called by the text view from {@link View#onAttachedToWindow()}.
   */
  public abstract void onAttachedToWindow();

  /**
   * Called by the text view from {@link View#onFinishTemporaryDetach()}.
   */
  public abstract void onFinishTemporaryDetach();

  /**
   * Set the textview that will contain this span.
   */
  public void setCallback(Drawable.Callback callback) {
    mCallback = callback;
  }

  public Drawable.Callback getCallback() {
    return mCallback;
  }

  /**
   * Get the width of the span.
   */
  public int getWidth() {
    return mWidth;
  }

  /**
   * Get the height of the span.
   */
  public int getHeight() {
    return mHeight;
  }

  private boolean isTruncated(CharSequence text, int start, int end) {
    if (text instanceof SpannableStringBuilder) {
      return false;
    }

    // Since Layout.Ellipsizer and Layout.SpannedEllipsizer is private class
    // check if the start char is same as InlineImageSpan replaced characters
    return text.charAt(start) != TextAttributes.INLINE_IMAGE_PLACEHOLDER.charAt(0);
  }
}
