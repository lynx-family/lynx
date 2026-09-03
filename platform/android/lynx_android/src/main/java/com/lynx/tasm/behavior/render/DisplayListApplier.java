// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import android.graphics.BlurMaskFilter;
import android.graphics.Canvas;
import android.graphics.LinearGradient;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PointF;
import android.graphics.RadialGradient;
import android.graphics.Rect;
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
import java.util.ArrayList;
import java.util.Stack;

public class DisplayListApplier implements Drawable.Callback {
  private static String TAG = "DisplayListApplier";

  static final int OP_BEGIN = 0;
  static final int OP_END = 1;
  static final int OP_FILL = 2;
  static final int OP_DRAW_VIEW = 3;
  static final int OP_TEXT = 6;
  static final int OP_IMAGE = 7;
  static final int OP_BORDER = 9;
  static final int OP_CLIP_RECT = 10;
  static final int OP_RECORD_BOX = 11;
  static final int OP_LINEAR_GRADIENT = 12;
  static final int OP_BOX_SHADOW = 13;
  static final int OP_BACKGROUND_IMAGE = 14;
  static final int OP_RADIAL_GRADIENT = 15;
  static final int SUBTREE_OP_TRANSFORM = 0;
  static final int SUBTREE_OP_OPACITY = 1;
  static final int SUBTREE_OP_FILTER = 2;

  // Native ABI shared with C++ DisplayListItem. PlatformRendererContext passes
  // this stride back to C++ before exposing the direct buffer.
  static final int DISPLAY_LIST_ITEM_SIZE = 56;
  static final int TYPE_OFFSET = 0;
  static final int BEGIN_ID_OFFSET = 4;
  static final int BEGIN_TYPE_OFFSET = 8;
  static final int BEGIN_X_OFFSET = 12;
  static final int BEGIN_Y_OFFSET = 16;
  static final int BEGIN_W_OFFSET = 20;
  static final int BEGIN_H_OFFSET = 24;
  static final int FILL_COLOR_OFFSET = 4;
  static final int FILL_CLIP_INDEX_OFFSET = 8;
  static final int DRAW_VIEW_ID_OFFSET = 4;
  static final int DRAW_VIEW_OFFSET_X_OFFSET = 8;
  static final int DRAW_VIEW_OFFSET_Y_OFFSET = 12;
  static final int TEXT_ID_OFFSET = 4;
  static final int TEXT_BOX_INDEX_OFFSET = 8;
  static final int IMAGE_ID_OFFSET = 4;
  static final int IMAGE_BOX_INDEX_OFFSET = 8;
  static final int BACKGROUND_IMAGE_ID_OFFSET = 4;
  static final int BACKGROUND_IMAGE_TILING_INDEX_OFFSET = 8;
  static final int BACKGROUND_IMAGE_CLIP_INDEX_OFFSET = 12;
  static final int BACKGROUND_IMAGE_REPEAT_X_OFFSET = 16;
  static final int BACKGROUND_IMAGE_REPEAT_Y_OFFSET = 20;
  static final int BORDER_OUT_INDEX_OFFSET = 4;
  static final int BORDER_INNER_INDEX_OFFSET = 8;
  static final int BORDER_COLORS_OFFSET = 12;
  static final int BORDER_STYLES_OFFSET = 28;
  static final int RECORD_BOX_X_OFFSET = 4;
  static final int RECORD_BOX_Y_OFFSET = 8;
  static final int RECORD_BOX_W_OFFSET = 12;
  static final int RECORD_BOX_H_OFFSET = 16;
  static final int RECORD_BOX_RADII_OFFSET = 20;
  static final int RECORD_BOX_HAS_RADII_OFFSET = 52;
  static final int CLIP_RECT_X_OFFSET = 4;
  static final int CLIP_RECT_Y_OFFSET = 8;
  static final int CLIP_RECT_W_OFFSET = 12;
  static final int CLIP_RECT_H_OFFSET = 16;
  static final int CLIP_RECT_RADII_OFFSET = 20;
  static final int CLIP_RECT_HAS_RADII_OFFSET = 52;
  static final int GRADIENT_COLOR_COUNT_OFFSET_OFFSET = 4;
  static final int GRADIENT_COLOR_COUNT_OFFSET = 8;
  static final int GRADIENT_STOP_COUNT_OFFSET_OFFSET = 12;
  static final int GRADIENT_STOP_COUNT_OFFSET = 16;
  static final int GRADIENT_TILING_INDEX_OFFSET = 20;
  static final int GRADIENT_CLIP_INDEX_OFFSET = 24;
  static final int GRADIENT_REPEAT_X_OFFSET = 28;
  static final int GRADIENT_REPEAT_Y_OFFSET = 32;
  static final int GRADIENT_ANGLE_OFFSET = 36;
  static final int RADIAL_GRADIENT_CENTER_X_OFFSET = 36;
  static final int RADIAL_GRADIENT_CENTER_Y_OFFSET = 40;
  static final int RADIAL_GRADIENT_RADIUS_X_OFFSET = 44;
  static final int RADIAL_GRADIENT_RADIUS_Y_OFFSET = 48;
  static final int BOX_SHADOW_SHADOW_BOX_INDEX_OFFSET = 4;
  static final int BOX_SHADOW_CLIP_BOX_INDEX_OFFSET = 8;
  static final int BOX_SHADOW_COLOR_OFFSET = 12;
  static final int BOX_SHADOW_BLUR_RADIUS_OFFSET = 16;
  static final int BOX_SHADOW_CLIP_MODE_OFFSET = 20;

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

  private boolean shouldKeepOverlayRootHorizontalOffset() {
    IRendererHost host = getRendererHost();
    Renderer renderer = host != null ? host.getRenderer() : null;
    return renderer != null && renderer.getUIHost() != null && renderer.getUIHost().isOverlay();
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

  private void drawBackgroundImage(
      Canvas canvas, int id, int tilingIndex, int clipIndex, int repeatX, int repeatY) {
    LynxImageManager imageManager = mContext.getImage(id);
    if (imageManager == null) {
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

    RectF tilingRect = tilingBox.getRectF();
    float width = tilingRect.width();
    float height = tilingRect.height();
    if (width <= 0.f || height <= 0.f) {
      return;
    }

    RectF clipRect = clipBox.getRectF();
    canvas.save();
    if (clipBox.hasBorderRadius()) {
      mReusablePath.reset();
      mReusablePath.addRoundRect(clipRect, clipBox.getBorderRadii(), Path.Direction.CW);
      canvas.clipPath(mReusablePath);
    } else {
      canvas.clipRect(clipRect);
    }

    imageManager.setView(getHostLayer());
    imageManager.updateInnerClipPathForBorderRadius(null);

    boolean repeatHorizontally = repeatX == StyleConstants.BACKGROUND_REPEAT_REPEAT
        || repeatX == StyleConstants.BACKGROUND_REPEAT_REPEAT_X;
    boolean repeatVertically = repeatY == StyleConstants.BACKGROUND_REPEAT_REPEAT
        || repeatY == StyleConstants.BACKGROUND_REPEAT_REPEAT_Y;

    if (!repeatHorizontally && !repeatVertically) {
      drawBackgroundImageTile(canvas, imageManager, tilingRect.left, tilingRect.top, width, height);
      canvas.restore();
      return;
    }

    float tileStartX = tilingRect.left;
    float tileStartY = tilingRect.top;
    if (repeatHorizontally) {
      if (tileStartX > clipRect.left) {
        tileStartX = tileStartX - ((int) Math.ceil((tileStartX - clipRect.left) / width)) * width;
      }
    }
    if (repeatVertically) {
      if (tileStartY > clipRect.top) {
        tileStartY = tileStartY - ((int) Math.ceil((tileStartY - clipRect.top) / height)) * height;
      }
    }

    float pixelAlignedStartX = Math.round(tileStartX);
    float pixelAlignedStartY = Math.round(tileStartY);
    float pixelAlignedWidth = Math.max(1, Math.round(width));
    float pixelAlignedHeight = Math.max(1, Math.round(height));
    for (float x = pixelAlignedStartX; x < clipRect.right; x += pixelAlignedWidth) {
      for (float y = pixelAlignedStartY; y < clipRect.bottom; y += pixelAlignedHeight) {
        drawBackgroundImageTile(canvas, imageManager, x, y, pixelAlignedWidth, pixelAlignedHeight);
        if (!repeatVertically) {
          break;
        }
      }
      if (!repeatHorizontally) {
        break;
      }
    }
    canvas.restore();
  }

  private void drawBackgroundImageTile(Canvas canvas, LynxImageManager imageManager, float left,
      float top, float width, float height) {
    imageManager.updateDrawableBounds(new Rect(
        Math.round(left), Math.round(top), Math.round(left + width), Math.round(top + height)));
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

    if (mItemsBuffer.capacity() < DISPLAY_LIST_ITEM_SIZE
        || mItemsBuffer.capacity() % DISPLAY_LIST_ITEM_SIZE != 0) {
      return;
    }
    int itemsCount = mItemsBuffer.capacity() / DISPLAY_LIST_ITEM_SIZE;

    int currentItemIndex = mItemIndex;

    while (currentItemIndex < itemsCount) {
      int itemByteOffset = currentItemIndex * DISPLAY_LIST_ITEM_SIZE;
      int op = getIntAt(itemByteOffset + TYPE_OFFSET);

      switch (op) {
        case OP_BEGIN: {
          boolean isRoot = mFragmentDepth == 0;
          float x = isRoot && !shouldKeepOverlayRootHorizontalOffset()
              ? .0f
              : getFloatAt(itemByteOffset + BEGIN_X_OFFSET);
          float y = isRoot ? .0f : getFloatAt(itemByteOffset + BEGIN_Y_OFFSET);
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
          int color = getIntAt(itemByteOffset + FILL_COLOR_OFFSET);
          int clipIndex = getIntAt(itemByteOffset + FILL_CLIP_INDEX_OFFSET);
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
          int textId = getIntAt(itemByteOffset + TEXT_ID_OFFSET);
          drawText(canvas, textId);
          break;
        }

        case OP_IMAGE: {
          int imageId = getIntAt(itemByteOffset + IMAGE_ID_OFFSET);
          int boxIndex = getIntAt(itemByteOffset + IMAGE_BOX_INDEX_OFFSET);
          drawImage(canvas, imageId, boxIndex);
          break;
        }

        case OP_BACKGROUND_IMAGE: {
          int imageId = getIntAt(itemByteOffset + BACKGROUND_IMAGE_ID_OFFSET);
          int tilingIndex = getIntAt(itemByteOffset + BACKGROUND_IMAGE_TILING_INDEX_OFFSET);
          int clipIndex = getIntAt(itemByteOffset + BACKGROUND_IMAGE_CLIP_INDEX_OFFSET);
          int repeatX = getIntAt(itemByteOffset + BACKGROUND_IMAGE_REPEAT_X_OFFSET);
          int repeatY = getIntAt(itemByteOffset + BACKGROUND_IMAGE_REPEAT_Y_OFFSET);
          drawBackgroundImage(canvas, imageId, tilingIndex, clipIndex, repeatX, repeatY);
          break;
        }

        case OP_BORDER: {
          mPaint.reset();
          mPaint.setAntiAlias(true);
          int outBoxIndex = getIntAt(itemByteOffset + BORDER_OUT_INDEX_OFFSET);
          int innerBoxIndex = getIntAt(itemByteOffset + BORDER_INNER_INDEX_OFFSET);

          int colorsOffset = itemByteOffset + BORDER_COLORS_OFFSET;
          mReusableBorderColors[Spacing.TOP] = getIntAt(colorsOffset);
          mReusableBorderColors[Spacing.RIGHT] = getIntAt(colorsOffset + 4);
          mReusableBorderColors[Spacing.BOTTOM] = getIntAt(colorsOffset + 8);
          mReusableBorderColors[Spacing.LEFT] = getIntAt(colorsOffset + 12);

          int stylesOffset = itemByteOffset + BORDER_STYLES_OFFSET;
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
          float left = getFloatAt(itemByteOffset + CLIP_RECT_X_OFFSET);
          float top = getFloatAt(itemByteOffset + CLIP_RECT_Y_OFFSET);
          float width = getFloatAt(itemByteOffset + CLIP_RECT_W_OFFSET);
          float height = getFloatAt(itemByteOffset + CLIP_RECT_H_OFFSET);

          mReusableRectF.set(left, top, left + width, top + height);
          normalizeClipRect(mReusableRectF);
          if (getIntAt(itemByteOffset + CLIP_RECT_HAS_RADII_OFFSET) != 0) {
            int radiiOffset = itemByteOffset + CLIP_RECT_RADII_OFFSET;
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
          float left = getFloatAt(itemByteOffset + RECORD_BOX_X_OFFSET);
          float top = getFloatAt(itemByteOffset + RECORD_BOX_Y_OFFSET);
          float width = getFloatAt(itemByteOffset + RECORD_BOX_W_OFFSET);
          float height = getFloatAt(itemByteOffset + RECORD_BOX_H_OFFSET);

          RectF rectF = new RectF(left, top, left + width, top + height);
          float[] borderRadii = null;
          if (getIntAt(itemByteOffset + RECORD_BOX_HAS_RADII_OFFSET) != 0) {
            borderRadii = new float[8];
            int radiiOffset = itemByteOffset + RECORD_BOX_RADII_OFFSET;
            for (int i = 0; i < 8; i++) {
              borderRadii[i] = getFloatAt(radiiOffset + i * 4);
            }
          }
          recordRoundedRectangle(new RoundedRectangle(rectF, borderRadii));
          break;
        }

        case OP_BOX_SHADOW: {
          int shadowBoxIndex = getIntAt(itemByteOffset + BOX_SHADOW_SHADOW_BOX_INDEX_OFFSET);
          int clipBoxIndex = getIntAt(itemByteOffset + BOX_SHADOW_CLIP_BOX_INDEX_OFFSET);
          int color = getIntAt(itemByteOffset + BOX_SHADOW_COLOR_OFFSET);
          float blurRadius = getFloatAt(itemByteOffset + BOX_SHADOW_BLUR_RADIUS_OFFSET);
          int clipMode = getIntAt(itemByteOffset + BOX_SHADOW_CLIP_MODE_OFFSET);
          drawBoxShadow(canvas, shadowBoxIndex, clipBoxIndex, color, blurRadius, clipMode);
          break;
        }

        case OP_LINEAR_GRADIENT: {
          int colorCount = getIntAt(itemByteOffset + GRADIENT_COLOR_COUNT_OFFSET);
          int stopCount = getIntAt(itemByteOffset + GRADIENT_STOP_COUNT_OFFSET);
          int tilingIndex = getIntAt(itemByteOffset + GRADIENT_TILING_INDEX_OFFSET);
          int clipIndex = getIntAt(itemByteOffset + GRADIENT_CLIP_INDEX_OFFSET);
          int repeatX = getIntAt(itemByteOffset + GRADIENT_REPEAT_X_OFFSET);
          int repeatY = getIntAt(itemByteOffset + GRADIENT_REPEAT_Y_OFFSET);
          float angle = getFloatAt(itemByteOffset + GRADIENT_ANGLE_OFFSET);

          int[] colors = null;
          if (colorCount > 0) {
            int colorOffset = getIntAt(itemByteOffset + GRADIENT_COLOR_COUNT_OFFSET_OFFSET);
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
            int stopOffset = getIntAt(itemByteOffset + GRADIENT_STOP_COUNT_OFFSET_OFFSET);
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

        case OP_RADIAL_GRADIENT: {
          int colorCount = getIntAt(itemByteOffset + GRADIENT_COLOR_COUNT_OFFSET);
          int stopCount = getIntAt(itemByteOffset + GRADIENT_STOP_COUNT_OFFSET);
          int tilingIndex = getIntAt(itemByteOffset + GRADIENT_TILING_INDEX_OFFSET);
          int clipIndex = getIntAt(itemByteOffset + GRADIENT_CLIP_INDEX_OFFSET);
          int repeatX = getIntAt(itemByteOffset + GRADIENT_REPEAT_X_OFFSET);
          int repeatY = getIntAt(itemByteOffset + GRADIENT_REPEAT_Y_OFFSET);
          float centerX = getFloatAt(itemByteOffset + RADIAL_GRADIENT_CENTER_X_OFFSET);
          float centerY = getFloatAt(itemByteOffset + RADIAL_GRADIENT_CENTER_Y_OFFSET);
          float radiusX = getFloatAt(itemByteOffset + RADIAL_GRADIENT_RADIUS_X_OFFSET);
          float radiusY = getFloatAt(itemByteOffset + RADIAL_GRADIENT_RADIUS_Y_OFFSET);

          int[] colors = null;
          if (colorCount > 0) {
            int colorOffset = getIntAt(itemByteOffset + GRADIENT_COLOR_COUNT_OFFSET_OFFSET);
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
            int stopOffset = getIntAt(itemByteOffset + GRADIENT_STOP_COUNT_OFFSET_OFFSET);
            if (mReusableGradientStops == null || mReusableGradientStops.length != stopCount) {
              mReusableGradientStops = new float[stopCount];
            }
            stops = mReusableGradientStops;
            for (int i = 0; i < stopCount; i++) {
              stops[i] = mDataBuffer == null ? 0.0f : mDataBuffer.getFloat(stopOffset + i * 4);
            }
          }

          drawRadialGradient(canvas, centerX, centerY, radiusX, radiusY, colors, stops, tilingIndex,
              clipIndex, repeatX, repeatY);
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

    drawGradient(canvas,
        new LinearGradient(startX, startY, endX, endY, colors, stops, Shader.TileMode.CLAMP),
        tilingBox, clipBox, repeatX, repeatY);
  }

  private void drawRadialGradient(Canvas canvas, float centerX, float centerY, float radiusX,
      float radiusY, int[] colors, float[] stops, int tilingIndex, int clipIndex, int repeatX,
      int repeatY) {
    if (colors == null || colors.length < 2 || (stops != null && stops.length != colors.length)
        || radiusX < 0.f || radiusY < 0.f) {
      return;
    }

    RoundedRectangle tilingBox = getNormalizedRoundedRectangle(tilingIndex);
    RoundedRectangle clipBox = getNormalizedRoundedRectangle(clipIndex);
    if (tilingBox == null || clipBox == null) {
      return;
    }

    boolean hasZeroRadius = radiusX == 0.f || radiusY == 0.f;
    RadialGradient gradient = new RadialGradient(
        centerX, centerY, Math.max(radiusX, 1.f), colors, stops, Shader.TileMode.CLAMP);
    if (!hasZeroRadius && radiusX != radiusY) {
      Matrix matrix = new Matrix();
      matrix.setScale(1.f, radiusY / radiusX, centerX, centerY);
      gradient.setLocalMatrix(matrix);
    }
    drawGradient(canvas, gradient, tilingBox, clipBox, repeatX, repeatY);
  }

  private void drawGradient(Canvas canvas, Shader shader, RoundedRectangle tilingBox,
      RoundedRectangle clipBox, int repeatX, int repeatY) {
    final RectF tilingRect = tilingBox.getRectF();
    final float width = tilingRect.width();
    final float height = tilingRect.height();
    if (Float.isNaN(width) || Float.isNaN(height) || width <= 0.f || height <= 0.f) {
      return;
    }

    final boolean repeatHorizontally = repeatX == StyleConstants.BACKGROUND_REPEAT_REPEAT
        || repeatX == StyleConstants.BACKGROUND_REPEAT_REPEAT_X;
    final boolean repeatVertically = repeatY == StyleConstants.BACKGROUND_REPEAT_REPEAT
        || repeatY == StyleConstants.BACKGROUND_REPEAT_REPEAT_Y;
    final float tileWidth = repeatHorizontally ? Math.max(1.f, Math.round(width)) : width;
    final float tileHeight = repeatVertically ? Math.max(1.f, Math.round(height)) : height;

    mPaint.reset();
    mPaint.setAntiAlias(true);
    mPaint.setShader(shader);

    final float left = tilingRect.left;
    final float top = tilingRect.top;

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

    if (!repeatHorizontally && !repeatVertically) {
      canvas.save();
      canvas.translate(left, top);
      canvas.drawRect(0, 0, width, height, mPaint);
      canvas.restore();
    } else {
      final float endTileX = clipRight;
      final float endTileY = clipBottom;

      float tileStartX = left;
      float tileStartY = top;

      if (repeatHorizontally) {
        if (tileStartX > clipLeft) {
          tileStartX = tileStartX - ((int) Math.ceil((tileStartX - clipLeft) / width)) * width;
        }
      }

      if (repeatVertically) {
        if (tileStartY > clipTop) {
          tileStartY = tileStartY - ((int) Math.ceil((tileStartY - clipTop) / height)) * height;
        }
      }

      final int saveCount = canvas.save();
      final float pixelAlignedStartX = Math.round(tileStartX);
      final float pixelAlignedStartY = Math.round(tileStartY);
      canvas.translate(pixelAlignedStartX, pixelAlignedStartY);
      for (float x = pixelAlignedStartX; x < endTileX; x += tileWidth) {
        final int rowSaveCount = canvas.save();
        for (float y = pixelAlignedStartY; y < endTileY; y += tileHeight) {
          canvas.drawRect(0, 0, tileWidth, tileHeight, mPaint);
          canvas.translate(0, tileHeight);

          if (!repeatVertically) {
            break;
          }
        }
        canvas.restoreToCount(rowSaveCount);
        canvas.translate(tileWidth, 0);

        if (!repeatHorizontally) {
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
