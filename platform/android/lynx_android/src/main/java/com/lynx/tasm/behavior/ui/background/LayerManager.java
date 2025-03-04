// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui.background;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Path;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.drawable.Drawable;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.StyleConstants;
import com.lynx.tasm.behavior.ui.LynxBaseUI;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.LynxServiceCenter;
import com.lynx.tasm.utils.PixelUtils;
import java.util.ArrayList;
import java.util.List;

public abstract class LayerManager implements Drawable.Callback {
  protected List<BackgroundLayerDrawable> mImageLayerDrawableList;
  protected List<BackgroundPosition> mImagePosList;
  protected List<Integer> mImageOriginList;
  protected List<Integer> mImageClipList;
  protected List<BackgroundRepeat> mImageRepeatList;
  protected List<BackgroundSize> mImageSizeList;

  protected LynxContext mContext;
  protected Drawable mDrawable;
  protected float mCurFontSize;
  protected Bitmap.Config mBitmapConfig = null;
  // Use bitmap shader to draw linear gradient
  protected boolean mEnableBitmapGradient = false;

  private ILynxImageService mImageService = null;

  public LayerManager(LynxContext context, Drawable drawable, float curFontSize) {
    mContext = context;
    mDrawable = drawable;
    mCurFontSize = curFontSize;

    mImageLayerDrawableList = new ArrayList<>();
    mImagePosList = new ArrayList<>();
    mImageOriginList = new ArrayList<>();
    mImageClipList = new ArrayList<>();
    mImageRepeatList = new ArrayList<>();
    mImageSizeList = new ArrayList<>();
    mImageService = LynxServiceCenter.inst().getService(ILynxImageService.class);
  }

  /*
   * @borderRect Bounding rect of the control
   * @paddingRect Bounding rect excludes the border widths
   * @contentRect Bounding rect excludes border widths + padding widths
   * @outerDrawPath drawPath for gradient
   */
  public void draw(Canvas canvas, RectF borderRect, RectF paddingRect, RectF contentRect,
      RectF clipBox, Path outerDrawPath, Path innerDrawPath, boolean hasBorder) {
    if (mImageLayerDrawableList.isEmpty())
      return;

    for (int index = mImageLayerDrawableList.size() - 1; index >= 0; --index) {
      if (!mImageLayerDrawableList.get(index).isReady())
        continue;

      // 1. decide the painting area from origin
      // 'mask-origin': initial value is border-box
      // 'background-origin': initial value is padding-box
      RectF paintingBox = isMask() ? borderRect : paddingRect;
      if (!mImageOriginList.isEmpty()) {
        int usedOriginIndex = index % mImageOriginList.size();
        switch (mImageOriginList.get(usedOriginIndex)) {
          case StyleConstants.BACKGROUND_ORIGIN_BORDER_BOX:
            paintingBox = borderRect;
            break;
          case StyleConstants.BACKGROUND_ORIGIN_PADDING_BOX:
            paintingBox = paddingRect;
            break;
          case StyleConstants.BACKGROUND_ORIGIN_CONTENT_BOX:
            paintingBox = contentRect;
            break;
          default:
            break;
        }
      }

      Path clipPath = null;

      if (!mImageClipList.isEmpty()) {
        int usedClipIndex = index % mImageClipList.size();
        switch (mImageClipList.get(usedClipIndex)) {
          case StyleConstants.BACKGROUND_CLIP_BORDER_BOX:
            clipBox = borderRect;
            clipPath = outerDrawPath;
            break;
          case StyleConstants.BACKGROUND_CLIP_PADDING_BOX:
            clipBox = paddingRect;
            clipPath = innerDrawPath;
            break;
          case StyleConstants.BACKGROUND_CLIP_CONTENT_BOX:
            clipBox = contentRect;
            clipPath = innerDrawPath;
            break;
        }
      } else {
        clipPath = outerDrawPath;
      }

      BackgroundLayerDrawable bgLayerDrawable = mImageLayerDrawableList.get(index);
      bgLayerDrawable.onAttach();
      final boolean isGradient = (bgLayerDrawable instanceof BackgroundGradientLayer);

      float width, height, widthSrc, heightSrc;
      if (isGradient) {
        widthSrc = width = paintingBox.width();
        heightSrc = height = paintingBox.height();
      } else {
        final float scale = PixelUtils.dipToPx(1.0f);
        widthSrc = mImageLayerDrawableList.get(index).getImageWidth();
        heightSrc = mImageLayerDrawableList.get(index).getImageHeight();
        width = scale * widthSrc;
        height = scale * heightSrc;
      }

      // 2. adjust the size respect 'background-size'/'mask-size'
      // gradient is not limit by background-size/mask-size
      if (!mImageSizeList.isEmpty() && mImageSizeList.size() >= 2) {
        BackgroundSize sizeX;
        BackgroundSize sizeY;
        if (index * 2 >= mImageSizeList.size()) {
          sizeX = mImageSizeList.get(mImageSizeList.size() - 2);
          sizeY = mImageSizeList.get(mImageSizeList.size() - 1);
        } else {
          sizeX = mImageSizeList.get(index * 2);
          sizeY = mImageSizeList.get(index * 2 + 1);
        }
        float selfWidth = paintingBox.width();
        float selfHeight = paintingBox.height();
        float aspect = width / height;

        if (sizeX.isCover()) { // apply cover
          width = selfWidth;
          height = width / aspect;
          if (height < selfHeight) {
            height = selfHeight;
            width = aspect * height;
          }
        } else if (sizeX.isContain()) { // apply contain
          width = selfWidth;
          height = width / aspect;

          if (height > selfHeight) {
            height = selfHeight;
            width = aspect * height;
          }
        } else {
          width = sizeX.apply(selfWidth, width);
          height = sizeY.apply(selfHeight, height);

          if (sizeX.isAuto()) {
            if (isGradient) {
              // gradient has no aspect
              width = paintingBox.width();
            } else {
              width = aspect * height;
            }
          }

          if (sizeY.isAuto()) {
            if (isGradient) {
              // gradient has no aspect
              height = paintingBox.height();
            } else {
              height = width / aspect;
            }
          }
        }
      }
      if (Double.valueOf(height).isNaN() || Double.valueOf(width).isNaN() || width < 1
          || height < 1) {
        continue;
      }

      // 3. decide the position respect 'background-position'/'mask-position'
      // 'background-position': initial value is 0% 0%
      // 'mask-position': initial value is center
      float offsetX = paintingBox.left;
      float offsetY = paintingBox.top;
      if (isMask() && mImagePosList.size() < 2) {
        offsetX += (paintingBox.width() - width) * 0.5;
        offsetY += (paintingBox.height() - height) * 0.5;
      }
      if (mImagePosList.size() >= 2) {
        final int usedPosIndex = index % (mImagePosList.size() / 2);

        final float deltaWidth = paintingBox.width() - width;
        final float deltaHeight = paintingBox.height() - height;

        final BackgroundPosition posX = mImagePosList.get(usedPosIndex * 2);
        final BackgroundPosition posY = mImagePosList.get(usedPosIndex * 2 + 1);
        offsetX += posX.apply(deltaWidth);
        offsetY += posY.apply(deltaHeight);
      }

      // 4. decide repeat type respect "background-repeat"/"mask-repeat"
      /*
       * TileMode:
       */
      BackgroundRepeat repeatXType = BackgroundRepeat.REPEAT;
      BackgroundRepeat repeatYType = BackgroundRepeat.REPEAT;

      if (mImageRepeatList.size() >= 2) {
        final int usedRepeatIndex = index % (mImageRepeatList.size() / 2);
        repeatXType = mImageRepeatList.get(usedRepeatIndex * 2);
        repeatYType = mImageRepeatList.get(usedRepeatIndex * 2 + 1);
      }
      bgLayerDrawable.setEnableBitmapGradient(mEnableBitmapGradient);
      bgLayerDrawable.setBounds(new Rect(0, 0, Math.round(width), Math.round(height)));

      if (outerDrawPath != null && isGradient && !hasBorder) {
        bgLayerDrawable.setPathEffect(outerDrawPath);
      }

      int saveCount = canvas.save();
      // need to make sure all background content is clipped inside view's bounds
      // if clipPath is empty, need to clip at view's bounding rect
      if (clipPath != null && hasBorder) {
        // if has border&radius, do not use PathEffect to draw gradient, just clip
        canvas.clipPath(clipPath);
      } else {
        canvas.clipRect(clipBox);
      }

      if (repeatXType == BackgroundRepeat.NO_REPEAT && repeatYType == BackgroundRepeat.NO_REPEAT) {
        // No repeat, paint the drawable directly
        canvas.save();
        canvas.translate(offsetX, offsetY);
        bgLayerDrawable.draw(canvas);
        canvas.restore();
      } else {
        final float endX = (int) Math.max(paintingBox.right, clipBox.right);
        final float endY = (int) Math.max(paintingBox.bottom, clipBox.bottom);
        float startX = offsetX, startY = offsetY;

        if (repeatXType == BackgroundRepeat.REPEAT || repeatXType == BackgroundRepeat.REPEAT_X) {
          startX = startX - ((int) Math.ceil((double) startX / width)) * width;
        }

        if (repeatYType == BackgroundRepeat.REPEAT || repeatYType == BackgroundRepeat.REPEAT_Y) {
          startY = startY - ((int) Math.ceil((double) startY / height)) * height;
        }

        canvas.save();
        canvas.clipRect(clipBox);
        for (float x = startX; x < endX; x += width) {
          for (float y = startY; y < endY; y += height) {
            canvas.save();
            canvas.translate(x, y);
            bgLayerDrawable.draw(canvas);
            canvas.restore();
            if (repeatYType == BackgroundRepeat.NO_REPEAT) {
              break;
            }
          }

          if (repeatXType == BackgroundRepeat.NO_REPEAT) {
            break;
          }
        }
        canvas.restore();
      }

      canvas.restoreToCount(saveCount);
    }
  }

  public void reset() {
    resetLayers();
  }

  protected abstract boolean isMask();

  public void resetLayers() {
    mImageLayerDrawableList.clear();
    mImagePosList.clear();
    mImageSizeList.clear();
    mImageOriginList.clear();
    mImageRepeatList.clear();
    mImageClipList.clear();
  }

  public boolean hasImageLayers() {
    return !mImageLayerDrawableList.isEmpty();
  }

  public void setBitmapConfig(@Nullable Bitmap.Config config) {
    mBitmapConfig = config;
    if (mImageLayerDrawableList == null) {
      return;
    }
    for (BackgroundLayerDrawable bgImage : mImageLayerDrawableList) {
      if (bgImage != null) {
        bgImage.setBitmapConfig(mBitmapConfig);
      }
    }
  }

  public void setLayerImage(ReadableArray bgImage, LynxBaseUI ui) {
    for (BackgroundLayerDrawable item : mImageLayerDrawableList) {
      item.onDetach();
    }
    mImageLayerDrawableList.clear();
    if (bgImage == null) {
      return;
    }
    final Rect bounds = mDrawable.getBounds();
    int count = bgImage.size();
    for (int i = 0; i < count; i++) {
      int type = (int) bgImage.getLong(i);
      if (type == StyleConstants.BACKGROUND_IMAGE_URL) {
        i++;
        if (mImageService == null) {
          continue;
        }
        BackgroundLayerDrawable layer =
            mImageService.createBackgroundImageDrawable(mContext, bgImage.getString(i));
        if (layer == null) {
          continue;
        }
        layer.setLynxUI(ui);
        layer.setCallback(this);
        mImageLayerDrawableList.add(layer);
      } else if (type == StyleConstants.BACKGROUND_IMAGE_LINEAR_GRADIENT) {
        i++;
        mImageLayerDrawableList.add(new BackgroundLinearGradientLayer(bgImage.getArray(i)));
      } else if (type == StyleConstants.BACKGROUND_IMAGE_RADIAL_GRADIENT) {
        i++;
        mImageLayerDrawableList.add(new BackgroundRadialGradientLayer(bgImage.getArray(i)));
      } else if (type == StyleConstants.BACKGROUND_IMAGE_NONE) {
        i++;
        mImageLayerDrawableList.add(new BackgroundNoneLayer());
      }
      if (!bounds.isEmpty()) {
        mImageLayerDrawableList.get(mImageLayerDrawableList.size() - 1)
            .onSizeChanged(bounds.width(), bounds.height());
      }
    }
  }

  public void setLayerPosition(ReadableArray bgImgPosition) {
    mImagePosList.clear();
    if (bgImgPosition == null || bgImgPosition.size() % 2 != 0) {
      return;
    }
    for (int i = 0; i < bgImgPosition.size(); i += 2) {
      int type = bgImgPosition.getInt(i + 1);
      mImagePosList.add(new BackgroundPosition(bgImgPosition.getDynamic(i), type));
    }
  }

  public void setLayerOrigin(ReadableArray bgImgOrigin) {
    mImageOriginList.clear();
    if (bgImgOrigin == null) {
      return;
    }
    int count = bgImgOrigin.size();
    for (int i = 0; i < count; i++) {
      int origin = bgImgOrigin.getInt(i);
      if (origin < StyleConstants.BACKGROUND_ORIGIN_PADDING_BOX
          || origin > StyleConstants.BACKGROUND_ORIGIN_CONTENT_BOX) {
        origin = StyleConstants.BACKGROUND_ORIGIN_BORDER_BOX;
      }
      mImageOriginList.add(origin);
    }
  }

  public void setLayerRepeat(ReadableArray bgImgRepeat) {
    mImageRepeatList.clear();

    if (bgImgRepeat == null) {
      return;
    }

    int count = bgImgRepeat.size();
    for (int i = 0; i < count; i++) {
      mImageRepeatList.add(BackgroundRepeat.valueOf(bgImgRepeat.getInt(i)));
    }
  }

  public void setLayerClip(ReadableArray bgClip) {
    mImageClipList.clear();
    if (bgClip == null) {
      return;
    }

    int count = bgClip.size();
    for (int i = 0; i < count; i++) {
      int value = bgClip.getInt(i);
      if (value < StyleConstants.BACKGROUND_CLIP_PADDING_BOX
          || value > StyleConstants.BACKGROUND_CLIP_CONTENT_BOX) {
        mImageClipList.add(StyleConstants.BACKGROUND_CLIP_BORDER_BOX);
      } else {
        mImageClipList.add(value);
      }
    }
  }

  // FIXME: fit old background code
  // once all background refactored code is merged this method will be removed!!
  public int getLayerClip() {
    if (mImageClipList.isEmpty()) {
      return StyleConstants.BACKGROUND_CLIP_BORDER_BOX;
    } else {
      return mImageClipList.get(mImageClipList.size() - 1);
    }
  }

  public void setLayerSize(ReadableArray bgSize) {
    mImageSizeList.clear();
    if (bgSize == null || bgSize.size() % 2 != 0) {
      return;
    }
    int count = bgSize.size();
    for (int i = 0; i < count; i += 2) {
      mImageSizeList.add(new BackgroundSize(bgSize.getDynamic(i), bgSize.getInt(i + 1)));
    }
  }

  public void onAttach() {
    for (BackgroundLayerDrawable item : mImageLayerDrawableList) item.onAttach();
  }

  public void onDetach() {
    for (BackgroundLayerDrawable item : mImageLayerDrawableList) item.onDetach();
  }

  public void configureBounds(Rect bounds) {
    for (BackgroundLayerDrawable item : mImageLayerDrawableList)
      item.onSizeChanged(bounds.width(), bounds.height());
  }

  @Override
  public void invalidateDrawable(@NonNull Drawable who) {
    mDrawable.invalidateSelf();
  }

  @Override
  public void scheduleDrawable(@NonNull Drawable who, @NonNull Runnable what, long when) {}

  @Override
  public void unscheduleDrawable(@NonNull Drawable who, @NonNull Runnable what) {}

  public void setEnableBitmapGradient(boolean enable) {
    mEnableBitmapGradient = enable;
  }
}
