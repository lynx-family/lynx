// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.RectF;
import android.graphics.Region;
import android.graphics.Shader;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.text.Layout;
import android.text.Spanned;
import android.view.View;
import androidx.annotation.NonNull;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.shadow.text.TextUpdateBundle;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.behavior.ui.scroll.AndroidScrollView;
import com.lynx.tasm.behavior.ui.text.AbsInlineImageSpan;
import com.lynx.tasm.behavior.ui.utils.BorderDrawingUtil;
import com.lynx.tasm.behavior.ui.utils.BorderStyle;
import com.lynx.tasm.behavior.ui.utils.Spacing;
import com.lynx.tasm.service.ILynxTextService.Page;
import java.lang.ref.WeakReference;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.Stack;

public class DisplayListApplier implements Drawable.Callback {
  private static String TAG = "DisplayListApplier";

  // Operation type constants matching C++ DisplayListOpType and DisplayListSubtreePropertyOpType
  private static final int OP_BEGIN = 0;
  private static final int OP_END = 1;
  private static final int OP_FILL = 2;
  private static final int OP_DRAW_VIEW = 3;
  private static final int OP_TEXT = 6;
  private static final int OP_IMAGE = 7;
  private static final int OP_CUSTOM = 8;
  private static final int OP_BORDER = 9;
  private static final int OP_CLIP_RECT = 10;
  private static final int OP_RECORD_BOX = 11;
  private static final int OP_LINEAR_GRADIENT = 12;
  private static final int OP_BOX_SHADOW = 13;

  // Subtree property operation types (matching C++ DisplayListSubtreePropertyOpType)
  static final int SUBTREE_OP_TRANSFORM = 0;
  static final int SUBTREE_OP_OPACITY = 1;

  // DisplayListItem size and field offsets (must match C++ static_asserts)
  private static final int ITEM_SIZE = 56; // sizeof(DisplayListItem)
  private static final int ITEM_SIZE_INTS = ITEM_SIZE / 4;

  // Payload field offsets within an item (in bytes, from start of item)
  // These are all relative to the item start (type is at offset 0)
  private static final int PAYLOAD_OFFSET = 4;

  // Begin payload offsets
  private static final int BEGIN_ID_OFFSET = 4;
  private static final int BEGIN_TYPE_OFFSET = 8;
  private static final int BEGIN_X_OFFSET = 12;
  private static final int BEGIN_Y_OFFSET = 16;
  private static final int BEGIN_W_OFFSET = 20;
  private static final int BEGIN_H_OFFSET = 24;

  // Fill payload offsets
  private static final int FILL_COLOR_OFFSET = 4;
  private static final int FILL_CLIP_INDEX_OFFSET = 8;

  // DrawView payload offsets
  private static final int DRAW_VIEW_ID_OFFSET = 4;

  // Text payload offsets
  private static final int TEXT_ID_OFFSET = 4;
  private static final int TEXT_BOX_INDEX_OFFSET = 8;

  // Image payload offsets
  private static final int IMAGE_ID_OFFSET = 4;
  private static final int IMAGE_BOX_INDEX_OFFSET = 8;

  // Border payload offsets
  private static final int BORDER_OUT_INDEX_OFFSET = 4;
  private static final int BORDER_INNER_INDEX_OFFSET = 8;
  private static final int BORDER_COLORS_OFFSET = 12;
  private static final int BORDER_STYLES_OFFSET = 28;

  // RecordBox payload offsets
  private static final int RECORD_BOX_X_OFFSET = 4;
  private static final int RECORD_BOX_Y_OFFSET = 8;
  private static final int RECORD_BOX_W_OFFSET = 12;
  private static final int RECORD_BOX_H_OFFSET = 16;
  private static final int RECORD_BOX_RADII_OFFSET = 20;
  private static final int RECORD_BOX_HAS_RADII_OFFSET = 52;

  // ClipRect payload offsets
  private static final int CLIP_RECT_X_OFFSET = 4;
  private static final int CLIP_RECT_Y_OFFSET = 8;
  private static final int CLIP_RECT_W_OFFSET = 12;
  private static final int CLIP_RECT_H_OFFSET = 16;
  private static final int CLIP_RECT_RADII_OFFSET = 20;
  private static final int CLIP_RECT_HAS_RADII_OFFSET = 52;

  // LinearGradient payload offsets
  private static final int GRADIENT_COLOR_COUNT_OFFSET_OFFSET = 4;
  private static final int GRADIENT_COLOR_COUNT_OFFSET = 8;
  private static final int GRADIENT_STOP_COUNT_OFFSET_OFFSET = 12;
  private static final int GRADIENT_STOP_COUNT_OFFSET = 16;
  private static final int GRADIENT_TILING_INDEX_OFFSET = 20;
  private static final int GRADIENT_CLIP_INDEX_OFFSET = 24;
  private static final int GRADIENT_REPEAT_X_OFFSET = 28;
  private static final int GRADIENT_REPEAT_Y_OFFSET = 32;
  private static final int GRADIENT_ANGLE_OFFSET = 36;

  // BoxShadow payload offsets
  private static final int BOX_SHADOW_SHADOW_BOX_INDEX_OFFSET = 4;
  private static final int BOX_SHADOW_CLIP_BOX_INDEX_OFFSET = 8;
  private static final int BOX_SHADOW_COLOR_OFFSET = 12;
  private static final int BOX_SHADOW_BLUR_RADIUS_OFFSET = 16;
  private static final int BOX_SHADOW_CLIP_MODE_OFFSET = 20;

  private ByteBuffer mItemsBuffer;
  private ByteBuffer mDataBuffer;
  private TextMeasurer mTextMeasurer;
  private Paint mPaint;

  // Reusable objects for optimization
  private final Path mReusablePath = new Path();
  private final Path mReusablePath2 = new Path();
  private final RectF mReusableRectF = new RectF();
  private final float[] mReusableBorderRadii = new float[8];
  private final int[] mReusableBorderColors = new int[4];
  private final BorderStyle[] mReusableBorderStyles = new BorderStyle[4];
  private final PointF mReusablePointF1 = new PointF();
  private final PointF mReusablePointF2 = new PointF();
  private final PointF mReusablePointF3 = new PointF();
  private final PointF mReusablePointF4 = new PointF();
  private int[] mReusableGradientColors;
  private float[] mReusableGradientStops;
  private BlurMaskFilter mReusableBlurMaskFilter;
  private float mLastBlurRadius = -1.f;

  private PlatformRendererContext mContext;
  private int mItemIndex;
  private int mFragmentDepth;

  private WeakReference<IRendererHost> mHostLayer;

  private final ArrayList<RoundedRectangle> mRoundedRectangleArray = new ArrayList<>();

  public DisplayListApplier(ByteBuffer itemsBuffer, ByteBuffer dataBuffer,
      PlatformRendererContext platformRendererContext, IRendererHost hostLayer) {
    mItemsBuffer = itemsBuffer;
    mDataBuffer = dataBuffer;
    mPaint = new Paint();
    mPaint.setAntiAlias(true);
    reset();
    mTextMeasurer = platformRendererContext.getTextMeasurer();
    mContext = platformRendererContext;
    mHostLayer = new WeakReference<>(hostLayer);
  }

  public void reset() {
    mItemIndex = 0;
    mFragmentDepth = 0;
    mRoundedRectangleArray.clear();
  }

  public void drawTillNextView(Canvas canvas) {
    if (mItemsBuffer == null) {
      return;
    }

    // Process content operations
    processContentOperations(canvas);
  }

  private static final class BorderBoxes {
    final RoundedRectangle outBox;
    final RoundedRectangle innerBox;

    BorderBoxes(RoundedRectangle outBox, RoundedRectangle innerBox) {
      this.outBox = outBox;
      this.innerBox = innerBox;
    }
  }

  private IRendererHost getRendererHost() {
    return mHostLayer.get();
  }

  private View getHostLayer() {
    IRendererHost host = getRendererHost();
    return host != null ? host.getView() : null;
  }

  private boolean shouldNormalizeRootGeometry() {
    IRendererHost hostLayer = getRendererHost();
    return mFragmentDepth == 1 && hostLayer != null && hostLayer.getRendererHostWidth() > 0
        && hostLayer.getRendererHostHeight() > 0;
  }

  private RectF getHostBoundsRectF(boolean includeScrollOffset) {
    IRendererHost hostLayer = getRendererHost();
    if (hostLayer == null) {
      return new RectF();
    }

    float left = 0.f;
    float top = 0.f;
    if (includeScrollOffset && hostLayer instanceof AndroidScrollView) {
      left = hostLayer.getRendererHostScrollX();
      top = hostLayer.getRendererHostScrollY();
    }
    return new RectF(left, top, left + hostLayer.getRendererHostWidth(),
        top + hostLayer.getRendererHostHeight());
  }

  private boolean shouldIncludeScrollOffsetForHostBounds() {
    IRendererHost hostLayer = getRendererHost();
    return hostLayer instanceof AndroidScrollView
        && !((AndroidScrollView) hostLayer).isHorizontal();
  }

  private void offsetRectForHostScroll(RectF rect) {
    IRendererHost hostLayer = getRendererHost();
    if (hostLayer == null) {
      return;
    }
    rect.offset(hostLayer.getRendererHostScrollX(), hostLayer.getRendererHostScrollY());
  }

  private float[] cloneBorderRadii(float[] borderRadii) {
    return borderRadii == null ? null : borderRadii.clone();
  }

  private RoundedRectangle createRoundedRectangle(RectF rect, RoundedRectangle source) {
    return new RoundedRectangle(new RectF(rect), cloneBorderRadii(source.getBorderRadii()));
  }

  private RoundedRectangle getRoundedRectangle(int index) {
    if (index < 0 || index >= mRoundedRectangleArray.size()) {
      return null;
    }
    return mRoundedRectangleArray.get(index);
  }

  private RoundedRectangle getNormalizedRoundedRectangle(int index) {
    RoundedRectangle roundedRectangle = getRoundedRectangle(index);
    if (roundedRectangle == null || !shouldNormalizeRootGeometry()) {
      return roundedRectangle;
    }

    RectF normalizedRect = new RectF(roundedRectangle.getRectF());
    if (shouldIncludeScrollOffsetForHostBounds()) {
      offsetRectForHostScroll(normalizedRect);
    } else if (!normalizedRect.intersect(getHostBoundsRectF(false))) {
      normalizedRect.setEmpty();
    }
    if (normalizedRect.equals(roundedRectangle.getRectF())) {
      return roundedRectangle;
    }
    return createRoundedRectangle(normalizedRect, roundedRectangle);
  }

  private BorderBoxes getNormalizedBorderBoxes(int outBoxIndex, int innerBoxIndex) {
    RoundedRectangle outBox = getRoundedRectangle(outBoxIndex);
    RoundedRectangle innerBox = getRoundedRectangle(innerBoxIndex);
    if (outBox == null || innerBox == null || !shouldNormalizeRootGeometry()) {
      return new BorderBoxes(outBox, innerBox);
    }

    RectF rawOutRect = outBox.getRectF();
    RectF rawInnerRect = innerBox.getRectF();
    RectF normalizedOutRect = new RectF(rawOutRect);

    if (shouldIncludeScrollOffsetForHostBounds()) {
      RectF normalizedInnerRect = new RectF(rawInnerRect);
      offsetRectForHostScroll(normalizedOutRect);
      offsetRectForHostScroll(normalizedInnerRect);
      return new BorderBoxes(createRoundedRectangle(normalizedOutRect, outBox),
          createRoundedRectangle(normalizedInnerRect, innerBox));
    }

    RectF hostBounds = getHostBoundsRectF(false);
    if (!normalizedOutRect.intersect(hostBounds)) {
      normalizedOutRect.setEmpty();
    }

    if (normalizedOutRect.equals(rawOutRect)) {
      return new BorderBoxes(outBox, innerBox);
    }

    float borderLeftWidth = Math.max(rawInnerRect.left - rawOutRect.left, 0.f);
    float borderTopWidth = Math.max(rawInnerRect.top - rawOutRect.top, 0.f);
    float borderRightWidth = Math.max(rawOutRect.right - rawInnerRect.right, 0.f);
    float borderBottomWidth = Math.max(rawOutRect.bottom - rawInnerRect.bottom, 0.f);

    RectF normalizedInnerRect =
        new RectF(Math.min(normalizedOutRect.left + borderLeftWidth, normalizedOutRect.right),
            Math.min(normalizedOutRect.top + borderTopWidth, normalizedOutRect.bottom),
            Math.max(normalizedOutRect.right - borderRightWidth, normalizedOutRect.left),
            Math.max(normalizedOutRect.bottom - borderBottomWidth, normalizedOutRect.top));

    if (normalizedInnerRect.right < normalizedInnerRect.left) {
      normalizedInnerRect.right = normalizedInnerRect.left;
    }
    if (normalizedInnerRect.bottom < normalizedInnerRect.top) {
      normalizedInnerRect.bottom = normalizedInnerRect.top;
    }

    return new BorderBoxes(createRoundedRectangle(normalizedOutRect, outBox),
        createRoundedRectangle(normalizedInnerRect, innerBox));
  }

  private void normalizeClipRect(RectF rect) {
    IRendererHost hostLayer = getRendererHost();
    if (hostLayer instanceof AndroidScrollView) {
      rect.offset(hostLayer.getRendererHostScrollX(), hostLayer.getRendererHostScrollY());
    }
    if (!shouldNormalizeRootGeometry()) {
      return;
    }
    if (!rect.intersect(getHostBoundsRectF(true))) {
      rect.setEmpty();
    }
  }

  private void drawImage(Canvas canvas, int id, int boxIndex) {
    LynxImageManager imageManager = mContext.getImage(id);
    if (imageManager == null) {
      return;
    }
    RoundedRectangle rect = boxIndex >= 0 && boxIndex < mRoundedRectangleArray.size()
        ? mRoundedRectangleArray.get(boxIndex)
        : null;
    if (rect != null) {
      imageManager.updateDrawableBounds(rect.getRect());
    }
    imageManager.updateInnerClipPathForBorderRadius(rect);
    imageManager.setView(getHostLayer());
    imageManager.onDraw(canvas);
  }

  private void drawText(Canvas canvas, int textId) {
    if (mContext.getLynxContext() != null && mContext.getLynxContext().isTextServiceModeOn()) {
      drawTextForTextra(canvas, textId);
      return;
    }
    if (mTextMeasurer == null) {
      return;
    }
    TextUpdateBundle textBundle = (TextUpdateBundle) mTextMeasurer.takeTextLayout(textId);
    if (textBundle == null) {
      return;
    }

    if (textBundle.hasImages()) {
      updateInlineImageSpans(textBundle);
    }

    Layout textLayout = textBundle.getTextLayout();
    if (textLayout != null) {
      PointF offset = textBundle.getTextTranslateOffset();
      if (offset != null) {
        canvas.translate(offset.x, offset.y);
      }
      textLayout.draw(canvas);
    }
  }

  private void drawTextForTextra(Canvas canvas, int textId) {
    Page page = mContext.getTextBundle(textId);
    if (page == null) {
      return;
    }
    page.drawPageCanvas(canvas, this);
  }

  private void updateInlineImageSpans(TextUpdateBundle textBundle) {
    Layout layout = textBundle.getTextLayout();
    if (layout == null) {
      return;
    }

    CharSequence text = layout.getText();
    if (text instanceof Spanned) {
      AbsInlineImageSpan.possiblyUpdateInlineImageSpans((Spanned) text, this);
    }
  }

  void recordRoundedRectangle(RoundedRectangle roundedRectangle) {
    mRoundedRectangleArray.add(roundedRectangle);
  }

  private int getIntAt(int byteOffset) {
    return mItemsBuffer.getInt(byteOffset);
  }

  private float getFloatAt(int byteOffset) {
    return mItemsBuffer.getFloat(byteOffset);
  }

  private void processContentOperations(Canvas canvas) {
    if (mItemsBuffer == null) {
      return;
    }

    mItemsBuffer.order(ByteOrder.nativeOrder());
    if (mDataBuffer != null) {
      mDataBuffer.order(ByteOrder.nativeOrder());
    }
    IntBuffer intBuf = mItemsBuffer.asIntBuffer();

    int itemsCount = mItemsBuffer.capacity() / ITEM_SIZE;

    int currentItemIndex = mItemIndex;

    while (currentItemIndex < itemsCount) {
      int itemOffset = currentItemIndex * ITEM_SIZE_INTS;
      int op = intBuf.get(itemOffset); // type field at offset 0

      switch (op) {
        case OP_BEGIN: {
          float x = mFragmentDepth == 0 ? .0f : getFloatAt(itemOffset * 4 + BEGIN_X_OFFSET);
          float y = mFragmentDepth == 0 ? .0f : getFloatAt(itemOffset * 4 + BEGIN_Y_OFFSET);
          canvas.save();
          canvas.translate(x, y);
          mFragmentDepth++;
          break;
        }

        case OP_END: {
          canvas.restore();
          if (mFragmentDepth > 0) {
            mFragmentDepth--;
          }
          break;
        }

        case OP_FILL: {
          mPaint.reset();
          int color = getIntAt(itemOffset * 4 + FILL_COLOR_OFFSET);
          int clipIndex = getIntAt(itemOffset * 4 + FILL_CLIP_INDEX_OFFSET);
          mPaint.setColor(color);

          RoundedRectangle roundedRectangle = getNormalizedRoundedRectangle(clipIndex);
          if (roundedRectangle != null) {
            if (roundedRectangle.hasBorderRadius()) {
              mReusablePath.reset();
              mReusablePath.addRoundRect(roundedRectangle.getRectF(),
                  roundedRectangle.getBorderRadii(), Path.Direction.CW);
              canvas.drawPath(mReusablePath, mPaint);
            } else {
              canvas.drawRect(roundedRectangle.getRectF(), mPaint);
            }
          }
          break;
        }

        case OP_DRAW_VIEW: {
          mItemIndex = currentItemIndex + 1;
          return;
        }

        case OP_TEXT: {
          int textId = getIntAt(itemOffset * 4 + TEXT_ID_OFFSET);
          drawText(canvas, textId);
          break;
        }

        case OP_IMAGE: {
          int imageId = getIntAt(itemOffset * 4 + IMAGE_ID_OFFSET);
          int boxIndex = getIntAt(itemOffset * 4 + IMAGE_BOX_INDEX_OFFSET);
          drawImage(canvas, imageId, boxIndex);
          break;
        }

        case OP_BORDER: {
          mPaint.reset();
          mPaint.setAntiAlias(true);
          int outBoxIndex = getIntAt(itemOffset * 4 + BORDER_OUT_INDEX_OFFSET);
          int innerBoxIndex = getIntAt(itemOffset * 4 + BORDER_INNER_INDEX_OFFSET);

          int colorsOffset = itemOffset * 4 + BORDER_COLORS_OFFSET;
          mReusableBorderColors[Spacing.TOP] = getIntAt(colorsOffset);
          mReusableBorderColors[Spacing.RIGHT] = getIntAt(colorsOffset + 4);
          mReusableBorderColors[Spacing.BOTTOM] = getIntAt(colorsOffset + 8);
          mReusableBorderColors[Spacing.LEFT] = getIntAt(colorsOffset + 12);

          int stylesOffset = itemOffset * 4 + BORDER_STYLES_OFFSET;
          mReusableBorderStyles[Spacing.TOP] = BorderStyle.parse(getIntAt(stylesOffset));
          mReusableBorderStyles[Spacing.RIGHT] = BorderStyle.parse(getIntAt(stylesOffset + 4));
          mReusableBorderStyles[Spacing.BOTTOM] = BorderStyle.parse(getIntAt(stylesOffset + 8));
          mReusableBorderStyles[Spacing.LEFT] = BorderStyle.parse(getIntAt(stylesOffset + 12));

          BorderBoxes borderBoxes = getNormalizedBorderBoxes(outBoxIndex, innerBoxIndex);
          drawRectangularBorders(canvas, mPaint, borderBoxes.outBox, borderBoxes.innerBox,
              mReusableBorderColors, mReusableBorderStyles);
          break;
        }

        case OP_CLIP_RECT: {
          float left = getFloatAt(itemOffset * 4 + CLIP_RECT_X_OFFSET);
          float top = getFloatAt(itemOffset * 4 + CLIP_RECT_Y_OFFSET);
          float width = getFloatAt(itemOffset * 4 + CLIP_RECT_W_OFFSET);
          float height = getFloatAt(itemOffset * 4 + CLIP_RECT_H_OFFSET);

          mReusableRectF.set(left, top, left + width, top + height);
          normalizeClipRect(mReusableRectF);
          if (getIntAt(itemOffset * 4 + CLIP_RECT_HAS_RADII_OFFSET) != 0) {
            int radiiOffset = itemOffset * 4 + CLIP_RECT_RADII_OFFSET;
            for (int i = 0; i < 8; i++) {
              mReusableBorderRadii[i] = getFloatAt(radiiOffset + i * 4);
            }
            mReusablePath.reset();
            mReusablePath.addRoundRect(mReusableRectF, mReusableBorderRadii, Path.Direction.CW);
            canvas.clipPath(mReusablePath);
          } else {
            canvas.clipRect(mReusableRectF);
          }
          break;
        }

        case OP_RECORD_BOX: {
          float left = getFloatAt(itemOffset * 4 + RECORD_BOX_X_OFFSET);
          float top = getFloatAt(itemOffset * 4 + RECORD_BOX_Y_OFFSET);
          float width = getFloatAt(itemOffset * 4 + RECORD_BOX_W_OFFSET);
          float height = getFloatAt(itemOffset * 4 + RECORD_BOX_H_OFFSET);

          RectF rectF = new RectF(left, top, left + width, top + height);
          float[] borderRadii = null;
          if (getIntAt(itemOffset * 4 + RECORD_BOX_HAS_RADII_OFFSET) != 0) {
            borderRadii = new float[8];
            int radiiOffset = itemOffset * 4 + RECORD_BOX_RADII_OFFSET;
            for (int i = 0; i < 8; i++) {
              borderRadii[i] = getFloatAt(radiiOffset + i * 4);
            }
          }
          recordRoundedRectangle(new RoundedRectangle(rectF, borderRadii));
          break;
        }

        case OP_BOX_SHADOW: {
          int shadowBoxIndex = getIntAt(itemOffset * 4 + BOX_SHADOW_SHADOW_BOX_INDEX_OFFSET);
          int clipBoxIndex = getIntAt(itemOffset * 4 + BOX_SHADOW_CLIP_BOX_INDEX_OFFSET);
          int color = getIntAt(itemOffset * 4 + BOX_SHADOW_COLOR_OFFSET);
          float blurRadius = getFloatAt(itemOffset * 4 + BOX_SHADOW_BLUR_RADIUS_OFFSET);
          int clipMode = getIntAt(itemOffset * 4 + BOX_SHADOW_CLIP_MODE_OFFSET);
          drawBoxShadow(canvas, shadowBoxIndex, clipBoxIndex, color, blurRadius, clipMode);
          break;
        }

        case OP_LINEAR_GRADIENT: {
          int colorCount = getIntAt(itemOffset * 4 + GRADIENT_COLOR_COUNT_OFFSET);
          int stopCount = getIntAt(itemOffset * 4 + GRADIENT_STOP_COUNT_OFFSET);
          int tilingIndex = getIntAt(itemOffset * 4 + GRADIENT_TILING_INDEX_OFFSET);
          int clipIndex = getIntAt(itemOffset * 4 + GRADIENT_CLIP_INDEX_OFFSET);
          int repeatX = getIntAt(itemOffset * 4 + GRADIENT_REPEAT_X_OFFSET);
          int repeatY = getIntAt(itemOffset * 4 + GRADIENT_REPEAT_Y_OFFSET);
          float angle = getFloatAt(itemOffset * 4 + GRADIENT_ANGLE_OFFSET);

          int[] colors = null;
          if (colorCount > 0) {
            int colorOffset = getIntAt(itemOffset * 4 + GRADIENT_COLOR_COUNT_OFFSET_OFFSET);
            if (mReusableGradientColors == null || mReusableGradientColors.length != colorCount) {
              mReusableGradientColors = new int[colorCount];
            }
            colors = mReusableGradientColors;
            for (int i = 0; i < colorCount; i++) {
              colors[i] = mDataBuffer == null ? 0 : mDataBuffer.getInt(colorOffset + i * 4);
            }
          }

          float[] stops = null;
          if (stopCount > 0) {
            int stopOffset = getIntAt(itemOffset * 4 + GRADIENT_STOP_COUNT_OFFSET_OFFSET);
            if (mReusableGradientStops == null || mReusableGradientStops.length != stopCount) {
              mReusableGradientStops = new float[stopCount];
            }
            stops = mReusableGradientStops;
            for (int i = 0; i < stopCount; i++) {
              stops[i] = mDataBuffer == null ? 0.0f : mDataBuffer.getFloat(stopOffset + i * 4);
            }
          }

          drawLinearGradient(
              canvas, angle, colors, stops, tilingIndex, clipIndex, repeatX, repeatY);
          break;
        }

        default:
          break;
      }

      currentItemIndex++;
    }

    mItemIndex = currentItemIndex;
  }

  private void drawLinearGradient(Canvas canvas, float angle, int[] colors, float[] stops,
      int tilingIndex, int clipIndex, int repeatX, int repeatY) {
    if (colors == null) {
      return;
    }

    RoundedRectangle tilingBox = getNormalizedRoundedRectangle(tilingIndex);
    if (tilingBox == null) {
      return;
    }

    RoundedRectangle clipBox = getNormalizedRoundedRectangle(clipIndex);
    if (clipBox == null) {
      return;
    }

    final RectF tilingRect = tilingBox.getRectF();
    final float width = tilingRect.width();
    final float height = tilingRect.height();
    final float left = tilingRect.left;
    final float top = tilingRect.top;

    PointF center = mReusablePointF1;
    center.set(width / 2.f, height / 2.f);
    double radial = Math.toRadians(angle);
    float sin = (float) Math.sin(radial);
    float cos = (float) Math.cos(radial);
    float tan = (float) Math.tan(radial);

    PointF m = mReusablePointF2;
    if (sin >= 0 && cos >= 0) {
      m.set(width, top);
    } else if (sin >= 0 && cos < 0) {
      m.set(width, height);
    } else if (sin < 0 && cos < 0) {
      m.set(left, height);
    } else {
      m.set(0, 0);
    }

    float tmp = (center.y - m.y - tan * center.x + tan * m.x);
    float endX = center.x + sin * tmp / (sin * tan + cos);
    float endY = center.y - tmp / (tan * tan + 1);
    float startX = 2 * center.x - endX;
    float startY = 2 * center.y - endY;

    mPaint.reset();
    mPaint.setAntiAlias(true);

    mPaint.setShader(
        new LinearGradient(startX, startY, endX, endY, colors, stops, Shader.TileMode.CLAMP));

    final RectF clipRect = clipBox.getRectF();
    final float clipLeft = clipRect.left;
    final float clipTop = clipRect.top;
    final float clipRight = clipRect.right;
    final float clipBottom = clipRect.bottom;

    canvas.save();
    if (clipBox.hasBorderRadius()) {
      mReusablePath.reset();
      mReusablePath.addRoundRect(clipRect, clipBox.getBorderRadii(), Path.Direction.CW);
      canvas.clipPath(mReusablePath);
    } else {
      canvas.clipRect(clipRect);
    }

    if (repeatX == StyleConstants.BACKGROUND_REPEAT_NO_REPEAT
        && repeatY == StyleConstants.BACKGROUND_REPEAT_NO_REPEAT) {
      canvas.save();
      canvas.translate(left, top);
      canvas.drawRect(0, 0, width, height, mPaint);
      canvas.restore();
    } else {
      final float endTileX = clipRight;
      final float endTileY = clipBottom;

      float tileStartX = left;
      float tileStartY = top;

      if (repeatX == StyleConstants.BACKGROUND_REPEAT_REPEAT
          || repeatX == StyleConstants.BACKGROUND_REPEAT_REPEAT_X) {
        if (tileStartX > clipLeft) {
          tileStartX = tileStartX - ((int) Math.ceil((tileStartX - clipLeft) / width)) * width;
        }
      }

      if (repeatY == StyleConstants.BACKGROUND_REPEAT_REPEAT
          || repeatY == StyleConstants.BACKGROUND_REPEAT_REPEAT_Y) {
        if (tileStartY > clipTop) {
          tileStartY = tileStartY - ((int) Math.ceil((tileStartY - clipTop) / height)) * height;
        }
      }

      final int saveCount = canvas.save();
      final float pixelAlignedStartX = Math.round(tileStartX);
      final float pixelAlignedStartY = Math.round(tileStartY);
      final float pixelAlignedWidth = Math.round(width);
      final float pixelAlignedHeight = Math.round(height);
      canvas.translate(pixelAlignedStartX, pixelAlignedStartY);
      for (float x = pixelAlignedStartX; x < endTileX; x += pixelAlignedWidth) {
        final int rowSaveCount = canvas.save();
        for (float y = pixelAlignedStartY; y < endTileY; y += pixelAlignedHeight) {
          canvas.drawRect(0, 0, pixelAlignedWidth, pixelAlignedHeight, mPaint);
          canvas.translate(0, pixelAlignedHeight);

          if (repeatY == StyleConstants.BACKGROUND_REPEAT_NO_REPEAT) {
            break;
          }
        }
        canvas.restoreToCount(rowSaveCount);
        canvas.translate(pixelAlignedWidth, 0);

        if (repeatX == StyleConstants.BACKGROUND_REPEAT_NO_REPEAT) {
          break;
        }
      }
      canvas.restoreToCount(saveCount);
    }
    canvas.restore();

    mPaint.setShader(null);
  }

  void drawRectangularBorders(Canvas canvas, Paint paint, RoundedRectangle outBox,
      RoundedRectangle innerBox, int[] borderColors, BorderStyle[] borderStyles) {
    if (outBox == null || innerBox == null) {
      LLog.e(TAG, "drawRectangularBorders failed since outBox or innerBox is null.");
      return;
    }

    BorderDrawingUtil.drawBorders(canvas, paint, outBox, innerBox, borderColors, borderStyles);
  }

  void drawRectangularBorders(Canvas canvas, Paint paint, int outBoxIndex, int innerBoxIndex,
      int[] borderColors, BorderStyle[] borderStyles) {
    BorderBoxes borderBoxes = getNormalizedBorderBoxes(outBoxIndex, innerBoxIndex);
    drawRectangularBorders(
        canvas, paint, borderBoxes.outBox, borderBoxes.innerBox, borderColors, borderStyles);
  }

  private void drawBoxShadow(Canvas canvas, int shadowBoxIndex, int clipBoxIndex, int color,
      float blurRadius, int clipMode) {
    RoundedRectangle shadowBox = getRoundedRectangle(shadowBoxIndex);
    if (shadowBox == null) {
      return;
    }

    mPaint.reset();
    mPaint.setAntiAlias(true);
    mPaint.setColor(color);

    if (blurRadius > 0.5f) {
      if (blurRadius != mLastBlurRadius) {
        mReusableBlurMaskFilter = new BlurMaskFilter(blurRadius, BlurMaskFilter.Blur.NORMAL);
        mLastBlurRadius = blurRadius;
      }
      mPaint.setMaskFilter(mReusableBlurMaskFilter);
    }

    mReusablePath.reset();
    RectF shadowRect = shadowBox.getRectF();

    if (shadowBox.hasBorderRadius()) {
      mReusablePath.addRoundRect(shadowRect, shadowBox.getBorderRadii(), Path.Direction.CW);
    } else {
      mReusablePath.addRect(shadowRect, Path.Direction.CW);
    }

    if (clipMode == 1) {
      RoundedRectangle clipBox = getNormalizedRoundedRectangle(clipBoxIndex);
      if (clipBox != null) {
        int saved = canvas.save();
        mReusablePath2.reset();
        RectF clipRect = clipBox.getRectF();
        if (clipBox.hasBorderRadius()) {
          mReusablePath2.addRoundRect(clipRect, clipBox.getBorderRadii(), Path.Direction.CW);
        } else {
          mReusablePath2.addRect(clipRect, Path.Direction.CW);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
          canvas.clipPath(mReusablePath2);
        } else {
          canvas.clipPath(mReusablePath2, Region.Op.INTERSECT);
        }

        mReusablePath2.reset();
        mReusablePath2.setFillType(Path.FillType.EVEN_ODD);
        float outset = Math.max(blurRadius, 1.f) + 100.f;
        mReusablePath2.addRect(clipRect.left - outset, clipRect.top - outset,
            clipRect.right + outset, clipRect.bottom + outset, Path.Direction.CW);
        if (shadowBox.hasBorderRadius()) {
          mReusablePath2.addRoundRect(shadowRect, shadowBox.getBorderRadii(), Path.Direction.CCW);
        } else {
          mReusablePath2.addRect(shadowRect, Path.Direction.CCW);
        }
        canvas.drawPath(mReusablePath2, mPaint);
        canvas.restoreToCount(saved);
      }
    } else {
      RoundedRectangle clipBox = getNormalizedRoundedRectangle(clipBoxIndex);
      if (clipBox != null) {
        int saved = canvas.save();
        mReusablePath2.reset();
        RectF clipRect = clipBox.getRectF();
        if (clipBox.hasBorderRadius()) {
          mReusablePath2.addRoundRect(clipRect, clipBox.getBorderRadii(), Path.Direction.CW);
        } else {
          mReusablePath2.addRect(clipRect, Path.Direction.CW);
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
          canvas.clipOutPath(mReusablePath2);
        } else {
          canvas.clipPath(mReusablePath2, Region.Op.DIFFERENCE);
        }
        canvas.drawPath(mReusablePath, mPaint);
        canvas.restoreToCount(saved);
      } else {
        canvas.drawPath(mReusablePath, mPaint);
      }
    }

    mPaint.setMaskFilter(null);
  }

  public void setBuffer(ByteBuffer itemsBuffer, ByteBuffer dataBuffer) {
    mItemsBuffer = itemsBuffer;
    mDataBuffer = dataBuffer;
    reset();
  }

  @Override
  public void invalidateDrawable(@NonNull Drawable who) {
    IRendererHost hostLayer = getRendererHost();
    if (hostLayer != null) {
      hostLayer.invalidateForRenderer();
    }
  }

  @Override
  public void scheduleDrawable(@NonNull Drawable who, Runnable what, long when) {}

  @Override
  public void unscheduleDrawable(@NonNull Drawable who, Runnable what) {}
}
