// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static com.lynx.tasm.behavior.StyleConstants.DIRECTION_NORMAL;
import static com.lynx.tasm.behavior.StyleConstants.DIRECTION_RTL;
import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_LEFT;
import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_RIGHT;
import static com.lynx.tasm.behavior.StyleConstants.TEXTOVERFLOW_CLIP;
import static com.lynx.tasm.behavior.StyleConstants.TEXTOVERFLOW_ELLIPSIS;
import static com.lynx.tasm.behavior.StyleConstants.WHITESPACE_NOWRAP;
import static com.lynx.tasm.behavior.shadow.text.TextAttributes.FIRST_CHAR_RTL_STATE_NONE_CHECK;
import static com.lynx.tasm.behavior.shadow.text.TextAttributes.FIRST_CHAR_RTL_STATE_RTL;
import static com.lynx.tasm.behavior.shadow.text.TextAttributes.NOT_SET;

import android.graphics.PointF;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.text.LineBreaker;
import android.os.Build;
import android.text.BoringLayout;
import android.text.Layout;
import android.text.Spannable;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.StaticLayout;
import android.text.TextDirectionHeuristics;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.style.StyleSpan;
import androidx.annotation.RequiresApi;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.ui.text.AbsInlineImageSpan;
import java.text.Bidi;

public class TextRenderer {
  private static final int MODE_NONE = -1;
  private static final int MODE_LINES = 0;
  private static final int MODE_PIXELS = 1;

  private static final String ELLIPSIS = "\u2026";
  // Strong direction unicodes to control the direction where ellipsis will be attached.
  private static final String LTR_MARK = "\u200E";
  private static final String RTL_MARK = "\u200F";
  private static final float TEXT_LAYOUT_MAX_WIDTH = Short.MAX_VALUE;
  private static final float SPACING_MULT = 1.f;

  private static final BoringLayout.Metrics UNKNOWN_BORING = new BoringLayout.Metrics();

  private Layout mTextLayout;
  final TextRendererKey mKey;
  boolean mHasEllipsis;
  // for bindlayout
  private int mEllipsisCount = 0;
  private Typeface mTypeface;

  /**
   *  canvas translate offset for text layout.
   *  for example, text width constraint is 100 at-most, text-align is right, the text layout result
   *  is as follow. The text actual width is 60, so text translate offset on x axis is -40.
   *  ***********
   *  *    align*
   *  *    right*
   *  ***********
   */
  private float mTextTranslateOffset = 0.f;
  private float mTextTranslateTopOffset = 0.f;

  // Used to cache to calculate the maximum width of textLayout, reducing the text line measure
  private float mCacheMaxWidth = -1.f;

  TextRenderer(LynxContext context, TextRendererKey key) {
    this.mKey = key;
    measure(context);
    if (key.enableTailColorConvert && !key.mEnabledTextRefactor) {
      overrideTruncatedSpan(context);
    }
    updateSpanRectIfNeed();
  }

  public Layout getTextLayout() {
    return mTextLayout;
  }

  public int getEllipsisCount() {
    return mEllipsisCount;
  }

  int getLayoutWidth() {
    if (mKey.widthMode == MeasureMode.EXACTLY && getTextLayoutWidth() <= mKey.width) {
      return (int) mKey.width;
    }

    return (int) Math.ceil(calculateMaxWidth());
  }

  public PointF getTextTranslateOffset() {
    return new PointF(mTextTranslateOffset, mTextTranslateTopOffset);
  }

  static class TextContextDescriptor {
    private CharSequence mSpan;
    private boolean mShouldBeSingleLine;
    private float mDesiredWidth;
    private int mEllipsizedMode;
    private int mEllipsizedLines;
    private TextPaint mTextPaint;
  }

  private TextContextDescriptor constructTextConstraints(LynxContext context) {
    TextContextDescriptor result = new TextContextDescriptor();
    result.mTextPaint = newTextPaint(context);
    result.mShouldBeSingleLine = shouldBeSingleLine();
    result.mEllipsizedMode = getEllipsizedMode();
    result.mSpan = getUsedSpanClippedWithMaxLength();
    result.mDesiredWidth = getDesiredWidth();
    result.mEllipsizedLines = result.mShouldBeSingleLine ? 1 : mKey.getAttributes().mMaxLineCount;
    return result;
  }

  private CharSequence getUsedSpanClippedWithMaxLength() {
    CharSequence span = mKey.getSpan();
    int maxTextLength = mKey.getAttributes().mMaxTextLength;
    int spanLength = span.length();
    if (maxTextLength != NOT_SET && maxTextLength < spanLength) {
      // Ellipsis will be appended disregarding the overflowing mode.
      span = getEllipsizedSpan((SpannableStringBuilder) span, maxTextLength);
    }
    return span;
  }

  private void insertDirectionMark(SpannableStringBuilder text, int index) {
    if (index < 0 || index > text.length()) {
      return;
    }
    if (mKey.getAttributes().getDirectionHeuristic() == TextDirectionHeuristics.LTR) {
      text.insert(index, LTR_MARK);
    } else if (mKey.getAttributes().getDirectionHeuristic() == TextDirectionHeuristics.RTL) {
      text.insert(index, RTL_MARK);
    }
  }

  protected CharSequence getEllipsizedSpan(SpannableStringBuilder text, int keepLength) {
    keepLength = Math.max(0, keepLength);
    keepLength = Math.min(text.length(), keepLength);
    SpannableStringBuilder dupSpan = (SpannableStringBuilder) text.subSequence(0, keepLength);

    if (mKey.getAttributes().getDirectionHeuristic() == TextDirectionHeuristics.LTR) {
      dupSpan.append(LTR_MARK);
    } else if (mKey.getAttributes().getDirectionHeuristic() == TextDirectionHeuristics.RTL) {
      dupSpan.append(RTL_MARK);
    }

    dupSpan.append(ELLIPSIS);
    return dupSpan;
  }

  private boolean convertTailColor(SpannableStringBuilder text, int index) {
    ForegroundColorSpan[] baseSpans = text.getSpans(0, 1, ForegroundColorSpan.class);
    if (baseSpans == null || baseSpans.length == 0) {
      return false;
    }
    int baseColor = baseSpans[0].getForegroundColor();
    text.setSpan(
        new ForegroundColorSpan(baseColor), index, text.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
    return true;
  }

  private float getDesiredWidth() {
    // If whidth mode is undefined, use Short.MAX_VALUE as width.
    return mKey.widthMode == MeasureMode.EXACTLY || mKey.widthMode == MeasureMode.AT_MOST
        ? mKey.width
        : TEXT_LAYOUT_MAX_WIDTH;
  }

  private boolean shouldBeSingleLine() {
    return mKey.getAttributes().mWhiteSpace == StyleConstants.WHITESPACE_NOWRAP
        || mKey.getAttributes().mMaxLineCount == 1;
  }

  private int getEllipsizedMode() {
    // Determine ellipsis mode and max line count for ellipsis
    boolean shouldEllipsis = mKey.getAttributes().mTextOverflow == TEXTOVERFLOW_ELLIPSIS;
    int lineCountForEllipsis = mKey.getAttributes().mWhiteSpace == StyleConstants.WHITESPACE_NOWRAP
        ? 1
        : mKey.getAttributes().mMaxLineCount;
    int ellipsisMode = MODE_NONE;
    if (shouldEllipsis) {
      if (lineCountForEllipsis != NOT_SET) {
        ellipsisMode = MODE_LINES;
      } else {
        ellipsisMode = MODE_PIXELS;
      }
    }
    return ellipsisMode;
  }

  private void buildTextLayout(TextContextDescriptor textContext, LynxContext context) {
    Layout.Alignment alignment;
    // only need to handle RTL for HitTest when TextAlign is set
    if ((mKey.getAttributes().mTextAlign == TEXTALIGN_LEFT
            || mKey.getAttributes().mTextAlign == TEXTALIGN_RIGHT)
        && mKey.getAttributes().mDirection == DIRECTION_NORMAL) {
      // Normal Direction && TextAlign is set && AlignSpan is in the beginning of the whole span
      if (mKey.getAttributes().mFirstCharacterRTLState != FIRST_CHAR_RTL_STATE_NONE_CHECK) {
        alignment = mKey.getAttributes().getLayoutAlignment(
            mKey.getAttributes().mFirstCharacterRTLState == FIRST_CHAR_RTL_STATE_RTL);
      } else {
        // AlignSpan is not the beginning of the whole span or Direction is not Normal
        Bidi bidi = new Bidi(textContext.mSpan.toString(), Bidi.DIRECTION_DEFAULT_LEFT_TO_RIGHT);
        alignment = mKey.getAttributes().getLayoutAlignment(!bidi.baseIsLeftToRight());
      }
    } else {
      alignment = mKey.getAttributes().getLayoutAlignment();
    }

    BoringLayout.Metrics metrics = null;
    if (canUseBoringLayout((SpannableStringBuilder) textContext.mSpan)) {
      metrics = BoringLayout.isBoring(textContext.mSpan, textContext.mTextPaint);
      if (metrics != null) {
        mTextLayout = generateBoringLayout(textContext, alignment, metrics);
      }
    }
    if (metrics == null) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
        // use StaticLayout#Builder
        StaticLayout.Builder builder = generateLayoutBuilder(
            textContext.mSpan, alignment, textContext.mTextPaint, textContext.mDesiredWidth);

        if (textContext.mEllipsizedMode == MODE_LINES) {
          builder.setEllipsize(TextUtils.TruncateAt.END)
              .setEllipsizedWidth((int) Math.floor(textContext.mDesiredWidth))
              .setMaxLines(textContext.mEllipsizedLines);
        }
        if (textContext.mEllipsizedLines > 0) {
          builder.setMaxLines(textContext.mEllipsizedLines);
        }
        if (textContext.mShouldBeSingleLine) {
          builder.setMaxLines(1);
        }
        if (mKey.getAttributes().mTextAlign == StyleConstants.TEXTALIGN_JUSTIFY
            && !mKey.getAttributes().hasInlineViewSpan()
            && Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
          builder.setJustificationMode(LineBreaker.JUSTIFICATION_MODE_INTER_WORD);
        }

        if (mKey.getAttributes().getHyphen()) {
          builder.setHyphenationFrequency(Layout.HYPHENATION_FREQUENCY_FULL);
          builder.setBreakStrategy(LineBreaker.BREAK_STRATEGY_HIGH_QUALITY);
        }

        mTextLayout = builder.build();

        if (textContext.mEllipsizedMode == MODE_LINES) {
          int lastLine = mTextLayout.getLineCount() - 1;
          if (Build.VERSION.SDK_INT <= Build.VERSION_CODES.P
              && (mKey.getAttributes().hasImageSpan() || mKey.getAttributes().hasInlineViewSpan())
              && mTextLayout.getEllipsisCount(lastLine) > 0) {
            // When the last character before the ellipsis is an inline element, the number of
            // omissions will be abnormal, and the ellipsis will disappear, so the text will be
            // relayout.
            int lastCharIndex =
                mTextLayout.getLineStart(lastLine) + mTextLayout.getEllipsisStart(lastLine) - 1;
            if (isInlineElementAtChar(lastCharIndex, (SpannableStringBuilder) textContext.mSpan)) {
              textContext.mSpan =
                  getEllipsizedSpan((SpannableStringBuilder) textContext.mSpan, lastCharIndex + 1);
              builder = generateLayoutBuilder(
                  textContext.mSpan, alignment, textContext.mTextPaint, textContext.mDesiredWidth);
              mTextLayout = builder.build();
            }
          }
          // TODO(zhouzhuangzhuang): try to avoid call getLineWidth
          // Fix the incomplete display of ellipsis in certain fonts
          if (mTextLayout.getLineWidth(mTextLayout.getLineCount() - 1)
              > textContext.mDesiredWidth) {
            // regenerate Builder
            builder = generateLayoutBuilder(
                textContext.mSpan, alignment, textContext.mTextPaint, textContext.mDesiredWidth);
            builder.setMaxLines(textContext.mEllipsizedLines);
            builder.setEllipsize(TextUtils.TruncateAt.END);
            builder.setEllipsizedWidth((int) (Math.floor(textContext.mDesiredWidth) * 2
                - mTextLayout.getLineWidth(mTextLayout.getLineCount() - 1)));
            mTextLayout = builder.build();
          }
        }
      } else {
        mTextLayout = StaticLayoutCompat.get(textContext.mSpan, 0, textContext.mSpan.length(),
            textContext.mTextPaint, (int) Math.floor(textContext.mDesiredWidth), alignment, 1,
            mKey.getAttributes().mLineSpacing, mKey.getAttributes().mIncludePadding,
            textContext.mEllipsizedMode == MODE_LINES ? TextUtils.TruncateAt.END : null,
            textContext.mEllipsizedLines, mKey.getAttributes().getDirectionHeuristic());
      }
    }
    // Max width cache invalidation
    mCacheMaxWidth = -1.f;
  }

  private boolean canUseBoringLayout(SpannableStringBuilder span) {
    return mKey.mEnableTextBoringLayout && !mKey.getAttributes().hasInlineViewSpan()
        && !mKey.getAttributes().hasImageSpan() && mKey.getAttributes().mDirection != DIRECTION_RTL
        && mKey.getAttributes().mFirstCharacterRTLState != FIRST_CHAR_RTL_STATE_RTL
        && (mKey.getAttributes().mWhiteSpace == StyleConstants.WHITESPACE_NOWRAP
            || (mKey.getAttributes().getMaxLineCount() == 1
                && mKey.getAttributes().getTextOverflow() == TEXTOVERFLOW_ELLIPSIS)
            || mKey.widthMode == MeasureMode.UNDEFINED)
        && (span.length() == 0 || span.getSpans(0, 1, CustomBaselineShiftSpan.class).length == 0);
  }

  private boolean isInlineElementAtChar(int charIndex, SpannableStringBuilder spannableString) {
    if (charIndex >= spannableString.length()) {
      return false;
    }

    return spannableString.getSpans(charIndex, charIndex + 1, AbsInlineImageSpan.class).length > 0
        || spannableString.getSpans(charIndex, charIndex + 1, NativeLayoutNodeSpan.class).length
        > 0;
  }

  private void handleEllipsisBidiAndColorConvert(
      TextContextDescriptor textContext, LynxContext context) {
    if (textContext.mEllipsizedMode == MODE_NONE
        || mKey.getAttributes().getDirectionHeuristic() == TextDirectionHeuristics.FIRSTSTRONG_LTR
            && !mKey.enableTailColorConvert) {
      return;
    }
    int lastLine = mTextLayout.getLineCount() - 1;
    mEllipsisCount = mTextLayout.getEllipsisCount(lastLine);
    if (mEllipsisCount > 0) {
      boolean isNeedRelayout = false;
      int ellipsisIndex =
          mTextLayout.getLineStart(lastLine) + mTextLayout.getEllipsisStart(lastLine);
      // When the text contains line breaks, the string returned by the function exceeds the
      // displayed range and needs to be manually truncated.
      SpannableStringBuilder newSpan =
          new SpannableStringBuilder(mTextLayout.getText().subSequence(0, ellipsisIndex + 1));
      if (mKey.enableTailColorConvert) {
        isNeedRelayout = convertTailColor(newSpan, ellipsisIndex);
      }
      if (mKey.getAttributes().getDirectionHeuristic() != TextDirectionHeuristics.FIRSTSTRONG_LTR) {
        insertDirectionMark(newSpan, ellipsisIndex);
        isNeedRelayout = true;
      }

      if (isNeedRelayout) {
        textContext.mSpan = newSpan;
        textContext.mEllipsizedMode = MODE_NONE;
        int originLineCount = mTextLayout.getLineCount();
        buildTextLayout(textContext, context);
        ensureSameLineCount(textContext, context, originLineCount);
      }
    }
  }

  private void ensureSameLineCount(
      TextContextDescriptor textContext, LynxContext context, int originLineCount) {
    if (mTextLayout.getLineCount() <= originLineCount) {
      return;
    }
    while (mTextLayout.getLineCount() > originLineCount && textContext.mSpan.length() >= 2) {
      int removeCharCount =
          Character.isLowSurrogate(textContext.mSpan.charAt(textContext.mSpan.length() - 2))
              && textContext.mSpan.length() > 2
          ? 2
          : 1;
      textContext.mSpan = ((SpannableStringBuilder) textContext.mSpan)
                              .delete(textContext.mSpan.length() - 1 - removeCharCount,
                                  textContext.mSpan.length() - 1);
      buildTextLayout(textContext, context);
    }
  }

  private void handleHeightOverflowByLineCount(
      TextContextDescriptor textContext, LynxContext context) {
    if (textContext.mEllipsizedMode == MODE_NONE) {
      return;
    }
    if (mKey.heightMode == MeasureMode.UNDEFINED || mTextLayout.getHeight() <= mKey.height
        || textContext.mShouldBeSingleLine) {
      return;
    }
    int lineIdx;
    for (lineIdx = mTextLayout.getLineCount() - 1; lineIdx > 0; --lineIdx) {
      if (mTextLayout.getLineBottom(lineIdx) <= mKey.height) {
        break;
      }
    }
    textContext.mEllipsizedLines = lineIdx + 1;
    textContext.mEllipsizedMode = MODE_LINES;
    buildTextLayout(textContext, context);
  }

  private void measure(LynxContext context) {
    CharSequence span = mKey.mBaseKey.mText;
    if (span == null) {
      throw new RuntimeException("prepareSpan() should be called!");
    }
    TextContextDescriptor textContext = constructTextConstraints(context);
    handleWhiteSpaceWrap(textContext);
    buildTextLayout(textContext, context);
    handleAutoSize(textContext, context);
    handleMaxWidthMode();
    handleHeightOverflowByLineCount(textContext, context);
    handleEllipsisBidiAndColorConvert(textContext, context);
    calcTextTranslateTopOffset();
  }

  private void calcTextTranslateTopOffset() {
    mTextTranslateTopOffset = 0.f;
    if (isNeedCalcOffsetForLineHeight()) {
      mTextTranslateTopOffset = TextHelper.calcTextTranslateTopOffsetAndAdjustFontMetric(
          (int) Math.ceil(mKey.getAttributes().getLineHeight()),
          mTextLayout.getPaint().getFontMetricsInt(), mKey.getAttributes().isIncludePadding());
    }
  }

  public boolean isNeedCalcOffsetForLineHeight() {
    return !MeasureUtils.isUndefined(mKey.getAttributes().getLineHeight())
        && mKey.mEnableTextBoringLayout && mKey.mEnabledTextRefactor
        && mKey.getAttributes().getWhiteSpace() == WHITESPACE_NOWRAP
        && !mKey.getAttributes().hasImageSpan() && !mKey.getAttributes().hasInlineViewSpan();
  }

  private void handleAutoSize(TextContextDescriptor textContext, LynxContext context) {
    if (!mKey.getAttributes().getIsAutoFontSize() || mKey.widthMode == MeasureMode.UNDEFINED
        || (Build.VERSION.SDK_INT < Build.VERSION_CODES.N
            && !mKey.getAttributes().isBoringSpan())) {
      // Can not be compatible with the situation that text contains inline text on Android version
      // below 7.0.
      return;
    }

    boolean isShrink = isTextContentOverflow();
    int currentFontSize = getCurrentFontSize();

    if (isShrink) {
      if (!MeasureUtils.isUndefined(mKey.getAttributes().getLineHeight())
          && mKey.heightMode != MeasureMode.UNDEFINED
          && mKey.getAttributes().getLineHeight() > mKey.height && !isTextTruncated()) {
        return;
      }
      // shrink
      do {
        int smallerFontSize = findSmallerFontSize(currentFontSize);
        if (smallerFontSize < 0) {
          break;
        }
        buildTextLayoutForAutoSize(smallerFontSize, textContext, context);
        currentFontSize = smallerFontSize;
      } while (isTextContentOverflow());
    } else {
      // grow
      while (true) {
        int largerFontSize = findLargerFontSize(currentFontSize);
        if (largerFontSize < 0) {
          break;
        }
        buildTextLayoutForAutoSize(largerFontSize, textContext, context);

        if (isTextContentOverflow()) {
          buildTextLayoutForAutoSize(currentFontSize, textContext, context);
          break;
        }
        currentFontSize = largerFontSize;
      }
    }
  }

  private int getCurrentFontSize() {
    int currentFontSize = (int) mKey.getAttributes().getFontSize();
    AbsoluteSizeSpan[] absoluteSizeSpans =
        ((Spanned) mKey.getSpan()).getSpans(0, mKey.getSpan().length(), AbsoluteSizeSpan.class);
    for (int i = 0; i < absoluteSizeSpans.length; i++) {
      if (currentFontSize < absoluteSizeSpans[i].getSize()) {
        currentFontSize = absoluteSizeSpans[i].getSize();
      }
    }

    return currentFontSize;
  }

  private void removeAbsoluteSizeSpan(SpannableStringBuilder spannableStringBuilder) {
    AbsoluteSizeSpan[] absoluteSizeSpans =
        spannableStringBuilder.getSpans(0, spannableStringBuilder.length(), AbsoluteSizeSpan.class);
    for (int i = 0; i < absoluteSizeSpans.length; i++) {
      spannableStringBuilder.removeSpan(absoluteSizeSpans[i]);
    }
  }

  private void buildTextLayoutForAutoSize(
      int fontSize, TextContextDescriptor textContext, LynxContext context) {
    removeAbsoluteSizeSpan((SpannableStringBuilder) textContext.mSpan);
    textContext.mTextPaint.setTextSize(fontSize);
    buildTextLayout(textContext, context);
  }

  private int findSmallerFontSize(int currentFontSize) {
    float[] fontSizes = mKey.getAttributes().getAutoFontSizePresetSizes();
    if (fontSizes != null) {
      for (int i = fontSizes.length - 1; i >= 0; i--) {
        if (fontSizes[i] < currentFontSize) {
          return (int) fontSizes[i];
        }
      }
    } else {
      int smallerFontSize =
          (int) (currentFontSize - mKey.getAttributes().getAutoFontSizeStepGranularity());
      if (smallerFontSize == currentFontSize) {
        return -1;
      }
      if (smallerFontSize >= mKey.getAttributes().getAutoFontSizeMinSize()) {
        return smallerFontSize;
      }
    }

    return -1;
  }

  private int findLargerFontSize(int currentFontSize) {
    float[] fontSizes = mKey.getAttributes().getAutoFontSizePresetSizes();
    if (fontSizes != null) {
      for (int i = 0; i < fontSizes.length; i++) {
        if (fontSizes[i] > currentFontSize) {
          return (int) fontSizes[i];
        }
      }
    } else {
      int largerFontSize =
          (int) (currentFontSize + mKey.getAttributes().getAutoFontSizeStepGranularity());
      if (largerFontSize == currentFontSize) {
        return -1;
      }
      if (largerFontSize <= mKey.getAttributes().getAutoFontSizeMaxSize()) {
        return largerFontSize;
      }
    }

    return -1;
  }

  protected boolean isTextContentOverflow() {
    return (mKey.heightMode != MeasureMode.UNDEFINED && mTextLayout.getHeight() > mKey.height)
        || calculateMaxWidth() > mKey.width || isTextTruncated();
  }

  private boolean isTextTruncated() {
    return mTextLayout.getLineCount() > getLineCount()
        || mTextLayout.getEllipsisCount(mTextLayout.getLineCount() - 1) > 0
        // Compatible with Android version below 8.0.The results returned by other APIs can not
        // determine whether the content of the text is overflow.
        || mTextLayout.getLineEnd(mTextLayout.getLineCount() - 1) < mTextLayout.getText().length();
  }

  @RequiresApi(api = Build.VERSION_CODES.M)
  private StaticLayout.Builder generateLayoutBuilder(
      CharSequence span, Layout.Alignment alignment, TextPaint textPaint, float desiredWidth) {
    // use StaticLayout#Builder
    StaticLayout.Builder builder = StaticLayout.Builder.obtain(
        span, 0, span.length(), textPaint, (int) Math.floor(desiredWidth));

    // set common properties
    // alignment
    builder.setAlignment(alignment);
    // line spacing
    builder.setLineSpacing(mKey.getAttributes().mLineSpacing, SPACING_MULT);
    // including padding
    builder.setIncludePad(mKey.getAttributes().mIncludePadding);
    // Set direction
    builder.setTextDirection(mKey.getAttributes().getDirectionHeuristic());
    // break strategy
    builder.setBreakStrategy(mKey.wordBreakStrategy);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      // API higher than 28 use line spacing fallbacks
      builder.setUseLineSpacingFromFallbacks(true);
    }

    return builder;
  }

  private Layout generateBoringLayout(
      TextContextDescriptor textContext, Layout.Alignment alignment, BoringLayout.Metrics metrics) {
    return textContext.mEllipsizedMode == MODE_LINES
        ? BoringLayout.make(textContext.mSpan, textContext.mTextPaint,
            (int) Math.floor(textContext.mDesiredWidth), alignment, SPACING_MULT,
            mKey.getAttributes().mLineSpacing, metrics, mKey.getAttributes().isIncludePadding(),
            TextUtils.TruncateAt.END, (int) Math.floor(textContext.mDesiredWidth))
        : BoringLayout.make(textContext.mSpan, textContext.mTextPaint,
            (int) Math.floor(textContext.mDesiredWidth), alignment, SPACING_MULT,
            mKey.getAttributes().mLineSpacing, metrics, mKey.getAttributes().isIncludePadding());
  }

  public int getTextLayoutWidth() {
    return mTextLayout.getWidth();
  }

  public int getTextLayoutHeight() {
    if (isNeedCalcOffsetForLineHeight()) {
      return (int) Math.ceil(mKey.getAttributes().getLineHeight());
    }

    int maxLine = mKey.getAttributes().getMaxLineCount();
    if (shouldBeSingleLine()) {
      maxLine = 1;
    }
    if (maxLine == NOT_SET || maxLine > mTextLayout.getLineCount()) {
      return mTextLayout.getHeight();
    }

    return mTextLayout.getLineBottom(maxLine - 1);
  }

  public int getLineCount() {
    int maxLine = mKey.getAttributes().getMaxLineCount();
    if (maxLine == NOT_SET || maxLine > mTextLayout.getLineCount()) {
      return mTextLayout.getLineCount();
    }
    return maxLine;
  }

  // Prepare styled text paint
  private TextPaint newTextPaint(LynxContext context) {
    // textPaint.setFlags may lose some properties,  maybe cause emoji overlap!
    mTypeface = TextHelper.getTypeFaceFromCache(context, mKey.getAttributes(), null);
    return TextHelper.newTextPaint(mKey.getAttributes(), mTypeface);
  }

  private void overrideTruncatedSpan(LynxContext context) {
    if (mTextLayout.getEllipsisCount(mTextLayout.getLineCount() - 1) == 0) {
      return;
    }

    if (!(mKey.getSpan() instanceof SpannableStringBuilder)) {
      return;
    }

    int lastLine = mTextLayout.getLineCount() - 1;
    int ellipsisStart = mTextLayout.getLineStart(lastLine) + mTextLayout.getEllipsisStart(lastLine);

    SpannableStringBuilder spannableStringBuilder = new SpannableStringBuilder(mKey.getSpan());
    ForegroundColorSpan[] baseSpans =
        spannableStringBuilder.getSpans(0, 1, ForegroundColorSpan.class);

    if (baseSpans == null || baseSpans.length == 0) {
      return;
    }
    ForegroundColorSpan[] tailSpans = spannableStringBuilder.getSpans(
        ellipsisStart, ellipsisStart + 1, ForegroundColorSpan.class);

    if (tailSpans == null || tailSpans.length == 0) {
      return;
    }
    ForegroundColorSpan tailSpan = tailSpans[tailSpans.length - 1];
    int tailSpanStart = spannableStringBuilder.getSpanStart(tailSpan);
    int tailSpanEnd = spannableStringBuilder.getSpanEnd(tailSpan);
    spannableStringBuilder.removeSpan(tailSpan);

    if (tailSpanStart < ellipsisStart) {
      spannableStringBuilder.setSpan(
          tailSpan, tailSpanStart, ellipsisStart, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
    }

    int baseColor = baseSpans[0].getForegroundColor();

    spannableStringBuilder.setSpan(new ForegroundColorSpan(baseColor), ellipsisStart, tailSpanEnd,
        Spanned.SPAN_EXCLUSIVE_INCLUSIVE);
    mKey.setSpan(spannableStringBuilder);
    mTextLayout = null;
    measure(context);
  }

  private void updateSpanRectIfNeed() {
    if (mTextLayout.getWidth() == 0 || mTextLayout.getHeight() == 0) {
      return;
    }
    Spanned spannedString = (Spanned) mTextLayout.getText();
    LynxTextGradientSpan[] gradientSpans =
        spannedString.getSpans(0, spannedString.length(), LynxTextGradientSpan.class);

    for (LynxTextGradientSpan gradientSpan : gradientSpans) {
      int start = spannedString.getSpanStart(gradientSpan);
      int end = spannedString.getSpanEnd(gradientSpan);

      if (start == 0 && end == spannedString.length()) {
        float maxWidth = calculateMaxWidth();
        // this is outer gradient just set total bounds size
        gradientSpan.updateBounds(new Rect((int) -mTextTranslateOffset, 0,
            (int) (-mTextTranslateOffset + maxWidth), mTextLayout.getHeight()));
        continue;
      }

      int startLine = mTextLayout.getLineForOffset(start);
      int endLine = mTextLayout.getLineForOffset(end);
      Rect bounds = new Rect();
      for (int i = startLine; i <= endLine; i++) {
        Rect lineBounds = new Rect();
        mTextLayout.getLineBounds(i, lineBounds);
        if (i == startLine) {
          lineBounds.left =
              Math.max(lineBounds.left, (int) mTextLayout.getPrimaryHorizontal(start));
        }
        if (i == endLine) {
          lineBounds.right =
              Math.min(lineBounds.right, (int) mTextLayout.getPrimaryHorizontal(end));
        }
        if (lineBounds.right == 0) {
          continue;
        }
        bounds.union(lineBounds);
      }
      gradientSpan.updateBounds(bounds);
    }

    LynxTextBackgroundSpan[] backgroundSpans =
        spannedString.getSpans(0, spannedString.length(), LynxTextBackgroundSpan.class);
    for (int i = 0; i < backgroundSpans.length; i++) {
      backgroundSpans[i].updateSpanPosition(mTextLayout);
    }
  }

  public boolean isEnableCache() {
    if (mKey.mBaseKey.mAttributes.mHasImageSpan || mKey.mBaseKey.mAttributes.mHasInlineViewSpan) {
      // inline-image or inline-view should not put to cache
      return false;
    }

    Spanned spannedString = (Spanned) mTextLayout.getText();
    EventTargetSpan[] spans =
        spannedString.getSpans(0, spannedString.length(), EventTargetSpan.class);

    return spans == null || spans.length <= 0;
  }

  private void handleWhiteSpaceWrap(TextContextDescriptor textContext) {
    if (mKey.getAttributes().mWhiteSpace != WHITESPACE_NOWRAP) {
      return;
    }

    // if white-space:nowrap, and contains hard line-break, all characters after first line-break
    // need to delete
    String rawString = textContext.mSpan.toString();
    SpannableStringBuilder dupSpan = new SpannableStringBuilder(textContext.mSpan);
    int lineBreak = rawString.indexOf('\n');
    if (lineBreak > 0) {
      dupSpan.delete(lineBreak, dupSpan.length());
    }

    textContext.mSpan = dupSpan;
    if (mKey.getAttributes().getTextOverflow() != TEXTOVERFLOW_ELLIPSIS) {
      textContext.mDesiredWidth = TEXT_LAYOUT_MAX_WIDTH;
    }
  }

  private void handleMaxWidthMode() {
    if (mKey.widthMode == MeasureMode.EXACTLY
        && mKey.getAttributes().getWhiteSpace() != WHITESPACE_NOWRAP) {
      return;
    }

    float textMaxWidth = calculateMaxWidth();
    if (mKey.getAttributes().getWhiteSpace() == WHITESPACE_NOWRAP
        && mKey.widthMode == MeasureMode.EXACTLY && mKey.width > textMaxWidth) {
      if (mKey.getAttributes().getTextOverflow() != TEXTOVERFLOW_ELLIPSIS) {
        // text-align will take effect if textMaxWidth < mKey.width
        mTextTranslateOffset = calcTextTranslateOffset(mKey.width);
      }
      // do not set mOffset.x if text-overflow is ellipsis
    } else {
      mTextTranslateOffset = calcTextTranslateOffset(textMaxWidth);
    }
  }

  private float calcTextTranslateOffset(float textMaxWidth) {
    if (mTextLayout.getLineLeft(0) == 0.f) {
      return 0.f;
    }

    float offset = 0.f;
    if (mTextLayout.getAlignment() == Layout.Alignment.ALIGN_CENTER) {
      // make text drawing in center
      offset = -(mTextLayout.getWidth() - textMaxWidth) / 2;
    } else if (mTextLayout.getAlignment() == Layout.Alignment.ALIGN_OPPOSITE
        || mTextLayout.getParagraphDirection(0) == Layout.DIR_RIGHT_TO_LEFT
        || mTextLayout.getParagraphAlignment(0) == Layout.Alignment.ALIGN_OPPOSITE) {
      // need to make sure text content is drawing nearest to the right
      // TODO(zhouzhuangzhuang): remove `mTextLayout.getParagraphAlignment(0) ==
      // Layout.Alignment.ALIGN_OPPOSITE` later
      offset = -(mTextLayout.getWidth() - textMaxWidth);
    }

    return offset;
  }

  protected float calculateMaxWidth() {
    if (mCacheMaxWidth >= 0.f) {
      return mCacheMaxWidth;
    }
    mCacheMaxWidth = -1.f;

    // use getLineCount to get real visible line count
    for (int i = 0; i < getLineCount(); i++) {
      mCacheMaxWidth = Math.max(mCacheMaxWidth, calculateLineWidth(i));
    }
    // handle italic font cut off
    if (isContainItalicFont()) {
      // use ascent instead of x-height, can not get x-height easily
      mCacheMaxWidth += -mTextLayout.getLineAscent(0) * 0.2;
    }
    return mCacheMaxWidth;
  }

  // trigger textLine measure
  private float calculateLineWidth(int lineIndex) {
    return mKey.getAttributes().getLayoutAlignment() == Layout.Alignment.ALIGN_NORMAL
        ? mTextLayout.getLineMax(lineIndex)
        : mTextLayout.getLineMax(lineIndex) - mTextLayout.getParagraphLeft(lineIndex);
  }

  private boolean isContainItalicFont() {
    if (mKey.getSpan().length() == 0) {
      return false;
    }

    int visibleTextLength = mTextLayout.getLineEnd(getLineCount() - 1);
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      CustomStyleSpan[] spans = ((SpannableStringBuilder) mKey.getSpan())
                                    .getSpans(0, visibleTextLength, CustomStyleSpan.class);
      for (int i = 0; i < spans.length; i++) {
        if (spans[i].getStyle() == Typeface.ITALIC) {
          return true;
        }
      }
    } else {
      StyleSpan[] spans =
          ((SpannableStringBuilder) mKey.getSpan()).getSpans(0, visibleTextLength, StyleSpan.class);
      for (int i = 0; i < spans.length; i++) {
        if (spans[i].getStyle() == Typeface.ITALIC) {
          return true;
        }
      }
    }

    return false;
  }
}
