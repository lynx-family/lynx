// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.background;

import static org.junit.Assert.assertEquals;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import android.graphics.Canvas;
import android.graphics.RectF;
import android.graphics.drawable.ColorDrawable;
import androidx.annotation.NonNull;
import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.tasm.behavior.StyleConstants;
import java.util.List;
import org.junit.Test;
import org.mockito.ArgumentCaptor;

public class LayerManagerTest {
  @Test
  public void repeatImageUsesPixelAlignedTileStep() {
    TestLayerManager layerManager = new TestLayerManager();
    TestBackgroundLayerDrawable layerDrawable = new TestBackgroundLayerDrawable();
    layerManager.addLayer(layerDrawable, BackgroundRepeat.REPEAT, BackgroundRepeat.NO_REPEAT);
    layerManager.setBackgroundSize(15.4f, 10.2f);

    Canvas canvas = mock(Canvas.class);
    RectF rect = new RectF(0, 0, 40, 10);

    layerManager.draw(canvas, rect, rect, rect, rect, null, null, false);

    assertEquals(15, layerDrawable.getBounds().width());
    assertEquals(10, layerDrawable.getBounds().height());

    ArgumentCaptor<Float> xCaptor = ArgumentCaptor.forClass(Float.class);
    ArgumentCaptor<Float> yCaptor = ArgumentCaptor.forClass(Float.class);
    verify(canvas, times(3)).translate(xCaptor.capture(), yCaptor.capture());

    List<Float> xValues = xCaptor.getAllValues();
    List<Float> yValues = yCaptor.getAllValues();
    assertEquals(3, xValues.size());
    assertEquals(0f, xValues.get(0), 0.01f);
    assertEquals(15f, xValues.get(1), 0.01f);
    assertEquals(30f, xValues.get(2), 0.01f);
    assertEquals(0f, yValues.get(0), 0.01f);
    assertEquals(0f, yValues.get(1), 0.01f);
    assertEquals(0f, yValues.get(2), 0.01f);
  }

  @Test
  public void backgroundColorClipUsesBottomImageLayerClip() {
    TestLayerManager layerManager = new TestLayerManager();
    layerManager.addLayer(new TestBackgroundLayerDrawable());
    layerManager.addLayer(new TestBackgroundLayerDrawable());
    layerManager.setLayerClips(StyleConstants.BACKGROUND_CLIP_BORDER_BOX,
        StyleConstants.BACKGROUND_CLIP_CONTENT_BOX, StyleConstants.BACKGROUND_CLIP_BORDER_BOX);

    assertEquals(StyleConstants.BACKGROUND_CLIP_CONTENT_BOX, layerManager.getLayerClip());
  }

  @Test
  public void backgroundColorClipCountsNoneImageLayers() {
    TestLayerManager layerManager = new TestLayerManager();
    layerManager.setLayerImages(
        StyleConstants.BACKGROUND_IMAGE_NONE, StyleConstants.BACKGROUND_IMAGE_NONE);
    layerManager.setLayerClips(StyleConstants.BACKGROUND_CLIP_BORDER_BOX,
        StyleConstants.BACKGROUND_CLIP_CONTENT_BOX, StyleConstants.BACKGROUND_CLIP_BORDER_BOX);

    assertEquals(StyleConstants.BACKGROUND_CLIP_CONTENT_BOX, layerManager.getLayerClip());
  }

  @Test
  public void backgroundColorClipUsesFirstClipForImplicitNoneLayer() {
    TestLayerManager layerManager = new TestLayerManager();
    layerManager.setLayerClips(
        StyleConstants.BACKGROUND_CLIP_PADDING_BOX, StyleConstants.BACKGROUND_CLIP_CONTENT_BOX);

    assertEquals(StyleConstants.BACKGROUND_CLIP_PADDING_BOX, layerManager.getLayerClip());
  }

  private static class TestLayerManager extends LayerManager {
    TestLayerManager() {
      super(null, new ColorDrawable(), 14f);
    }

    void addLayer(
        BackgroundLayerDrawable drawable, BackgroundRepeat repeatX, BackgroundRepeat repeatY) {
      mImageLayerDrawableList.add(drawable);
      mImageRepeatList.add(repeatX);
      mImageRepeatList.add(repeatY);
    }

    void addLayer(BackgroundLayerDrawable drawable) {
      mImageLayerDrawableList.add(drawable);
    }

    void setLayerClips(int... clips) {
      JavaOnlyArray values = new JavaOnlyArray();
      for (int clip : clips) {
        values.pushInt(clip);
      }
      setLayerClip(values);
    }

    void setLayerImages(int... images) {
      JavaOnlyArray values = new JavaOnlyArray();
      for (int image : images) {
        values.pushInt(image);
      }
      setLayerImage(values, null);
    }

    void setBackgroundSize(float width, float height) {
      JavaOnlyArray sizes = new JavaOnlyArray();
      sizes.pushDouble(width);
      sizes.pushDouble(height);
      mImageSizeList.add(
          new BackgroundSize(sizes.getDynamic(0), StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER));
      mImageSizeList.add(
          new BackgroundSize(sizes.getDynamic(1), StyleConstants.PLATFORM_LENGTH_UNIT_NUMBER));
    }

    @Override
    protected boolean isMask() {
      return false;
    }
  }

  private static class TestBackgroundLayerDrawable extends BackgroundLayerDrawable {
    @Override
    public boolean isReady() {
      return true;
    }

    @Override
    public int getImageWidth() {
      return 1;
    }

    @Override
    public int getImageHeight() {
      return 1;
    }

    @Override
    public void onAttach() {}

    @Override
    public void onDetach() {}

    @Override
    public void onSizeChanged(int width, int height) {}

    @Override
    public void draw(@NonNull Canvas canvas) {}
  }
}
