// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.shadow.text;

import static com.lynx.tasm.behavior.AutoGenStyleConstants.FONTSTYLE_ITALIC;
import static com.lynx.tasm.behavior.AutoGenStyleConstants.FONTSTYLE_OBLIQUE;
import static com.lynx.tasm.behavior.AutoGenStyleConstants.WHITESPACE_NOWRAP;
import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_CENTER;
import static com.lynx.tasm.behavior.StyleConstants.TEXTALIGN_RIGHT;
import static com.lynx.tasm.behavior.StyleConstants.TEXT_DECORATION_LINETHROUGH;
import static com.lynx.tasm.behavior.StyleConstants.TEXT_DECORATION_UNDERLINE;
import static java.util.Arrays.asList;

import android.graphics.Typeface;
import android.os.Build;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.TextUtils;
import android.text.style.AlignmentSpan;
import android.text.style.StyleSpan;
import android.util.Log;
import android.util.SparseArray;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.mapbuffer.CompactArrayBuffer;
import com.lynx.react.bridge.mapbuffer.ReadableCompactArrayBuffer;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureMode;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.behavior.shadow.ShadowStyle;
import com.lynx.tasm.behavior.ui.background.BackgroundLinearGradientLayer;
import com.lynx.tasm.behavior.ui.background.BackgroundRadialGradientLayer;
import com.lynx.tasm.behavior.ui.image.InlineImageSpan;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.behavior.ui.text.AbsInlineImageSpan;
import com.lynx.tasm.behavior.ui.utils.LynxBackground;
import com.lynx.tasm.behavior.utils.UnicodeFontUtils;
import com.lynx.tasm.fontface.FontFaceManager;
import com.lynx.tasm.utils.DeviceUtils;
import com.lynx.tasm.utils.PixelUtils;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class TextMeasurer {
  private final static int kPropInlineStart = 0; // value:inline str begin pos
  private final static int kPropInlineEnd = 1; // value: inline str end pos
  private final static int kPropTextString = 2; // text

  // styles
  private final static int kTextPropFontSize = 3;
  private final static int kTextPropColor = 4;
  private final static int kTextPropWhiteSpace = 5;
  private final static int kTextPropTextOverflow = 6;
  private final static int kTextPropFontWeight = 7;
  private final static int kTextPropFontStyle = 8;
  private final static int kTextPropFontFamily = 9;
  private final static int kTextPropLineHeight = 10;
  private final static int kTextPropLetterSpacing = 11;
  private final static int kTextPropLineSpacing = 12;
  private final static int kTextPropTextShadow = 13;
  private final static int kTextPropTextDecoration = 14;
  private final static int kTextPropTextAlign = 15;
  private final static int kTextPropVerticalAlign = 16;

  // attributes
  private final static int kTextPropTextMaxLine = 99;
  private final static int kTextPropBackGroundColor = 100;
  private final static int kPropImageSrc = 101; // image
  private final static int kPropInlineViewSign = 102;
  private final static int kPropRectSize = 103;
  private final static int kPropMargin = 104;

  private final static int kPropBorderRadius = 105;

  private final static int kPropColorLinearGradient = 106;
  private final static int kPropColorRadialGradient = 107;
  private final static int kPropColorConicGradient = 108;

  private final static int kTextPropEnd = 0xFF;

  private LynxContext mContext;
  private SparseArray<Object> mExtraDatas;
  private SparseArray<Object> mAttributedTextBundles;
  private long mNativePtr;
  public TextMeasurer(LynxContext context) {
    mContext = context;
    mExtraDatas = new SparseArray<>();
    mAttributedTextBundles = new SparseArray<>();
    mNativePtr = 0;
  }

  public void dispatchLayoutBefore(int sign, ReadableCompactArrayBuffer valueArray) {
    CharSequence span;
    boolean mHasImageSpan = false;
    List<BaseTextShadowNode.SetSpanOperation> ops = new ArrayList<>();
    SpannableStringBuilder spannableString = new SpannableStringBuilder();

    Iterator<CompactArrayBuffer.Entry> iterator = valueArray.iterator();
    String text;
    ShadowStyle shadowStyle = null;

    /**
     *  [inline-range-start,...k,v,k,v...,inline-range-end,
     * inline-range-start,..k,v,k,v..inline-range-end, 'para-start'...,k,v,...,textString]
     */
    boolean isParagraph = true;
    int start = 0, end = 0;
    int inlineViewSign = -1;
    HashMap<Integer, NativeLayoutNodeSpan> inlineViewMap = new HashMap<>();

    int[] margins = null;

    InlineImageProps inlineImageProps = null;
    int mWordBreakStyle = StyleConstants.WORDBREAK_NORMAL;
    boolean isSetVerticalAlign = false;
    ArrayList<AbsBaselineShiftCalculatorSpan> baselineShiftCalculatorSpans = new ArrayList<>();
    float maxFontSize = Math.round(PixelUtils.dipToPx(14, mContext.getScreenMetrics().density));

    TextAttributes textAttributes = null;
    while (iterator.hasNext()) {
      int operation = iterator.next().getInt();
      switch (operation) {
        case kPropInlineStart: // only inline node has the kPropRangeStart&kPropRangeEnd!
          start = iterator.next().getInt();
          isParagraph = false;
          shadowStyle = null;
          break;
        case kPropInlineEnd:
          end = iterator.next().getInt();
          if (inlineViewSign != -1) {
            buildNativeNodeSpan(start, end, ops, shadowStyle, baselineShiftCalculatorSpans,
                inlineViewMap, inlineViewSign);
          } else if (inlineImageProps != null) {
            // inline image
            buildImageStyledSpan(start, end, ops, inlineImageProps, textAttributes, shadowStyle,
                baselineShiftCalculatorSpans);
          } else {
            buildStyledSpanIfNeeded(
                start, end, ops, textAttributes, new TypefaceListener(sign, this), false);
          }

          // reset current span for inline node
          textAttributes = null;
          start = 0;
          isParagraph = true;
          inlineImageProps = null;
          inlineViewSign = -1;
          break;
        case kPropTextString:
          text = iterator.next().getString();
          textAttributes =
              ensureTextAttributes(textAttributes); // by default, we need a para textAttributes...

          // TODO(linxs): it's better to move the decode logic to C++ size
          int wordBreakStyle = UnicodeFontUtils.DECODE_DEFAULT;
          if (mWordBreakStyle == StyleConstants.WORDBREAK_BREAKALL) {
            wordBreakStyle = UnicodeFontUtils.DECODE_INSERT_ZERO_WIDTH_CHAR;
          } else if (mWordBreakStyle == StyleConstants.WORDBREAK_KEEPALL) {
            wordBreakStyle = UnicodeFontUtils.DECODE_CJK_INSERT_WORD_JOINER;
          }
          spannableString.append(UnicodeFontUtils.decode(text, wordBreakStyle));

          buildStyledSpanIfNeeded(
              0, text.length(), ops, textAttributes, new TypefaceListener(sign, this), true);
          break;
        case kTextPropFontSize:
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.setFontSize((float) iterator.next().getDouble());
          maxFontSize = Math.max(maxFontSize, textAttributes.mFontSize);
          break;
        case kTextPropFontFamily:
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.setFontFamily(iterator.next().getString());
          break;
        case kTextPropColor:
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.setFontColor(iterator.next().getInt());

          break;
        case kPropColorLinearGradient: {
          textAttributes = ensureTextAttributes(textAttributes);
          JavaOnlyArray array = new JavaOnlyArray();
          // [angle_float, colors_int_array, positions_float_array,side_or_corner_int]
          array.pushDouble(iterator.next().getDouble()); // angle
          int colorsCount = iterator.next().getInt(); // colors
          JavaOnlyArray colorsArray = new JavaOnlyArray();
          if (colorsCount > 0) {
            for (int i = 0; i < colorsCount; i++) {
              colorsArray.pushInt(iterator.next().getInt());
            }
          }
          array.pushArray(colorsArray);

          int positionsCount = iterator.next().getInt(); // positions
          JavaOnlyArray positionsArray = new JavaOnlyArray();
          if (positionsCount > 0) {
            for (int i = 0; i < positionsCount; i++) {
              positionsArray.pushDouble(iterator.next().getDouble());
            }
          }
          array.pushArray(positionsArray);

          array.pushInt(iterator.next().getInt()); // side_or_corner_int
          textAttributes.mTextGradient = new BackgroundLinearGradientLayer(array);
          break;
        }

        case kPropColorRadialGradient: {
          textAttributes = ensureTextAttributes(textAttributes);
          JavaOnlyArray array = new JavaOnlyArray();
          // radialGradientData:[shape_array, color_int_array, position_int_array]
          // shape_array:[shape_int, shape_size_int, pos_x_pattern_int, pos_x_value_double,
          // pos_y_pattern_int, pos_y_value_double, {size_x_value_double,size_x_pattern_int,
          // size_y_value_double,size_y_pattern_int}]

          // shape array(flat)
          JavaOnlyArray shapeArray = new JavaOnlyArray();
          {
            shapeArray.pushInt(iterator.next().getInt()); // shape: index0
            int shapeSize = iterator.next().getInt();
            shapeArray.pushInt(shapeSize); // shapeSize: index1
            shapeArray.pushInt(iterator.next().getInt()); // pos_x_pattern: index2
            shapeArray.pushDouble(iterator.next().getDouble()); // pos_x_value: index3
            shapeArray.pushInt(iterator.next().getInt()); // pos_y_pattern: index4
            shapeArray.pushDouble(iterator.next().getDouble()); // pos_y_value: index5

            // Take care: we need to put 4 useless data for array index:6,7,8,9
            for (int i = 0; i < 4; ++i) {
              shapeArray.pushInt(0);
            }

            if (shapeSize == 4) { // RADIAL_SIZE_LENGTH is 4
              shapeArray.pushDouble(iterator.next().getDouble()); // size_x_value: index10
              shapeArray.pushInt(iterator.next().getInt()); // size_x_pattern: index11
              shapeArray.pushDouble(iterator.next().getDouble()); // size_y_value: index12
              shapeArray.pushInt(iterator.next().getInt()); // size_y_pattern: index13
            }
          }

          array.pushArray(shapeArray);

          int colorsCount = iterator.next().getInt(); // colors
          JavaOnlyArray colorsArray = new JavaOnlyArray();
          if (colorsCount > 0) {
            for (int i = 0; i < colorsCount; i++) {
              colorsArray.pushInt(iterator.next().getInt());
            }
          }
          array.pushArray(colorsArray);

          int positionsCount = iterator.next().getInt(); // positions
          JavaOnlyArray positionsArray = new JavaOnlyArray();
          if (positionsCount > 0) {
            for (int i = 0; i < positionsCount; i++) {
              positionsArray.pushDouble(iterator.next().getDouble());
            }
          }
          array.pushArray(positionsArray);
          textAttributes.mTextGradient = new BackgroundRadialGradientLayer(array);
          break;
        }
        case kPropColorConicGradient:
          // TODO(linxs): tbd
          break;

        case kTextPropLineHeight:
          float lineHeight = (float) (iterator.next().getDouble());
          if (!isParagraph) {
            Log.w("TextMeasurer", "line-height should be set to paragraph");
            continue;
          }
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.setLineHeight(lineHeight);

          break;
        case kTextPropFontStyle:
          int fontStyle = iterator.next().getInt();
          if (fontStyle == FONTSTYLE_ITALIC || fontStyle == FONTSTYLE_OBLIQUE) {
            fontStyle = Typeface.ITALIC;
          } else {
            fontStyle = Typeface.NORMAL;
          }
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.setFontStyle(fontStyle);
          break;

        case kTextPropFontWeight:
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.setFontWeight(iterator.next().getInt());
          break;

        case kTextPropTextMaxLine:
          int maxLine = iterator.next().getInt();
          if (!isParagraph) {
            Log.w("TextMeasurer", "text-maxline should be set to paragraph");
            continue;
          }
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.mMaxLineCount = maxLine;
          break;

        case kTextPropWhiteSpace:
          int whitespace = iterator.next().getInt();
          if (!isParagraph) {
            Log.w("TextMeasurer", "white-space should be set to paragraph");
            continue;
          }
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.mWhiteSpace = whitespace;
          break;

        case kTextPropTextOverflow:
          int textOverflow = iterator.next().getInt();
          if (!isParagraph) {
            Log.w("TextMeasurer", "text-overflow should be set to paragraph");
            continue;
          }
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.mTextOverflow = textOverflow;
          break;

        case kTextPropLetterSpacing:
          double letterSpacing = iterator.next().getDouble();
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.mLetterSpacing = (float) letterSpacing;
          break;

        // inline -image
        case kPropImageSrc:
          inlineImageProps = new InlineImageProps();
          inlineImageProps.mSrc = iterator.next().getString();
          mHasImageSpan = true;
          break;

        case kTextPropVerticalAlign:
          shadowStyle = new ShadowStyle();
          shadowStyle.verticalAlign = iterator.next().getInt();
          shadowStyle.verticalAlignLength = (float) iterator.next().getDouble();
          isSetVerticalAlign = true;
          break;

        case kTextPropTextAlign:
          int textAlign = iterator.next().getInt();
          if (!isParagraph) {
            Log.w("TextMeasurer", "text-align should be set to paragraph");
            continue;
          }
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.mTextAlign = textAlign;
          break;

        case kPropRectSize:
          int width = iterator.next().getInt();
          int height = iterator.next().getInt();
          if (inlineImageProps != null) {
            inlineImageProps.mWidth = width;
            inlineImageProps.mHeight = height;
          }
          break;
        case kPropMargin:
          margins = new int[4];
          margins[0] = iterator.next().getInt();
          margins[1] = iterator.next().getInt();
          margins[2] = iterator.next().getInt();
          margins[3] = iterator.next().getInt();
          if (inlineImageProps != null) {
            inlineImageProps.mMargins = margins;
          }
          break;

        case kPropBorderRadius:
          double top_left = iterator.next().getDouble();
          int top_left_unit = iterator.next().getInt();

          double top_right = iterator.next().getDouble();
          int top_right_unit = iterator.next().getInt();

          double bottom_left = iterator.next().getDouble();
          int bottom_left_unit = iterator.next().getInt();

          double bottom_right = iterator.next().getDouble();
          int bottom_right_unit = iterator.next().getInt();

          if (inlineImageProps == null) {
            Log.w("TextMeasurer", "border-radius should be processed for inline image");
            continue;
          }

          LynxBackground background = new LynxBackground(mContext);
          // top_left_x
          JavaOnlyArray array = new JavaOnlyArray();
          array.pushDouble(top_left);
          array.pushInt(top_left_unit);
          // top_left_y
          array.pushDouble(top_left);
          array.pushInt(top_left_unit);

          // top_right_x
          array.pushDouble(top_right);
          array.pushInt(top_right_unit);
          // top_right_y
          array.pushDouble(top_right);
          array.pushInt(top_right_unit);

          // bottom_left_x
          array.pushDouble(bottom_left);
          array.pushInt(bottom_left_unit);
          // bottom_left_y
          array.pushDouble(bottom_left);
          array.pushInt(bottom_left_unit);

          // bottom_right_x
          array.pushDouble(bottom_right);
          array.pushInt(bottom_right_unit);
          // bottom_right_y
          array.pushDouble(bottom_right);
          array.pushInt(bottom_right_unit);

          background.setBorderRadius(0, array);
          inlineImageProps.mComplexBackground = background;
          break;
        case kPropInlineViewSign:
          inlineViewSign = iterator.next().getInt();
          break;

        case kTextPropTextDecoration:
          // type, style,color
          int textDecorationLine = iterator.next().getInt();
          int textDecorationStyle = iterator.next().getInt();
          int textDecorationColor = iterator.next().getInt();
          textAttributes = ensureTextAttributes(textAttributes);
          textAttributes.mTextDecoration = textDecorationLine;
          textAttributes.mTextDecorationStyle = textDecorationStyle;
          textAttributes.mTextDecorationColor = textDecorationColor;
          break;

        default:
          break;
      }
    }

    if (spannableString.length() == 0 && end > 0) {
      Log.e("TextMeasurer", "decode buffer error:" + valueArray.getString(valueArray.count() - 1));
    }

    // generate spannableString
    for (int i = ops.size() - 1; i >= 0; i--) {
      BaseTextShadowNode.SetSpanOperation op = ops.get(i);
      op.execute(spannableString);
    }

    span = spannableString;
    if (span == null || textAttributes == null) {
      return;
    }

    if (isSetVerticalAlign) {
      float minAscender = -maxFontSize * 1.2f * 0.78f;
      float maxDescender = maxFontSize * 1.2f * 0.22f;
      float maxXHeight = maxFontSize * 1.2f * 0.5f;
      float lineHeight =
          textAttributes.mLineHeight == MeasureUtils.UNDEFINED ? 0.f : textAttributes.mLineHeight;
      BaselineShiftCalculator baselineShiftCalculator =
          new BaselineShiftCalculator(asList(minAscender, maxDescender, maxXHeight, lineHeight));
      for (int i = 0; i < baselineShiftCalculatorSpans.size(); i++) {
        AbsBaselineShiftCalculatorSpan baselineShiftCalculatorSpan =
            baselineShiftCalculatorSpans.get(i);
        baselineShiftCalculatorSpan.setBaselineShiftCalculator(baselineShiftCalculator);
      }
    }

    textAttributes.setHasImageSpan(mHasImageSpan);
    textAttributes.setHasInlineViewSpan(!inlineViewMap.isEmpty());
    AttributedTextBundle attributedTextBundle = new AttributedTextBundle(span, textAttributes);
    if (!inlineViewMap.isEmpty()) {
      attributedTextBundle.setInlineViewMap(inlineViewMap);
    }
    mAttributedTextBundles.put(sign, attributedTextBundle);
  }

  public float[] measureText(int sign, float width, int widthMode, float height, int heightMode,
      float[] inlineViewLayoutResult) {
    AttributedTextBundle bundle = (AttributedTextBundle) mAttributedTextBundles.get(sign);
    if (bundle != null) {
      for (int i = 0; i < inlineViewLayoutResult.length / 4; i++) {
        int inlineViewSign = (int) inlineViewLayoutResult[i * 4];
        NativeLayoutNodeSpan span = bundle.getNativeLayoutNodeSpan(inlineViewSign);
        if (span != null) {
          span.updateLayoutNodeSize((int) Math.ceil(inlineViewLayoutResult[i * 4 + 1]),
              (int) Math.ceil(inlineViewLayoutResult[i * 4 + 2]),
              (int) Math.ceil(inlineViewLayoutResult[i * 4 + 3]));
        }
      }
    }

    float[] result = measureTextInternal(sign, width, MeasureMode.fromInt(widthMode), height,
        MeasureMode.fromInt(heightMode), new TextMeasurer.TypefaceListener(sign, this));

    return result;
  }

  TextAttributes ensureTextAttributes(TextAttributes attributes) {
    if (attributes == null) {
      attributes = buildTextAttributes();
    }
    return attributes;
  }

  private float[] measureTextInternal(int sign, float width, MeasureMode widthMode, float height,
      MeasureMode heightMode, TextMeasurer.TypefaceListener typefaceListener) {
    float[] result = new float[3];

    AttributedTextBundle attributedTextBundle =
        (AttributedTextBundle) mAttributedTextBundles.get(sign);
    if (attributedTextBundle == null) {
      return result;
    }

    TextRendererKey key = new TextRendererKey(attributedTextBundle.getSpan(),
        attributedTextBundle.getTextAttributes(), widthMode, heightMode, width, height, 0, false,
        true, true);
    TextRenderer renderer = new TextRenderer(mContext, key);
    float measuredHeight = renderer.getTextLayoutHeight();
    float measuredWidth = renderer.getLayoutWidth();

    int baseline = renderer.getTextLayout().getLineBaseline(0);
    result[0] = measuredWidth;
    result[1] = measuredHeight;
    result[2] = baseline;

    TextUpdateBundle bundle = new TextUpdateBundle(renderer.getTextLayout(),
        attributedTextBundle.getTextAttributes().hasImageSpan(), null,
        false
            && (attributedTextBundle.getTextAttributes() != null
                && attributedTextBundle.getTextAttributes().getTextAlign()
                    == StyleConstants.TEXTALIGN_JUSTIFY));
    bundle.setTextTranslateOffset(renderer.getTextTranslateOffset());

    bundle.setOriginText(attributedTextBundle.getSpan());

    if (bundle != null) {
      mExtraDatas.put(sign, bundle);
    }
    return result;
  }

  private boolean needSetLineHeightSpan(TextAttributes attributes) {
    return !MeasureUtils.isUndefined(attributes.mLineHeight)
        && !(attributes.getWhiteSpace() == WHITESPACE_NOWRAP && !attributes.hasImageSpan()
            && !attributes.hasInlineViewSpan());
  }

  private void buildStyledSpanIfNeeded(int start, int end,
      List<BaseTextShadowNode.SetSpanOperation> ops, TextAttributes attributes,
      TextMeasurer.TypefaceListener typefaceListener, boolean isParaAttr) {
    if (attributes == null) {
      return;
    }

    // para attributes which need span
    //  Set text line-height
    if (isParaAttr && needSetLineHeightSpan(attributes)) {
      ops.add(new BaseTextShadowNode.SetSpanOperation(
          start, end, new CustomLineHeightSpan(attributes.mLineHeight, true, 0, false)));
    }

    // Set letter spacing
    if (attributes.mLetterSpacing != MeasureUtils.UNDEFINED
        && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      ops.add(new BaseTextShadowNode.SetSpanOperation(
          start, end, new CustomLetterSpacingSpan(attributes.mLetterSpacing)));
    }

    // text decoration
    if (attributes.mTextDecorationStyle != 4 || attributes.mTextDecorationColor != 0) {
      boolean underline = ((attributes.mTextDecoration & TEXT_DECORATION_UNDERLINE) != 0);
      boolean through = ((attributes.mTextDecoration & TEXT_DECORATION_LINETHROUGH) != 0);
      if (underline || through) {
        ops.add(new BaseTextShadowNode.SetSpanOperation(start, end,
            new TextDecorationSpan(underline, through, attributes.mTextDecorationStyle,
                attributes.mTextDecorationColor)));
      }
    } else { // if not set the color and style, we will use default span.
      if ((attributes.mTextDecoration & TEXT_DECORATION_LINETHROUGH) != 0) {
        ops.add(new BaseTextShadowNode.SetSpanOperation(start, end, new LynxStrikethroughSpan()));
      }
      if ((attributes.mTextDecoration & TEXT_DECORATION_UNDERLINE) != 0) {
        ops.add(new BaseTextShadowNode.SetSpanOperation(start, end, new LynxUnderlineSpan()));
      }
    }

    if (attributes.mTextGradient != null) {
      ops.add(new BaseTextShadowNode.SetSpanOperation(
          start, end, new LynxTextGradientSpan(attributes.mTextGradient)));
    }

    if (isParaAttr) {
      return;
    }

    // paragraph attributes do not need to handle below attributes

    ops.add(new BaseTextShadowNode.SetSpanOperation(
        start, end, new AbsoluteSizeSpan(Math.round(attributes.mFontSize))));

    if (attributes.mFontColor != null) {
      ops.add(new BaseTextShadowNode.SetSpanOperation(
          start, end, new ForegroundColorSpan(attributes.mFontColor)));
    }

    // Set font family
    if (!TextUtils.isEmpty(attributes.mFontFamily)) {
      int style = 0;
      Typeface typeface = TypefaceCache.getTypeface(mContext, attributes.mFontFamily, style);
      if (typeface == null) {
        FontFaceManager.getInstance().getTypeface(
            mContext, attributes.mFontFamily, style, typefaceListener);
        // If typeface is null, avoid setting typeface, see TextHelper.newTextPaint
        typeface = DeviceUtils.getDefaultTypeface();
      }
      ops.add(new BaseTextShadowNode.SetSpanOperation(start, end, new FontFamilySpan(typeface)));
    }

    // Set text font weight and font style
    // FIXME(zhouzhuangzhuang): need to fix the inline node default attributes not working issue
    // later
    if (attributes.isFontWeightBOLD() || attributes.mFontStyle > Typeface.NORMAL) {
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
        ops.add(new BaseTextShadowNode.SetSpanOperation(start, end,
            new CustomStyleSpan(attributes.mFontStyle, attributes.mFontWeight,
                attributes.mFontFamily, attributes.getFontVariationSettings(),
                attributes.getFontFeatureSettings(), attributes.mHasValidTypeface)));
      } else {
        ops.add(new BaseTextShadowNode.SetSpanOperation(
            start, end, new StyleSpan(attributes.getTypefaceStyle())));
      }
    }

    // Set text alignment
    if (attributes.mTextAlign == TEXTALIGN_RIGHT) {
      ops.add(new BaseTextShadowNode.SetSpanOperation(
          start, end, new AlignmentSpan.Standard(Layout.Alignment.ALIGN_OPPOSITE)));
    } else if (attributes.mTextAlign == TEXTALIGN_CENTER) {
      ops.add(new BaseTextShadowNode.SetSpanOperation(
          start, end, new AlignmentSpan.Standard(Layout.Alignment.ALIGN_CENTER)));
    }
  }

  private void buildNativeNodeSpan(int start, int end,
      List<BaseTextShadowNode.SetSpanOperation> ops, ShadowStyle shadowStyle,
      ArrayList<AbsBaselineShiftCalculatorSpan> baselineShiftCalculatorSpans,
      HashMap<Integer, NativeLayoutNodeSpan> inlineViewMap, int inlineViewSign) {
    NativeLayoutNodeSpan nativeLayoutNodeSpan = new NativeLayoutNodeSpan();
    nativeLayoutNodeSpan.setEnableTextRefactor(true);
    if (shadowStyle != null) {
      nativeLayoutNodeSpan.setVerticalAlign(
          shadowStyle.verticalAlign, shadowStyle.verticalAlignLength);
    }

    baselineShiftCalculatorSpans.add(nativeLayoutNodeSpan);
    nativeLayoutNodeSpan.setSpanIndex(start);
    ops.add(new BaseTextShadowNode.SetSpanOperation(start, end, nativeLayoutNodeSpan));
    inlineViewMap.put(inlineViewSign, nativeLayoutNodeSpan);
  }

  private void buildImageStyledSpan(int start, int end,
      List<BaseTextShadowNode.SetSpanOperation> ops, InlineImageProps imageProps,
      TextAttributes attributes, ShadowStyle shadowStyle,
      ArrayList<AbsBaselineShiftCalculatorSpan> baselineShiftCalculatorSpans) {
    if (imageProps.mSrc == null) {
      return;
    }

    AbsInlineImageSpan imageSpan =
        new InlineImageSpan(imageProps.mWidth, imageProps.mHeight, imageProps.mMargins);
    imageSpan.setEnableTextRefactor(true);
    if (baselineShiftCalculatorSpans != null) {
      baselineShiftCalculatorSpans.add(imageSpan);
    }

    LynxImageManager lynxImageManager = new LynxImageManager(mContext) {
      @Override
      public void invalidate() {
        if (imageSpan.getCallback() != null) {
          imageSpan.getCallback().invalidateDrawable(getSrcImageDrawable());
        }
      }

      @Override
      protected void onImageLoadSuccess(int width, int height) {}

      @Override
      protected void onImageLoadError(LynxError error, int categorizedCode, int imageErrorCode) {}
    };
    ((InlineImageSpan) imageSpan).setImageManager(lynxImageManager);

    lynxImageManager.setSrc(imageProps.mSrc);
    if (imageProps.mMode != null) {
      lynxImageManager.setMode(imageProps.mMode);
    }

    if (shadowStyle != null) {
      imageSpan.setVerticalAlign(shadowStyle.verticalAlign, shadowStyle.verticalAlignLength);
    }

    if (imageProps.mComplexBackground != null
        && imageProps.mComplexBackground.getDrawable() != null) {
      imageProps.mComplexBackground.getDrawable().setBounds(
          0, 0, imageProps.mWidth, imageProps.mHeight);
      imageSpan.setComplexBackground(imageProps.mComplexBackground);
    }

    // trigger image request
    lynxImageManager.updateNodeProps();

    // TODO: background color support later if needed

    if (attributes != null) {
      // TBD
      imageSpan.setVerticalShift(attributes.mBaselineShift);
    }

    ops.add(new BaseTextShadowNode.SetSpanOperation(start, end, imageSpan));
  }

  private TextAttributes buildTextAttributes() {
    TextAttributes attr = new TextAttributes();
    attr.setFontSize(Math.round(PixelUtils.dipToPx(14, mContext.getScreenMetrics().density)));
    return attr;
  }

  public Object takeTextLayout(int sign) {
    return mExtraDatas.get(sign);
  }

  public void releaseLayoutObject(int sign) {
    mExtraDatas.remove(sign);
    mAttributedTextBundles.remove(sign);
  }

  public void removeLayoutObjects() {
    mExtraDatas.clear();
    mAttributedTextBundles.clear();
  }

  public float[] align(int sign) {
    AttributedTextBundle attributedBundle = (AttributedTextBundle) mAttributedTextBundles.get(sign);
    TextUpdateBundle extraData = (TextUpdateBundle) mExtraDatas.get(sign);
    if (attributedBundle == null || extraData == null) {
      return new float[0];
    }

    ArrayList<Float> alignResult = new ArrayList<>();
    Set<Map.Entry<Integer, NativeLayoutNodeSpan>> nativeLayoutNodeSpans =
        attributedBundle.getNativeLayoutNodeSpans();
    if (nativeLayoutNodeSpans == null) {
      return new float[0];
    }
    Layout layout = extraData.getTextLayout();
    HashSet viewTruncatedSet = new HashSet();
    for (Map.Entry<Integer, NativeLayoutNodeSpan> entry : nativeLayoutNodeSpans) {
      NativeLayoutNodeSpan span = entry.getValue();
      float leftOffset = 0.f, topOffset = 0.f;
      if (span.getSpanIndex() < layout.getText().length()) {
        int line = layout.getLineForOffset(span.getSpanIndex());
        leftOffset =
            layout.getPrimaryHorizontal(span.getSpanIndex()) + extraData.getTextTranslateOffset().x;
        if (layout.isRtlCharAt(span.getSpanIndex())) {
          leftOffset -= span.getWidth();
        }
        topOffset = span.getYOffset(layout.getLineTop(line), layout.getLineBottom(line),
                        layout.getLineAscent(line), layout.getLineDescent(line))
            + extraData.getTextTranslateOffset().y;
      }
      if (span.getSpanIndex() >= layout.getText().length()
          || layout.getText().charAt(span.getSpanIndex())
              != TextAttributes.INLINE_IMAGE_PLACEHOLDER.charAt(0)) {
        viewTruncatedSet.add(entry.getKey());
      }

      alignResult.add(Float.valueOf(entry.getKey()));
      alignResult.add(topOffset);
      alignResult.add(leftOffset);
    }
    extraData.setViewTruncatedSet(viewTruncatedSet);
    float[] res = new float[alignResult.size()];
    for (int i = 0; i < alignResult.size(); i++) {
      res[i] = alignResult.get(i);
    }

    return res;
  }

  public class TypefaceListener implements TypefaceCache.TypefaceListener {
    private int mSign;
    private WeakReference<TextMeasurer> mReference;

    TypefaceListener(int sign, TextMeasurer manager) {
      this.mSign = sign;
      this.mReference = new WeakReference<>(manager);
    }

    @Override
    public void onTypefaceUpdate(Typeface typeface, int style) {
      TextMeasurer textMeasurer = mReference.get();
      if (textMeasurer != null) {
        // TODO: we need a new api from LynxEngine to mark Element do requestLayout!
        //        uiowner.markLayoutNodeDirty(mSign);
      }
    }
  }

  private class InlineImageProps {
    public int mWidth;
    public int mHeight;
    public int[] mMargins = new int[4];
    public String mSrc;
    public String mMode;
    public LynxBackground mComplexBackground;
  }
}
