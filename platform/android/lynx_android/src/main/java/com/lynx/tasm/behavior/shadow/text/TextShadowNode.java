// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_START;
import static com.lynx.tasm.behavior.StyleConstants.TEXTOVERFLOW_CLIP;
import static java.util.Arrays.asList;

import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextPaint;
import android.text.TextUtils;
import android.text.style.LeadingMarginSpan;
import android.text.style.MetricAffectingSpan;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.TraceEvent;
import com.lynx.tasm.behavior.LynxProp;
import com.lynx.tasm.behavior.PropsConstants;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.AlignContext;
import com.lynx.tasm.behavior.shadow.AlignParam;
import com.lynx.tasm.behavior.shadow.CustomMeasureFunc;
import com.lynx.tasm.behavior.shadow.LayoutNode;
import com.lynx.tasm.behavior.shadow.MeasureContext;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureOutput;
import com.lynx.tasm.behavior.shadow.MeasureParam;
import com.lynx.tasm.behavior.shadow.MeasureResult;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.shadow.NativeLayoutNodeRef;
import com.lynx.tasm.behavior.shadow.ShadowNode;
import com.lynx.tasm.behavior.shadow.text.TextHelper;
import com.lynx.tasm.fontface.FontFaceManager;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class TextShadowNode extends BaseTextShadowNode implements CustomMeasureFunc {
  private static final String TAG = "lynx_TextShadowNode";

  protected TextRenderer mRenderer;
  // origin text node string
  protected CharSequence mSpannableString;
  // truncation node string
  private CharSequence mTruncationSpannableString;
  // string after truncated
  private CharSequence mTruncatedSpannableString;
  private InlineTruncationShadowNode mTruncationShadowNode;
  private boolean mEnableTailColorConvert = false;
  private boolean mEnableFullJustify = false;

  private boolean mIsCalcXHeight = false;
  private boolean mIsCalcAscenderAndDescender = false;
  private float mMaxXHeight = Float.MIN_VALUE;
  private float mMinAscender = Float.MAX_VALUE;
  private float mMaxDescender = Float.MIN_VALUE;

  protected int mEllipsisCount = 0;

  public TextShadowNode() {
    initMeasureFunction();
  }

  private void initMeasureFunction() {
    if (!isVirtual()) {
      setCustomMeasureFunc(this);
    }
  }

  @Override
  public void onLayoutBefore() {
    if (!isVirtual()) {
      mRenderer = null;
      mTruncationSpannableString = null;
      prepareSpan();
    }
  }

  protected boolean isBoringSpan() {
    return ((getChildCount() == 1 && getChildAt(0) instanceof RawTextShadowNode)
               || (getChildCount() == 0 && mText != null))
        && MeasureUtils.isUndefined(getTextAttributes().mLineHeight);
  }

  protected void prepareSpan() {
    if (!isTextRefactorEnabled()) {
      setTextAlignFromInlineText();
    }
    boolean isBoringSpan = isBoringSpan();
    getTextAttributes().setIsBoringSpan(isBoringSpan);
    if (isBoringSpan) {
      if (getChildCount() == 0) {
        mSpannableString = getCharSequence(mText, false);
      } else {
        RawTextShadowNode rawTextShadowNode = (RawTextShadowNode) getChildAt(0);
        mSpannableString =
            getCharSequence(rawTextShadowNode.getText(), rawTextShadowNode.isPseudo());
      }

      // TODO(zhouzhuangzhuang): When mSpannableString is equal to null, the layout height returns
      // 0, which is inconsistent with iOS.
      if (mSpannableString == null) {
        // Fix npe exception
        return;
      }
      List<SetSpanOperation> ops = new ArrayList<>();
      SpannableStringBuilder spannableStringBuilder = new SpannableStringBuilder();
      spannableStringBuilder.append(mSpannableString);
      buildStyledSpan(0, mSpannableString.length(), ops);
      for (SetSpanOperation op : ops) {
        op.execute(spannableStringBuilder);
      }
      mSpannableString = spannableStringBuilder;
      return;
    }

    mSpannableString = new SpannableStringBuilder();
    buildSpannableString((SpannableStringBuilder) mSpannableString, this);
    prepareTruncationSpan();

    setFontMetricForVerticalAlign();
  }

  // In order to be compatible with old pages,setting text-align on first descendant node will
  // override TextShadowNode alignment.
  private void setTextAlignFromInlineText() {
    int textAlign = TEXTALIGN_START;
    ShadowNode node = this;
    while (node.getChildCount() > 0) {
      ShadowNode firstChild = node.getChildAt(0);
      if (firstChild instanceof InlineTextShadowNode) {
        int childTextAlign = ((InlineTextShadowNode) firstChild).getTextAttributes().getTextAlign();
        if (childTextAlign != TEXTALIGN_START) {
          textAlign = childTextAlign;
        }
      } else {
        break;
      }
      node = firstChild;
    }

    if (textAlign != TEXTALIGN_START) {
      getTextAttributes().setTextAlign(textAlign);
    }
  }

  private void prepareTruncationSpan() {
    mTruncationShadowNode = null;
    for (int i = 0; i < getChildCount(); i++) {
      if (getChildAt(i) instanceof InlineTruncationShadowNode) {
        mTruncationShadowNode = (InlineTruncationShadowNode) getChildAt(i);
        break;
      }
    }

    if (mTruncationShadowNode != null) {
      mTruncationSpannableString = new SpannableStringBuilder();
      buildSpannableString(
          (SpannableStringBuilder) mTruncationSpannableString, mTruncationShadowNode);

      getTextAttributes().setTextOverflow(TEXTOVERFLOW_CLIP);
    }
  }

  private void buildSpannableString(
      SpannableStringBuilder spannableString, BaseTextShadowNode node) {
    // The {@link SpannableStringBuilder} implementation require setSpan operation to be called
    // up-to-bottom, otherwise all the spannables that are withing the region for which one may set
    // a new spannable will be wiped out
    List<SetSpanOperation> ops = new ArrayList<>();
    node.generateStyleSpan(spannableString, ops);

    // While setting the Spans on the final text, we also check whether any of them are images
    for (int i = ops.size() - 1; i >= 0; i--) {
      SetSpanOperation op = ops.get(i);
      op.execute(spannableString);
    }

    setIsCalcMaxFontMetric(spannableString);
  }

  protected void setIsCalcMaxFontMetric(SpannableStringBuilder spannableString) {
    MetricAffectingSpan[] metricAffectingSpans =
        ((SpannableStringBuilder) spannableString)
            .getSpans(0, spannableString.length(), MetricAffectingSpan.class);

    for (int i = 0; i < metricAffectingSpans.length; i++) {
      int verticalAlign = 0;
      if (metricAffectingSpans[i] instanceof AbsBaselineShiftCalculatorSpan) {
        verticalAlign =
            ((AbsBaselineShiftCalculatorSpan) metricAffectingSpans[i]).getVerticalAlign();
      } else if (metricAffectingSpans[i] instanceof InlineTextBaselineShiftSpan) {
        verticalAlign = ((InlineTextBaselineShiftSpan) metricAffectingSpans[i]).getVerticalAlign();
      }
      mIsCalcAscenderAndDescender =
          mIsCalcAscenderAndDescender || isNeedCalcAscenderAndDescender(verticalAlign);
      mIsCalcXHeight = mIsCalcXHeight || isNeedCalcXHeight(verticalAlign);
    }
  }

  private boolean isNeedCalcAscenderAndDescender(int verticalAlign) {
    return verticalAlign == StyleConstants.VERTICAL_ALIGN_TEXT_TOP
        || verticalAlign == StyleConstants.VERTICAL_ALIGN_TEXT_BOTTOM
        || verticalAlign == StyleConstants.VERTICAL_ALIGN_TOP
        || verticalAlign == StyleConstants.VERTICAL_ALIGN_BOTTOM
        || verticalAlign == StyleConstants.VERTICAL_ALIGN_CENTER;
  }

  private boolean isNeedCalcXHeight(int verticalAlign) {
    return verticalAlign == StyleConstants.VERTICAL_ALIGN_MIDDLE;
  }

  private void calcFontMetricForVerticalAlign(BaseTextShadowNode node) {
    TextPaint textPaint = TextHelper.newTextPaint(getContext(), node.getTextAttributes(), null);
    if (mIsCalcAscenderAndDescender) {
      mMinAscender = Math.min(textPaint.getFontMetrics().ascent, mMinAscender);
      mMaxDescender = Math.max(textPaint.getFontMetrics().descent, mMaxDescender);
    }
    if (mIsCalcXHeight) {
      Rect bounds = new Rect();
      // this will just retrieve the bounding rect for 'x'
      textPaint.getTextBounds("x", 0, 1, bounds);
      int xHeight = bounds.height();
      mMaxXHeight = Math.max(mMaxXHeight, xHeight);
    }
    for (int i = 0; i < node.getChildCount(); i++) {
      ShadowNode child = node.getChildAt(i);
      if (child instanceof InlineTextShadowNode || child instanceof InlineTruncationShadowNode) {
        calcFontMetricForVerticalAlign((BaseTextShadowNode) child);
      }
    }
  }

  protected void setFontMetricForVerticalAlign() {
    mMinAscender = Float.MAX_VALUE;
    mMaxDescender = Float.MIN_VALUE;
    mMaxXHeight = Float.MIN_VALUE;
    if (mIsCalcAscenderAndDescender || mIsCalcXHeight) {
      // include raw text
      calcFontMetricForVerticalAlign(this);
    }

    float lineHeight = getTextAttributes().mLineHeight == MeasureUtils.UNDEFINED
        ? 0.f
        : getTextAttributes().mLineHeight;
    BaselineShiftCalculator baselineShiftCalculator =
        new BaselineShiftCalculator(asList(mMinAscender, mMaxDescender, mMaxXHeight, lineHeight));

    initBaselineShiftSpan(mSpannableString, baselineShiftCalculator);
    if (mTruncationSpannableString != null) {
      initBaselineShiftSpan(mTruncationSpannableString, baselineShiftCalculator);
    }
  }

  private void initBaselineShiftSpan(
      CharSequence spannableString, BaselineShiftCalculator baselineShiftCalculator) {
    AbsBaselineShiftCalculatorSpan[] absBaselineShiftCalculatorSpans =
        ((SpannableStringBuilder) spannableString)
            .getSpans(0, spannableString.length(), AbsBaselineShiftCalculatorSpan.class);
    for (int i = 0; i < absBaselineShiftCalculatorSpans.length; i++) {
      absBaselineShiftCalculatorSpans[i].setBaselineShiftCalculator(baselineShiftCalculator);
      absBaselineShiftCalculatorSpans[i].setEnableTextRefactor(
          getContext().isTextRefactorEnabled());
    }

    InlineTextBaselineShiftSpan[] inlineTextBaselineShiftSpans =
        ((SpannableStringBuilder) spannableString)
            .getSpans(0, spannableString.length(), InlineTextBaselineShiftSpan.class);
    for (int i = 0; i < inlineTextBaselineShiftSpans.length; i++) {
      inlineTextBaselineShiftSpans[i].setBaselineShiftCalculator(baselineShiftCalculator);
      inlineTextBaselineShiftSpans[i].setLineHeight(getTextAttributes().getLineHeight());
    }
  }

  @Override
  protected void buildStyledSpan(int start, int end, List<SetSpanOperation> ops) {
    if (getTextAttributes().mTextIndent != null) {
      ops.add(new SetSpanOperation(start, end,
          new LeadingMarginSpan.Standard(
              (int) getTextAttributes().mTextIndent.getValue(getStyle().getWidth()), 0)));
    }
    super.buildStyledSpan(start, end, ops);
    if (getTextAttributes().mFontColor == null) {
      ForegroundColorSpan foregroundColorSpan = new ForegroundColorSpan(Color.BLACK);
      configTextStroke(foregroundColorSpan);
      ops.add(new SetSpanOperation(start, end, foregroundColorSpan));
    }
    // try to download font if needed
    if (!TextUtils.isEmpty(getTextAttributes().mFontFamily)) {
      String fontFamily = getTextAttributes().mFontFamily;
      int style = getTypefaceStyle();
      Typeface typeface = TypefaceCache.getTypeface(getContext(), fontFamily, style);

      if (typeface == null) {
        FontFaceManager.getInstance().getTypeface(
            getContext(), fontFamily, style, new TextShadowNode.WeakTypefaceListener(this));
      } else {
        // for TextShadowNode, ues this flag to invalidate TextRenderKey
        getTextAttributes().setHasValidTypeface(true);
      }
    }
  }

  // To avoid break change.
  private MeasureParam mMeasureParam = null;
  private MeasureContext mMeasureContext = null;
  public long measure(
      LayoutNode node, float width, MeasureMode widthMode, float height, MeasureMode heightMode) {
    if (TraceEvent.enableTrace()) {
      Map<String, String> args = new HashMap<>();
      String string = "";
      if (mSpannableString != null) {
        string = mSpannableString.toString();
      }
      // The average word in the English language is approximately 5 characters.
      // 10 words may be enough to distinguish text.
      if (string.length() > 50) {
        args.put("first_fifty_characters", string.substring(0, 50));
      } else {
        args.put("characters", string);
      }
      TraceEvent.beginSection("text.TextShadowNode.measure", args);
    }
    mRenderer = null;
    mTruncatedSpannableString = null;
    // Check if it is worth to measure
    if (widthMode != MeasureMode.UNDEFINED && heightMode != MeasureMode.UNDEFINED && width == 0
        && height == 0) {
      TraceEvent.endSection("text.TextShadowNode.measure");
      return MeasureOutput.make(0, 0);
    }
    CharSequence span = mSpannableString;
    if (span == null) {
      TraceEvent.endSection("text.TextShadowNode.measure");
      return MeasureOutput.make(0, 0);
    }
    // inline view measure
    if (mMeasureContext != null && mMeasureParam != null) {
      measureNativeNode((SpannableStringBuilder) mSpannableString, mMeasureParam, mMeasureContext);
    }

    TextAttributes attributes = getTextAttributes().copy();
    TextRendererKey key = new TextRendererKey(span, attributes, widthMode, heightMode, width,
        height, mWordBreakStrategy, mEnableTailColorConvert, isTextRefactorEnabled(),
        isTextBoringLayoutEnabled());

    mRenderer = TextRendererCache.cache().getRenderer(getContext(), key);

    handleInlineTruncation(width, widthMode, height, heightMode);

    float measuredHeight = mRenderer.getTextLayoutHeight();
    float measuredWidth = mRenderer.getLayoutWidth();
    mBaseline = mRenderer.getTextLayout().getLineBaseline(0);

    TraceEvent.endSection("text.TextShadowNode.measure");
    return MeasureOutput.make(measuredWidth, measuredHeight);
  }

  private void handleInlineTruncation(
      float width, MeasureMode widthMode, float height, MeasureMode heightMode) {
    if (mTruncationShadowNode != null) {
      resetNativeNodeIndex(mTruncationShadowNode);
    }
    if (mTruncationSpannableString != null && mRenderer.isTextContentOverflow()
        && widthMode != MeasureMode.UNDEFINED) {
      TextRendererKey truncationKey =
          new TextRendererKey(mTruncationSpannableString, getTextAttributes().copy(),
              MeasureMode.AT_MOST, heightMode, width, height, mWordBreakStrategy,
              mEnableTailColorConvert, isTextRefactorEnabled(), isTextBoringLayoutEnabled());
      if (mMeasureContext != null && mMeasureParam != null) {
        mTruncationShadowNode.measureNativeNode(
            (SpannableStringBuilder) mTruncationSpannableString, mMeasureParam, mMeasureContext);
      }
      TextRenderer truncationTextRender =
          TextRendererCache.cache().getRenderer(getContext(), truncationKey);
      if (isTruncationWidthSmallerThanConstraintWidth(truncationTextRender.getTextLayout())) {
        int lastLineIndex = getTruncatedLastLineIndex(mRenderer, height, heightMode);
        if (mRenderer.getTextLayout().getLineEnd(lastLineIndex) == mSpannableString.length()
            && mRenderer.getTextLayout().getWidth() <= width) {
          return;
        }

        float truncationWidth = truncationTextRender.getLayoutWidth();
        int lastLineStartCharIndex = mRenderer.getTextLayout().getLineStart(lastLineIndex);
        int truncationPositionIndex = findTruncationPositionIndex(
            lastLineIndex, lastLineStartCharIndex, width, truncationWidth);

        while (truncationPositionIndex >= lastLineStartCharIndex) {
          // handle white space
          while (truncationPositionIndex > lastLineStartCharIndex
              && Character.isWhitespace(mSpannableString.charAt(truncationPositionIndex - 1))) {
            truncationPositionIndex--;
          }
          buildTextLayoutForTruncatedString(truncationPositionIndex, lastLineStartCharIndex, width,
              widthMode, height, heightMode);
          // Reduce truncationPositionIndex if text content overflow.The probability of needing to
          // be relayout is very small.
          if (isTextOverflowAfterTruncated(
                  mTruncatedSpannableString, mRenderer.getTextLayout(), lastLineIndex)
              && truncationPositionIndex > lastLineStartCharIndex) {
            truncationPositionIndex--;
            resetNativeNodeIndex(mTruncationShadowNode);
          } else {
            break;
          }
        };

        // update ellipsis count for bind layout event
        mEllipsisCount = mSpannableString.length() - truncationPositionIndex;
      }
    }
  }

  private void buildTextLayoutForTruncatedString(int truncationPositionIndex,
      int lastLineStartCharIndex, float width, MeasureMode widthMode, float height,
      MeasureMode heightMode) {
    CharSequence truncationLine = truncationPositionIndex <= lastLineStartCharIndex
        ? new SpannableStringBuilder()
        : mSpannableString.subSequence(lastLineStartCharIndex, truncationPositionIndex);

    int lengthExceptTruncation = lastLineStartCharIndex + truncationLine.length();
    updateNativeNodeIndex(lengthExceptTruncation, mTruncationShadowNode);
    mTruncatedSpannableString =
        ((SpannableStringBuilder) mSpannableString.subSequence(0, lastLineStartCharIndex))
            .append(truncationLine)
            .append(mTruncationSpannableString);
    updateInlineTextBackgroundSpanIndex(
        (Spanned) mTruncatedSpannableString, lengthExceptTruncation);

    getTextAttributes().mHasImageSpan |= mTruncationShadowNode.getTextAttributes().mHasImageSpan;
    getTextAttributes().mHasInlineViewSpan |=
        mTruncationShadowNode.getTextAttributes().mHasInlineViewSpan;
    TextRendererKey appendTruncationSpanKey = new TextRendererKey(mTruncatedSpannableString,
        getTextAttributes().copy(), widthMode, heightMode, width, height, mWordBreakStrategy,
        mEnableTailColorConvert, isTextRefactorEnabled(), isTextBoringLayoutEnabled());
    mRenderer = TextRendererCache.cache().getRenderer(getContext(), appendTruncationSpanKey);
  }

  private boolean isTextOverflowAfterTruncated(
      CharSequence truncatedString, Layout layout, int lastLineIndex) {
    return layout.getLineEnd(lastLineIndex) < truncatedString.length();
  }

  private boolean isTruncationWidthSmallerThanConstraintWidth(Layout truncationTextLayout) {
    return truncationTextLayout.getLineCount() == 1
        && truncationTextLayout.getLineEnd(0) == truncationTextLayout.getText().length();
  }

  private int getTruncatedLastLineIndex(
      TextRenderer textRenderer, float constraintHeight, MeasureMode constraintHeightMode) {
    int lastLineIndex = 0;
    if (constraintHeightMode != MeasureMode.UNDEFINED
        && textRenderer.getTextLayoutHeight() > constraintHeight) {
      Layout textLayout = textRenderer.getTextLayout();
      for (lastLineIndex = textLayout.getLineCount() - 1; lastLineIndex > 0; --lastLineIndex) {
        if (textLayout.getLineBottom(lastLineIndex) <= constraintHeight) {
          break;
        }
      }
    } else {
      lastLineIndex = textRenderer.getLineCount() - 1;
    }

    return lastLineIndex;
  }

  private void updateInlineTextBackgroundSpanIndex(
      Spanned truncatedSpannableString, int truncationStartIndex) {
    LynxTextBackgroundSpan[] backgroundSpans = truncatedSpannableString.getSpans(
        truncationStartIndex, truncatedSpannableString.length(), LynxTextBackgroundSpan.class);
    for (int i = 0; i < backgroundSpans.length; i++) {
      backgroundSpans[i].updateBackgroundStartEndIndex(truncationStartIndex);
    }

    backgroundSpans =
        truncatedSpannableString.getSpans(0, truncationStartIndex, LynxTextBackgroundSpan.class);
    for (int i = 0; i < backgroundSpans.length; i++) {
      backgroundSpans[i].updateBackgroundEndIndex(truncationStartIndex);
    }
  }

  /**
   * Return most suitable truncation position index in text.
   *     *********************
   *     *This is a long long* text.
   *     *********************
   *              *truncation*
   * As shown above, the single line text needs to be omitted.
   * Find the most suitable truncation position based on the truncation width.
   * "This is...truncation" is a suitable result.
   */
  private int findTruncationPositionIndex(
      int lastLineIndex, int lastLineStartCharIndex, float textMaxWidth, float truncationWidth) {
    int lastLineEndCharIndex = mRenderer.getTextLayout().getLineEnd(lastLineIndex);
    int truncationPositionIndex = lastLineEndCharIndex;
    float remainLastLineWidth = textMaxWidth - truncationWidth;
    if (mRenderer.getTextLayout().getLineMax(lastLineIndex)
            - mRenderer.getTextLayout().getLineLeft(lastLineIndex)
        <= remainLastLineWidth) {
      return truncationPositionIndex;
    }
    Map<Integer, Float> lastLineGlyphWidthMap = calculateLastLineGlyphWidth(
        lastLineIndex, lastLineStartCharIndex, lastLineEndCharIndex, mRenderer.getTextLayout());
    float sumOfGlyphWidth = 0.f;
    for (int i = lastLineStartCharIndex; i < lastLineEndCharIndex; i++) {
      if (lastLineGlyphWidthMap.containsKey(i)) {
        sumOfGlyphWidth += lastLineGlyphWidthMap.get(i);
        if (sumOfGlyphWidth > remainLastLineWidth) {
          truncationPositionIndex = i;
          break;
        }
      }
    }
    return truncationPositionIndex;
  }

  /**
   * Return last line glyph map. The key is character index,the value is glyph width.
   * @param lastLineIndex last line index in layout
   * @param lastLineStartCharIndex first char index in last line
   * @param lastLineEndCharIndex last char index in last line
   * @param layout layout result
   * @return
   */
  private Map<Integer, Float> calculateLastLineGlyphWidth(
      int lastLineIndex, int lastLineStartCharIndex, int lastLineEndCharIndex, Layout layout) {
    Map<Integer, Float> lastLineGlyphIndexWidthMap = new HashMap<>();
    ArrayList<Float> charPositionList = new ArrayList<>();
    charPositionList.add(layout.getLineLeft(lastLineIndex));
    charPositionList.add(layout.getLineRight(lastLineIndex));
    for (int i = lastLineStartCharIndex; i < lastLineEndCharIndex; i++) {
      if (!Character.isHighSurrogate(layout.getText().charAt(i))) {
        float charPosition = layout.getSecondaryHorizontal(i);
        charPositionList.add(charPosition);
        lastLineGlyphIndexWidthMap.put(i, charPosition);
      }
    }
    Collections.sort(charPositionList);
    for (Map.Entry<Integer, Float> entry : lastLineGlyphIndexWidthMap.entrySet()) {
      int charIndex = entry.getKey();
      float charInsertPosition = entry.getValue();
      int indexInOrderArray = Collections.binarySearch(charPositionList, charInsertPosition);
      if (layout.isRtlCharAt(charIndex)) {
        while (indexInOrderArray >= 0
            && charPositionList.get(indexInOrderArray) >= charInsertPosition) {
          indexInOrderArray--;
        }
      } else {
        while (indexInOrderArray < charPositionList.size()
            && charPositionList.get(indexInOrderArray) <= charInsertPosition) {
          indexInOrderArray++;
        }
      }
      if (indexInOrderArray >= 0 && indexInOrderArray < charPositionList.size()) {
        lastLineGlyphIndexWidthMap.put(
            charIndex, Math.abs(charInsertPosition - charPositionList.get(indexInOrderArray)));
      } else {
        // white space
        lastLineGlyphIndexWidthMap.put(charIndex, 0.f);
      }
    }
    return lastLineGlyphIndexWidthMap;
  }

  // update native node index in truncation before appending to the truncated string
  private void updateNativeNodeIndex(int lengthExceptTruncation, BaseTextShadowNode node) {
    for (int i = 0; i < node.getChildCount(); i++) {
      ShadowNode child = node.getChildAt(i);
      if (child instanceof NativeLayoutNodeRef) {
        ((NativeLayoutNodeRef) child).updateNativeNodeIndex(lengthExceptTruncation);
      } else if (child instanceof BaseTextShadowNode) {
        updateNativeNodeIndex(lengthExceptTruncation, (BaseTextShadowNode) child);
      }
    }
  }

  private void resetNativeNodeIndex(BaseTextShadowNode node) {
    for (int i = 0; i < node.getChildCount(); i++) {
      ShadowNode child = node.getChildAt(i);
      if (child instanceof NativeLayoutNodeRef) {
        ((NativeLayoutNodeRef) child).resetNativeNodeIndex();
      } else if (child instanceof BaseTextShadowNode) {
        resetNativeNodeIndex((BaseTextShadowNode) child);
      }
    }
  }

  public int getEllipsisCount() {
    return mEllipsisCount == 0 ? mRenderer.getEllipsisCount() : mEllipsisCount;
  }

  public boolean isBindEvent(String eventName) {
    return mEvents != null && mEvents.containsKey(eventName);
  }

  @Override
  public MeasureResult measure(MeasureParam param, @Nullable MeasureContext ctx) {
    mMeasureParam = param;
    mMeasureContext = ctx;
    mEllipsisCount = 0;
    // To avoid break change.
    long result = measure(this, param.mWidth, param.mWidthMode, param.mHeight, param.mHeightMode);
    float width = MeasureOutput.getWidth(result);
    float height = MeasureOutput.getHeight(result);
    return new MeasureResult(width, height, mBaseline);
  }

  @Override
  public void align(AlignParam param, AlignContext ctx) {
    if (mRenderer == null) {
      return;
    }

    alignNativeNode(mRenderer.getTextLayout(), getSpannableStringAfterMeasure(), param, ctx,
        mRenderer.getTextTranslateOffset());
  }

  private SpannableStringBuilder getSpannableStringAfterMeasure() {
    return (SpannableStringBuilder) (mTruncatedSpannableString == null ? mSpannableString
                                                                       : mTruncatedSpannableString);
  }

  protected boolean isLayoutEventContainTextSize() {
    return true;
  }

  @Nullable
  @Override
  public Object getExtraBundle() {
    if (mRenderer == null) {
      return null;
    }

    TextHelper.dispatchLayoutEvent(this);

    TextUpdateBundle bundle = createNewUpdateBundle();
    bundle.setSelectionColor(mSelectionColor);
    bundle.setTextTranslateOffset(mRenderer.getTextTranslateOffset());
    bundle.setNeedDrawStroke(mNeedDrawStroke);
    bundle.setOriginText(mSpannableString);
    mRenderer = null;
    return bundle;
  }

  protected TextUpdateBundle createNewUpdateBundle() {
    Set viewTruncated = null;
    if (getTextAttributes().hasInlineViewSpan() || mTruncationShadowNode != null) {
      // For inline view.
      viewTruncated = new HashSet();
      getNativeNodeTruncatedMap(mRenderer.getTextLayout().getText(), viewTruncated);
      if (mTruncationShadowNode != null && mEllipsisCount == 0) {
        // Find truncated view in truncation node.
        getTruncatedNativeNodeInTruncationShadowNode(mTruncationShadowNode, viewTruncated);
      }
    }

    return new TextUpdateBundle(mRenderer.getTextLayout(), getTextAttributes().mHasImageSpan,
        viewTruncated,
        mEnableFullJustify
            && getTextAttributes().getTextAlign() == StyleConstants.TEXTALIGN_JUSTIFY);
  }

  private void getTruncatedNativeNodeInTruncationShadowNode(
      BaseTextShadowNode node, Set viewTruncated) {
    for (int i = 0; i < node.getChildCount(); i++) {
      ShadowNode child = node.getChildAt(i);
      if (child instanceof NativeLayoutNodeRef) {
        viewTruncated.add(child.getSignature());
      } else if (child instanceof BaseTextShadowNode) {
        getTruncatedNativeNodeInTruncationShadowNode((BaseTextShadowNode) child, viewTruncated);
      }
    }
  }

  public int getSpannableStringLength() {
    return mSpannableString == null ? 0 : mSpannableString.length();
  }

  public @Nullable TextRenderer getTextRenderer() {
    return mRenderer;
  }

  static class WeakTypefaceListener implements TypefaceCache.TypefaceListener {
    private WeakReference<ShadowNode> mReference;

    WeakTypefaceListener(ShadowNode node) {
      this.mReference = new WeakReference<>(node);
    }

    @Override
    public void onTypefaceUpdate(Typeface typeface, int style) {
      ShadowNode node = mReference.get();
      if (node == null || node.isDestroyed()) {
        return;
      }
      if (node instanceof TextShadowNode) {
        // for TextShadowNode, ues this flag to invalidate TextRenderKey
        ((TextShadowNode) node).getTextAttributes().setHasValidTypeface(true);
      }
      node.markDirty();
    }
  }

  @LynxProp(name = "tail-color-convert")
  public void setEnableTailColorConvert(boolean enable) {
    mEnableTailColorConvert = enable;
    markDirty();
  }

  @LynxProp(name = "enable-full-justify")
  public void setEnableFullJustify(boolean enable) {
    if (mEnableFullJustify != enable) {
      markDirty();
      mEnableFullJustify = enable;
    }
  }

  @Override
  @LynxProp(name = PropsConstants.LINE_HEIGHT, defaultFloat = MeasureUtils.UNDEFINED)
  public void setLineHeight(float lineHeight) {
    setLineHeightInternal(lineHeight);
  }

  @LynxProp(name = PropsConstants.X_AUTO_FONT_SIZE)
  public void setAutoFontSize(ReadableArray autoFontSize) {
    getTextAttributes().setAutoFontSize(autoFontSize);
    markDirty();
  }

  @LynxProp(name = PropsConstants.X_AUTO_FONT_SIZE_PRESET_SIZES)
  public void setAutoFontSizePresetSizes(ReadableArray presetSizes) {
    getTextAttributes().setAutoFontSizePresetSizes(presetSizes);
  }

  @LynxProp(
      name = "text-single-line-vertical-align", defaultInt = StyleConstants.VERTICAL_ALIGN_DEFAULT)
  public void
  setVerticalTextAlign(String verticalTextAlign) {
    if ("center".equals(verticalTextAlign)) {
      getTextAttributes().mTextSingleLineVerticalAlign = StyleConstants.VERTICAL_ALIGN_CENTER;
    } else if ("top".equals(verticalTextAlign)) {
      getTextAttributes().mTextSingleLineVerticalAlign = StyleConstants.VERTICAL_ALIGN_TOP;
    } else if ("bottom".equals(verticalTextAlign)) {
      getTextAttributes().mTextSingleLineVerticalAlign = StyleConstants.VERTICAL_ALIGN_BOTTOM;
    } else {
      getTextAttributes().mTextSingleLineVerticalAlign = StyleConstants.VERTICAL_ALIGN_DEFAULT;
    }
    markDirty();
  }

  @LynxProp(name = PropsConstants.HYPHENS)
  public void setHyphen(int value) {
    getTextAttributes().setHyphen(value == StyleConstants.HYPHENS_AUTO);
    markDirty();
  }
}
