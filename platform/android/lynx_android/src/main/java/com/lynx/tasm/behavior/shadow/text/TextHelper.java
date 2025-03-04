// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.shadow.text;

import static android.text.Layout.DIR_RIGHT_TO_LEFT;

import android.graphics.Canvas;
import android.graphics.DashPathEffect;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.os.Build;
import android.text.Layout;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.StaticLayout;
import android.text.TextDirectionHeuristics;
import android.text.TextPaint;
import android.text.TextUtils;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.Dynamic;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.react.bridge.ReadableType;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.MeasureUtils;
import com.lynx.tasm.event.LynxDetailEvent;
import com.lynx.tasm.utils.DeviceUtils;
import com.lynx.tasm.utils.PixelUtils;
import com.lynx.tasm.utils.UIThreadUtils;
import com.lynx.tasm.utils.UnitUtils;
import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class TextHelper {
  static final String TAG = "TextHelper";
  static final String EVENT_LAYOUT = "layout";
  public static TextPaint newTextPaint(
      LynxContext context, TextAttributes attributes, TypefaceCache.TypefaceListener listener) {
    Typeface typefaceCache = getTypeFaceFromCache(context, attributes, listener);
    return newTextPaint(attributes, typefaceCache);
  }

  public static TextPaint newTextPaint(TextAttributes attributes, Typeface typefaceCache) {
    TextPaint textPaint = new TextPaint(TextPaint.ANTI_ALIAS_FLAG);
    textPaint.setTextSize(attributes.mFontSize);
    String fontFamily = attributes.mFontFamily;
    if (!TextUtils.isEmpty(fontFamily) && typefaceCache != null) {
      textPaint.setTypeface(typefaceCache);
    } else {
      textPaint.setTypeface(DeviceUtils.getDefaultTypeface());
    }

    if (attributes.mLetterSpacing != MeasureUtils.UNDEFINED
        && Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
      textPaint.setLetterSpacing(attributes.mLetterSpacing / textPaint.getTextSize());
    }
    if (null != attributes.mTextShadow) {
      textPaint.setShadowLayer(attributes.mTextShadow.blurRadius, attributes.mTextShadow.offsetX,
          attributes.mTextShadow.offsetY, attributes.mTextShadow.color);
    }
    return textPaint;
  }

  public static Typeface getTypeFaceFromCache(
      LynxContext context, TextAttributes attributes, TypefaceCache.TypefaceListener listener) {
    String fontFamily = attributes.mFontFamily;
    if (!TextUtils.isEmpty(fontFamily)) {
      return TypefaceCache.getTypeface(context, fontFamily, attributes.getTypefaceStyle());
    } else {
      return null;
    }
  }

  // according to the CSS Property text-decoration, draw line.
  public static void drawLine(Canvas canvas, Layout textLayout) {
    Spanned spannableString = (Spanned) textLayout.getText();
    TextDecorationSpan[] textDecorationSpans =
        spannableString.getSpans(0, spannableString.length(), TextDecorationSpan.class);

    // if text-decoration doesn't set color and style, textDecorationSpans is empty
    if (textDecorationSpans == null || textDecorationSpans.length == 0) {
      return;
    }

    int start = spannableString.getSpanStart(textDecorationSpans[0]);
    int end = spannableString.getSpanEnd(textDecorationSpans[0]);
    int size = (int) textLayout.getPaint().getTextSize();
    if (start == 0) { // text has inline text and set text-decoration for all inline text
      for (int i = 1; i < textDecorationSpans.length; i++) {
        int curStart = spannableString.getSpanStart(textDecorationSpans[i]);
        int curEnd = spannableString.getSpanEnd(textDecorationSpans[i]);
        AbsoluteSizeSpan[] absoluteSizeSpan =
            spannableString.getSpans(curStart, curEnd, AbsoluteSizeSpan.class);
        int absoluteSize = size;
        if (absoluteSizeSpan.length != 0) {
          absoluteSize = absoluteSizeSpan[0].getSize();
        }
        draw(canvas, textLayout, textDecorationSpans[i].mTextDecorationStyle,
            textDecorationSpans[i].mTextDecorationColor, textDecorationSpans[i].mUnderline,
            textDecorationSpans[i].mLineThrough, curStart, curEnd, absoluteSize);

        // partition [start, end] to ( [start, curStart], [curStart, curEnd] and [curEnd + 1, end] )
        if (curStart >= start && curEnd <= end) {
          draw(canvas, textLayout, textDecorationSpans[0].mTextDecorationStyle,
              textDecorationSpans[0].mTextDecorationColor, textDecorationSpans[0].mUnderline,
              textDecorationSpans[0].mLineThrough, curEnd + 1, end, size);
          end = curStart;
          boolean underline = false, lineThrough = false;
          if (textDecorationSpans[0].mUnderline && !textDecorationSpans[i].mUnderline) {
            underline = true;
          }
          if (textDecorationSpans[0].mLineThrough && !textDecorationSpans[i].mLineThrough) {
            lineThrough = true;
          }
          draw(canvas, textLayout, textDecorationSpans[0].mTextDecorationStyle,
              textDecorationSpans[0].mTextDecorationColor, underline, lineThrough, curStart, curEnd,
              absoluteSize);
        }
      }
      draw(canvas, textLayout, textDecorationSpans[0].mTextDecorationStyle,
          textDecorationSpans[0].mTextDecorationColor, textDecorationSpans[0].mUnderline,
          textDecorationSpans[0].mLineThrough, start, end, size);
    } else { // text has inline text but not set text-decoration for all inline text
      for (int i = 0; i < textDecorationSpans.length; i++) {
        int curStart = spannableString.getSpanStart(textDecorationSpans[i]);
        int curEnd = spannableString.getSpanEnd(textDecorationSpans[i]);
        AbsoluteSizeSpan[] absoluteSizeSpans =
            spannableString.getSpans(curStart, curEnd, AbsoluteSizeSpan.class);
        int absoluteSize = size;
        if (absoluteSizeSpans.length != 0) {
          absoluteSize = absoluteSizeSpans[0].getSize();
        }
        draw(canvas, textLayout, textDecorationSpans[i].mTextDecorationStyle,
            textDecorationSpans[i].mTextDecorationColor, textDecorationSpans[i].mUnderline,
            textDecorationSpans[i].mLineThrough, curStart, curEnd, absoluteSize);
      }
    }
  }

  public static void draw(Canvas canvas, Layout textLayout, int textDecorationStyle,
      int textDecorationColor, boolean underline, boolean lineThrough, int start, int end,
      int absoluteSize) {
    if (start >= end || (!underline && !lineThrough)) {
      return;
    }
    switch (textDecorationStyle) {
      case 4: // solid
        drawSolid(canvas, textLayout, textDecorationColor, underline, lineThrough, start, end,
            absoluteSize);
        break;
      case 8: // double
        drawDouble(canvas, textLayout, textDecorationColor, underline, lineThrough, start, end,
            absoluteSize);
        break;
      case 16: // dotted
        drawDotted(canvas, textLayout, textDecorationColor, underline, lineThrough, start, end,
            absoluteSize);
        break;
      case 32: // dashed
        drawDash(canvas, textLayout, textDecorationColor, underline, lineThrough, start, end,
            absoluteSize);
        break;
      case 64: // wavy
        drawWavy(canvas, textLayout, textDecorationColor, underline, lineThrough, start, end,
            absoluteSize);
        break;
      default:
        break;
    }
  }

  public static void drawSolid(Canvas canvas, Layout textLayout, int color, boolean underline,
      boolean lineThrough, int start, int end, float textSize) {
    Paint solidPaint = new Paint();
    solidPaint.setColor(color);
    float lineWidth = textSize / 3;
    // lineWidth = textSize / 3, setStrokeWidth = lineWidth / 5 is close to the width in Web
    solidPaint.setStrokeWidth(lineWidth / 5);
    int startLine = textLayout.getLineForOffset(start);
    int endLine = textLayout.getLineForOffset(end);
    for (int i = startLine; i <= endLine; i++) {
      float x = textLayout.getLineLeft(i);
      float y = textLayout.getLineBaseline(i);
      float width = textLayout.getLineMax(i);
      if (i == startLine) {
        width = x + width - textLayout.getPrimaryHorizontal(start);
        x = textLayout.getPrimaryHorizontal(start); // find the start position of text
      }
      if (i == endLine) {
        width = textLayout.getPrimaryHorizontal(end) - x; // find the end position of text
      }
      if (underline) {
        canvas.drawLine(x, y + lineWidth / 3, x + width, y + lineWidth / 3, solidPaint);
      }
      if (lineThrough) {
        y -= textSize / 15 * 4;
        // line-through should draw a line in the middle of a character, (baseline -
        // textSize / 15 * 4) is a proper position to draw line, which is close to the
        // position in Web, other computations are similar
        canvas.drawLine(x, y, x + width, y, solidPaint);
      }
    }
  }

  public static void drawDouble(Canvas canvas, Layout textLayout, int color, boolean underline,
      boolean lineThrough, int start, int end, float textSize) {
    Paint doublePaint = new Paint();
    doublePaint.setColor(color);
    float lineWidth = textSize / 3;
    int startLine = textLayout.getLineForOffset(start);
    int endLine = textLayout.getLineForOffset(end);
    doublePaint.setStrokeWidth(lineWidth / 5);
    for (int i = startLine; i <= endLine; i++) {
      float x = textLayout.getLineLeft(i);
      float y = textLayout.getLineBaseline(i);
      float width = textLayout.getLineMax(i);
      if (i == startLine) {
        width = x + width - textLayout.getPrimaryHorizontal(start);
        x = textLayout.getPrimaryHorizontal(start); // find the start position of text
      }
      if (i == endLine) {
        width = textLayout.getPrimaryHorizontal(end) - x; // find the end position of text
      }
      if (underline) {
        canvas.drawLine(x, y + lineWidth / 3, x + width, y + lineWidth / 3, doublePaint);
        canvas.drawLine(x, y + lineWidth / 5 * 3, x + width, y + lineWidth / 5 * 3,
            doublePaint); // set the proper distance between two line
      }
      if (lineThrough) {
        y -= textSize / 15 * 4;
        canvas.drawLine(x, y, x + width, y, doublePaint);
        canvas.drawLine(x, y + lineWidth / 15 * 4, x + width, y + lineWidth / 15 * 4, doublePaint);
      }
    }
  }

  public static void drawDotted(Canvas canvas, Layout textLayout, int color, boolean underline,
      boolean lineThrough, int start, int end, float textSize) {
    Paint dottedPaint = new Paint();
    dottedPaint.setColor(color);
    float lineWidth = textSize / 3;
    float space = textSize / 4; // textSize / 4 is a proper distance between two dot
    int startLine = textLayout.getLineForOffset(start);
    int endLine = textLayout.getLineForOffset(end);
    dottedPaint.setStrokeWidth(lineWidth / 5);
    for (int i = startLine; i <= endLine; i++) {
      float x = textLayout.getLineLeft(i);
      float y = textLayout.getLineBaseline(i);
      float width = textLayout.getLineMax(i);
      if (i == startLine) {
        width = x + width - textLayout.getPrimaryHorizontal(start);
        x = textLayout.getPrimaryHorizontal(start); // find the start position of text
      }
      if (i == endLine) {
        width = textLayout.getPrimaryHorizontal(end) - x; // find the end position of text
      }
      int number = (int) Math.floor(width / space); // Calculate how many points can be drawn
      if (number == 0) { // avoid some point
        return;
      }
      if (underline) {
        float xOffset = 0;
        for (int j = 0; j < number + 1; j++) {
          canvas.drawPoint(x + xOffset, y + lineWidth / 3, dottedPaint);
          xOffset += space;
        }
      }
      if (lineThrough) {
        y -= textSize / 15 * 4;
        float xOffset = 0;
        for (int j = 0; j < number + 1; j++) {
          canvas.drawPoint(x + xOffset, y, dottedPaint);
          xOffset += space;
        }
      }
    }
  }

  public static void drawDash(Canvas canvas, Layout textLayout, int color, boolean underline,
      boolean lineThrough, int start, int end, float textSize) {
    float dashWidth = textSize / 7; // the width of a dash
    float dashSpaceWidth = textSize / 20; // the distance between two dashes
    int startLine = textLayout.getLineForOffset(start);
    int endLine = textLayout.getLineForOffset(end);
    Paint dashPaint = new Paint();
    dashPaint.setColor(color);
    float lineWidth = textSize / 3;
    dashPaint.setStrokeWidth(lineWidth / 5);
    dashPaint.setPathEffect(new DashPathEffect(new float[] {dashWidth, dashSpaceWidth}, 0));
    for (int i = startLine; i <= endLine; i++) {
      float x = textLayout.getLineLeft(i);
      float y = textLayout.getLineBaseline(i);
      float width = textLayout.getLineMax(i);
      if (i == startLine) {
        width = x + width - textLayout.getPrimaryHorizontal(start);
        x = textLayout.getPrimaryHorizontal(start); // find the start position of text
      }
      if (i == endLine) {
        width = textLayout.getPrimaryHorizontal(end) - x; // find the end position of text
      }
      if (underline) {
        canvas.drawLine(x, y + lineWidth / 3, x + width, y + lineWidth / 3, dashPaint);
      }
      if (lineThrough) {
        y -= textSize / 15 * 4;
        canvas.drawLine(x, y, x + width, y, dashPaint);
      }
    }
  }

  public static void drawWavy(Canvas canvas, Layout textLayout, int color, boolean underline,
      boolean lineThrough, int start, int end, float textSize) {
    // the width of a complete wave
    float wavyWidth = textSize / 2;
    // the distance between the highest position and the lowest position in a complete wave
    float wavyHeight = textSize / 3;
    int startLine = textLayout.getLineForOffset(start);
    int endLine = textLayout.getLineForOffset(end);
    Paint wavyPaint = new Paint();
    wavyPaint.setStyle(Paint.Style.STROKE);
    wavyPaint.setColor(color);
    wavyPaint.setStrokeWidth(wavyHeight / 5);
    for (int i = startLine; i <= endLine; i++) {
      float x = textLayout.getLineLeft(i);
      float y = textLayout.getLineBaseline(i);
      float width = textLayout.getLineMax(i);
      if (i == startLine) {
        width = x + width - textLayout.getPrimaryHorizontal(start);
        x = textLayout.getPrimaryHorizontal(start); // find the start position of text
      }
      if (i == endLine) {
        width = textLayout.getPrimaryHorizontal(end) - x; // find the end position of text
      }
      int number = Math.round(width / wavyWidth); // Calculate how many wave can be drawn
      if (underline) {
        Path path = new Path();
        path.moveTo(x, y + wavyHeight / 2);
        float xOffset = 0;
        for (int j = 0; j < number; j++) {
          path.quadTo(x + wavyWidth / 4 + xOffset, y, x + wavyWidth / 2 + xOffset,
              y + wavyHeight / 2); // first draw an upper half circle
          path.quadTo(x + wavyWidth / 4 * 3 + xOffset, y + wavyHeight, x + wavyWidth + xOffset,
              y + wavyHeight / 2); // then draw a lower half circle
          xOffset += wavyWidth;
        }
        canvas.drawPath(path, wavyPaint);
      }
      if (lineThrough) {
        Path path = new Path();
        y -= textSize / 15 * 4;
        path.moveTo(x, y);
        float xOffset = 0;
        for (int j = 0; j < number; j++) {
          path.quadTo(
              x + wavyWidth / 4 + xOffset, y - wavyHeight / 2, x + wavyWidth / 2 + xOffset, y);
          path.quadTo(
              x + wavyWidth / 4 * 3 + xOffset, y + wavyHeight / 2, x + wavyWidth + xOffset, y);
          xOffset += wavyWidth;
        }
        canvas.drawPath(path, wavyPaint);
      }
    }
  }

  public static void drawTextStroke(Layout textLayout, Canvas canvas) {
    Spanned spannableString = (Spanned) textLayout.getText();
    ForegroundColorSpan[] colorSpans =
        spannableString.getSpans(0, spannableString.length(), ForegroundColorSpan.class);
    if (colorSpans != null && colorSpans.length > 0) {
      for (ForegroundColorSpan span : colorSpans) {
        span.setDrawStroke(true);
      }
      textLayout.draw(canvas);
      for (ForegroundColorSpan span : colorSpans) {
        span.setDrawStroke(false);
      }
    }
  }

  public static void drawText(Canvas canvas, Layout textLayout, float width) {
    int lineCount = textLayout.getLineCount();
    ArrayList<CharSequence>[] wordsOfLines = new ArrayList[lineCount];
    ArrayList<Float>[] wordsPositionOfLines = new ArrayList[lineCount];
    boolean isCaught = false;
    try {
      CharSequence text = textLayout.getText();
      for (int i = 0; i < lineCount; i++) {
        float x = 0;
        int start = textLayout.getLineStart(i);
        int end = textLayout.getLineEnd(i);
        CharSequence line = text.subSequence(start, end);
        if (i != lineCount - 1) {
          ArrayList<CharSequence> words =
              splitLineToWords(line, textLayout.getParagraphDirection(i) == DIR_RIGHT_TO_LEFT);
          wordsOfLines[i] = words;
          ArrayList<Float> wordsPosition = new ArrayList<>();

          int spaceCount = words.size() - 1;
          float sumWordWidth = 0.f;
          float[] wordWidths = new float[words.size()];
          for (int j = 0; j < words.size(); j++) {
            float wordWidth = Layout.getDesiredWidth(words.get(j), textLayout.getPaint());
            wordWidths[j] = wordWidth;
            sumWordWidth += wordWidth;
          }
          float spaceWidth =
              spaceCount == 0 ? width - sumWordWidth : (width - sumWordWidth) / spaceCount;
          if (spaceCount == 0 && textLayout.getParagraphDirection(i) == DIR_RIGHT_TO_LEFT) {
            wordsPosition.add(width - sumWordWidth);
          } else {
            wordsPosition.add(0.f);
          }
          for (int j = 1; j < words.size(); j++) {
            x = x + wordWidths[j - 1] + spaceWidth;
            wordsPosition.add(x);
          }

          wordsPositionOfLines[i] = wordsPosition;
        } else {
          float lastLineWidth = Layout.getDesiredWidth(line, textLayout.getPaint());
          ArrayList<CharSequence> lastLine = new ArrayList<>();
          lastLine.add(line);
          wordsOfLines[i] = lastLine;
          ArrayList<Float> lastLinePosition = new ArrayList<>();
          lastLinePosition.add(textLayout.getParagraphDirection(i) == DIR_RIGHT_TO_LEFT
                  ? width - lastLineWidth
                  : 0.f);
          wordsPositionOfLines[i] = lastLinePosition;
        }
      }

    } catch (Exception e) {
      LLog.e("TextHelper", "draw justify text error:" + e.toString());
      isCaught = true;
      // fallback to use textLayout.draw(canvas)
      textLayout.draw(canvas);
    }
    if (!isCaught) {
      for (int i = 0; i < wordsOfLines.length; i++) {
        float y = textLayout.getLineBaseline(i);
        ArrayList<CharSequence> words = wordsOfLines[i];
        ArrayList<Float> wordsPosition = wordsPositionOfLines[i];
        for (int j = 0; j < words.size(); j++) {
          canvas.drawText(words.get(j).toString(), wordsPosition.get(j), y, textLayout.getPaint());
        }
      }
    }
  }

  public static ArrayList<CharSequence> splitLineToWords(CharSequence line, boolean isRtl) {
    int index = 0;
    int lastWordEndIndex = 0;
    ArrayList<CharSequence> words = new ArrayList<>();
    while (index < line.length()) {
      char c = line.charAt(index);

      Pattern pattern = null;
      if (c >= '\u4e00' && c <= '\u9fa5') {
        // Chinese and punctuation
        pattern = Pattern.compile("[\u4e00-\u9fa5]["
            + "\\u3002\\uff1f\\uff01\\uff0c\\u3001\\uff1b\\uff1a\\u2018\\u2019\\u201c\\u201d\\uff08"
            + "\\uff09\\u3014\\u3015\\u3010\\u3011\\u2026\\u2014\\p{Punct}]*");
      } else if (Character.isLetter(c)) {
        // English and punctuation
        pattern = Pattern.compile("\\w+\\p{Punct}*");
      } else if (Character.isDigit(c)) {
        // number
        pattern = Pattern.compile("\\d+\\.\\d+");
      } else if (Character.isSpaceChar(c)) {
        pattern = Pattern.compile(("\\s*"));
      }
      if (pattern != null) {
        Matcher matcher = pattern.matcher(line.subSequence(index, line.length()));
        if (matcher.find()) {
          int endIndex = matcher.end();
          words.add(line.subSequence(lastWordEndIndex, index + endIndex));
          lastWordEndIndex = index = index + endIndex;
        } else {
          words.add(line.subSequence(index, index + 1));
          lastWordEndIndex = index = index + 1;
        }
      } else if (Character.isHighSurrogate(c)) {
        words.add(line.subSequence(index, index + 2));
        lastWordEndIndex = index = index + 2;
      } else {
        index++;
      }
    }
    if (lastWordEndIndex != index) {
      words.add(line.subSequence(lastWordEndIndex, index));
    }
    // romove last space word
    int lastWordIndex = isRtl ? 0 : words.size() - 1;
    boolean isWhiteSpace = true;
    CharSequence lastWord = words.get(lastWordIndex);
    for (int i = 0; i < lastWord.length(); i++) {
      char c = lastWord.charAt(i);
      if (!Character.isWhitespace(c)) {
        isWhiteSpace = false;
        break;
      }
    }
    if (isWhiteSpace) {
      words.remove(lastWordIndex);
    }

    return words;
  }

  /*
   {
     lineCount: number;    //line count of display text
     lines: LineInfo[];    //contain line layout info
     size: {width: number, height: number}; //text layout rect
   }
   class LineInfo {
     start: number;        //the line start offset for text
     end: number;          //the line end offset for text
     ellipsisCount: number;//ellipsis count of the line. If larger than 0, truncate text in this
   line.
   }
  */
  public static LynxDetailEvent getTextLayoutEvent(int sign, Layout layout, int textOverflow,
      int lineCount, int ellipsisCount, int spannableStringLength, float textLayoutWidth,
      boolean containTextSize) {
    LynxDetailEvent event = new LynxDetailEvent(sign, EVENT_LAYOUT);

    // lineCount
    event.addDetail("lineCount", lineCount);
    if (lineCount > layout.getLineCount() || lineCount == 0) {
      LLog.e(TAG, "getTextLayoutEvent: get lineCount error");
      event.addDetail("lineCount", 0);
      return event;
    }
    // lines
    ArrayList<HashMap<String, Integer>> lineInfo = new ArrayList<>();
    for (int i = 0; i < lineCount; i++) {
      HashMap<String, Integer> info = new HashMap<>();
      info.put("start", layout.getLineStart(i));
      info.put("end", layout.getLineEnd(i));
      info.put("ellipsisCount", layout.getEllipsisCount(i));

      lineInfo.add(info);
    }
    HashMap<String, Integer> lastLine = lineInfo.get(lineCount - 1);
    int lastLineEllipsisCount = lastLine.get("ellipsisCount");
    int lastLineEnd = lastLine.get("end");

    if (ellipsisCount > 0) {
      // truncated or ellipsis being replaced
      lastLineEllipsisCount = ellipsisCount;
    } else if (lineCount < layout.getLineCount()
        || textOverflow == StyleConstants.TEXTOVERFLOW_CLIP) {
      // clip
      lastLineEllipsisCount = spannableStringLength - lastLineEnd;
    }

    lastLine.put("ellipsisCount", lastLineEllipsisCount);
    lastLine.put("end", spannableStringLength);
    event.addDetail("lines", lineInfo);

    if (containTextSize) {
      HashMap<String, Float> size = new HashMap<>();
      size.put("width", PixelUtils.pxToDip(textLayoutWidth));
      size.put("height", PixelUtils.pxToDip(layout.getLineBottom(lineCount - 1)));
      event.addDetail("size", size);
    }

    return event;
  }

  public static void dispatchLayoutEvent(TextShadowNode textShadowNode) {
    if (!textShadowNode.isBindEvent(EVENT_LAYOUT) || textShadowNode.getTextRenderer() == null
        || textShadowNode.getTextRenderer().getTextLayout() == null) {
      return;
    }

    LynxDetailEvent event = getTextLayoutEvent(textShadowNode.getSignature(),
        textShadowNode.getTextRenderer().getTextLayout(),
        textShadowNode.getTextAttributes().getTextOverflow(),
        textShadowNode.getTextRenderer().getLineCount(), textShadowNode.getEllipsisCount(),
        textShadowNode.getSpannableStringLength(),
        textShadowNode.getTextRenderer().calculateMaxWidth(),
        textShadowNode.isLayoutEventContainTextSize());
    UIThreadUtils.runOnUiThread(new Runnable() {
      @Override
      public void run() {
        textShadowNode.getContext().getEventEmitter().sendCustomEvent(event);
      }
    });
  }

  public static TextPaint newTextPaint(float fontSize, String fontFamily) {
    TextPaint textPaint = new TextPaint(TextPaint.ANTI_ALIAS_FLAG);
    textPaint.setTextSize(fontSize);
    Typeface customTypeface = null;
    if (!TextUtils.isEmpty(fontFamily)) {
      customTypeface = TypefaceCache.getCachedTypeface(fontFamily, Typeface.NORMAL);
    }
    if (customTypeface != null) {
      textPaint.setTypeface(customTypeface);
    } else {
      textPaint.setTypeface(DeviceUtils.getDefaultTypeface());
    }

    return textPaint;
  }

  public static double getTextWidth(String text, String fontSize, @Nullable String fontFamily) {
    int textFontSize = Math.round(UnitUtils.toPx(fontSize, 0, 0, 0, 0, PixelUtils.dipToPx(14)));
    if (textFontSize == 0) {
      return 0;
    }
    Paint paint = newTextPaint(textFontSize, fontFamily);
    float result = paint.measureText(text);
    float width = PixelUtils.pxToDip(result);
    return width;
  }

  public static String getFirstLineText(
      String text, String fontSize, @Nullable String fontFamily, String maxWidth) {
    int textFontSize = Math.round(UnitUtils.toPx(fontSize, 0, 0, 0, 0, PixelUtils.dipToPx(14)));
    float maxMeasureWidth = TextUtils.isEmpty(maxWidth)
        ? 0
        : UnitUtils.toPx(maxWidth, 0, 0, 0, 0, PixelUtils.dipToPx(0));
    if (textFontSize == 0 || maxMeasureWidth < 1) {
      return "";
    }
    CharSequence textSpan = text;
    TextPaint textPaint = newTextPaint(textFontSize, fontFamily);

    int maxMeasureLine = 1;
    Layout textLayout;
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      // use StaticLayout#Builder
      StaticLayout.Builder builder = StaticLayout.Builder.obtain(
          textSpan, 0, textSpan.length(), textPaint, (int) Math.floor(maxMeasureWidth));
      builder.setMaxLines(maxMeasureLine);

      textLayout = builder.build();
    } else {
      textLayout = StaticLayoutCompat.get(textSpan, 0, textSpan.length(), textPaint,
          (int) Math.floor(maxMeasureWidth), Layout.Alignment.ALIGN_NORMAL, 1, 0.0f, false, null,
          maxMeasureLine, TextDirectionHeuristics.FIRSTSTRONG_LTR);
    }

    return textLayout.getLineCount() > 0
        ? text.substring(textLayout.getLineStart(0), textLayout.getLineEnd(0))
        : "";
  }

  private static float calculateMaxWidth(Layout layout) {
    float maxWidth = -1.f;

    // use getLineCount to get real visible line count
    for (int i = 0; i < layout.getLineCount(); i++) {
      // TODO(zhouzhuangzhuang:) to consider alignment, it's better to reuse getTextRenderer APIs
      float lineWidth = layout.getLineMax(i);
      maxWidth = Math.max(maxWidth, lineWidth);
    }
    return maxWidth;
  }

  public static JavaOnlyMap getTextInfo(String text, String fontSize, @Nullable String fontFamily,
      @Nullable String maxWidth, int maxLine) {
    float width = 0.f;
    int textFontSize = Math.round(UnitUtils.toPx(fontSize, 0, 0, 0, 0, PixelUtils.dipToPx(14)));
    int maxMeasureLine = maxLine;
    float maxMeasureWidth = TextUtils.isEmpty(maxWidth)
        ? 0
        : UnitUtils.toPx(maxWidth, 0, 0, 0, 0, PixelUtils.dipToPx(0));

    JavaOnlyMap resultMap = new JavaOnlyMap();
    if (text.isEmpty() || textFontSize == 0 || (maxLine > 1 && maxMeasureWidth < 1)) {
      // if no valid fontSize or maxWidth set, just return width
      resultMap.putDouble("width", width);
      return resultMap;
    }

    if (maxLine == 1 && maxMeasureWidth < 1) {
      maxMeasureWidth = Short.MAX_VALUE;
    }

    CharSequence textSpan = text;
    TextPaint textPaint = newTextPaint(textFontSize, fontFamily);

    final JavaOnlyArray paramArray = new JavaOnlyArray();
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
      // use StaticLayout#Builder
      StaticLayout.Builder builder = StaticLayout.Builder.obtain(
          textSpan, 0, textSpan.length(), textPaint, (int) Math.floor(maxMeasureWidth));
      builder.setMaxLines(maxMeasureLine);
      Layout textLayout = builder.build();
      width = calculateMaxWidth(textLayout);
      width = PixelUtils.pxToDip(width);
      int lineCount = textLayout.getLineCount();
      for (int i = 0; i < lineCount; i++) {
        int start = textLayout.getLineStart(i);
        int end = textLayout.getLineEnd(i);
        String lineStr = text.substring(start, end);
        paramArray.add(lineStr);
      }
    } else {
      Layout textLayout = StaticLayoutCompat.get(textSpan, 0, textSpan.length(), textPaint,
          (int) Math.floor(maxMeasureWidth), Layout.Alignment.ALIGN_NORMAL, 1, 0.0f, false, null,
          maxMeasureLine, TextDirectionHeuristics.FIRSTSTRONG_LTR);
      int lineCount = textLayout.getLineCount();
      for (int i = 0; i < lineCount; i++) {
        int start = textLayout.getLineStart(i);
        int end = textLayout.getLineEnd(i);
        String lineStr = text.substring(start, end);
        paramArray.add(lineStr);
      }
      width = calculateMaxWidth(textLayout);
      width = PixelUtils.pxToDip(width);
    }
    resultMap.putDouble("width", width);
    resultMap.putArray("content", paramArray);

    return resultMap;
  }

  public static int calcTextTranslateTopOffsetAndAdjustFontMetric(
      int lineHeight, Paint.FontMetricsInt fm, boolean includeFontPadding) {
    int originFontMetricTop = fm.top, originFontMetricAscent = fm.ascent;

    int fontHeight = fm.descent - fm.ascent;
    int halfLeadingTop = (lineHeight - fontHeight) / 2;
    int halfLeadingBottom = lineHeight - halfLeadingTop - fontHeight;
    int fontHeightIncludingPadding = fm.bottom - fm.top;
    int halfLeadingTopIncludingPadding = (lineHeight - fontHeightIncludingPadding) / 2;
    int halfLeadingBottomIncludingPadding =
        lineHeight - halfLeadingTopIncludingPadding - fontHeightIncludingPadding;
    fm.ascent -= halfLeadingTop;
    fm.descent += halfLeadingBottom;
    fm.top -= halfLeadingTopIncludingPadding;
    fm.bottom += halfLeadingBottomIncludingPadding;
    if (fm.descent < 0) {
      fm.ascent -= fm.descent;
      fm.descent = 0;
    }
    if (fm.ascent > 0) {
      fm.descent -= fm.ascent;
      fm.ascent = 0;
    }
    if (fm.bottom < 0) {
      fm.top -= fm.bottom;
      fm.bottom = 0;
    }
    if (fm.top > 0) {
      fm.bottom -= fm.top;
      fm.top = 0;
    }

    if (includeFontPadding) {
      return originFontMetricTop - fm.top;
    } else {
      return originFontMetricAscent - fm.ascent;
    }
  }

  public static String convertRawTextValue(@Nullable Dynamic text) {
    String str = null;
    if (text == null) {
      return str;
    }
    ReadableType type = text.getType();
    switch (type) {
      case String:
        str = text.asString();
        break;
      case Int:
        str = String.valueOf(text.asInt());
        break;
      case Long:
        str = String.valueOf(text.asLong());
        break;
      case Number:
        str = formatDoubleToStringManually(text.asDouble());
        break;
      case Boolean:
        str = String.valueOf(text.asBoolean());
        break;
      case Null:
        str = null;
        break;
    }
    return str;
  }

  /**
   * format double number, such as 1.0 to 1
   *
   * remove extra ".0" in double number, such as "1.0" to "1"
   * remove scientific notation when display big num, such as "1.23456789012E11" to "123456789012"
   * @param num
   * @return String
   */
  public static String formatDoubleToString(double num) {
    DecimalFormat decimalFormat = new DecimalFormat("###################.###########");

    return decimalFormat.format(num);
  }

  /**
   * format double number, such as 1.0 to 1
   *
   * remove extra ".0" in double number, such as "1.0" to "1"
   * remove scientific notation when display big num, such as "1.23456789012E11" to "123456789012"
   * @param num
   * @return String
   */
  public static String formatDoubleToStringManually(double num) {
    if (num < Long.MAX_VALUE && num > Long.MIN_VALUE) {
      long floor = (long) Math.floor(num);
      if (num == floor) {
        return String.valueOf(floor);
      }
    }
    return formatDoubleToString(num);
  }
}
