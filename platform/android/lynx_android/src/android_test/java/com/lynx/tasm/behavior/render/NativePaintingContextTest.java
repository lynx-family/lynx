// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.view.View;
import android.view.ViewGroup;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.BehaviorRegistry;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.shadow.text.TextMeasurer;
import com.lynx.tasm.behavior.ui.ILynxUIMeaningfulContent;
import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import com.lynx.tasm.service.ILynxTextService.Page;
import java.lang.reflect.Field;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

@RunWith(AndroidJUnit4.class)
public class NativePaintingContextTest {
  private NativePaintingContext mNativePaintingContext;
  @Mock private UIBody.UIBodyView mRootView;
  @Mock private LynxContext mContext;
  @Mock private BehaviorRegistry mockBehaviorRegistry;
  private PlatformRendererContext mSpyPlatformContext;

  @Before
  public void setup() {
    MockitoAnnotations.initMocks(this);
    LynxEnv.inst().initNativeLibraries(new INativeLibraryLoader() {
      @Override
      public void loadLibrary(String libName) throws UnsatisfiedLinkError {
        System.loadLibrary(libName);
      }
    });
    mNativePaintingContext = new NativePaintingContext(mRootView, mContext, mockBehaviorRegistry);

    try {
      Field field = NativePaintingContext.class.getDeclaredField("mPlatformRendererContext");
      field.setAccessible(true);
      PlatformRendererContext realPlatformContext =
          (PlatformRendererContext) field.get(mNativePaintingContext);
      mSpyPlatformContext = spy(realPlatformContext);
      field.set(mNativePaintingContext, mSpyPlatformContext);
    } catch (Exception e) {
      fail("Failed to access mPlatformRendererContext field: " + e.getMessage());
    }
  }

  private void installRendererHost(
      int sign, int visibility, float alpha, float scaleX, float scaleY) {
    IRendererHost host = mock(IRendererHost.class);
    ViewGroup view = mock(ViewGroup.class);
    when(host.getView()).thenReturn(view);
    when(view.getVisibility()).thenReturn(visibility);
    when(view.getAlpha()).thenReturn(alpha);
    when(view.getScaleX()).thenReturn(scaleX);
    when(view.getScaleY()).thenReturn(scaleY);
    mSpyPlatformContext.mViewHolder.put(sign, host);
  }

  private void setNativePtr(long nativePtr) {
    try {
      Field field = NativePaintingContext.class.getDeclaredField("mNativePtr");
      field.setAccessible(true);
      field.setLong(mNativePaintingContext, nativePtr);
    } catch (Exception e) {
      fail("Failed to update mNativePtr field: " + e.getMessage());
    }
  }

  @Test
  public void testConstructorInitializesNativePtr() {
    assertTrue("Native pointer should be initialized",
        mNativePaintingContext.getNativePaintingContextPtr() != 0);
  }

  @Test
  public void getNativePaintingContextPtrReturnsCorrectValue() {
    long nativePtr = mNativePaintingContext.getNativePaintingContextPtr();
    assertEquals("getNativePaintingContextPtr should return consistent value", nativePtr,
        mNativePaintingContext.getNativePaintingContextPtr());
  }

  @Test
  public void attachUIBodyViewSetsRootView() {
    UIBody.UIBodyView newView = mock(UIBody.UIBodyView.class);
    mNativePaintingContext.attachUIBodyView(newView);
    verify(mSpyPlatformContext).setRootView(newView);
  }

  @Test
  public void destroyDoesNotThrowException() {
    try {
      mNativePaintingContext.destroy();
    } catch (Exception e) {
      fail("destroy() should not throw exception: " + e.getMessage());
    }
  }

  @Test
  public void buildMeaningfulPaintingArea_irrelevantTypeShouldReturnNull() {
    int[] records = {1001, PlatformRendererContext.PlatformRendererType.kView, 0, 0, 100, 100};
    MeaningfulPaintingArea area = MeaningfulPaintingAreaHelper.buildMeaningfulPaintingArea(
        records, 0, mSpyPlatformContext, mContext);

    assertNull("IRRELEVANT renderer records should be filtered out", area);
  }

  @Test
  public void platformRendererContextMeaningfulPaintingAreaProperties_withoutHostUseDefaults() {
    int sign = 2001;

    assertEquals(View.VISIBLE, mSpyPlatformContext.getMeaningfulPaintingAreaVisibleStatus(sign));
    assertEquals(1.f, mSpyPlatformContext.getMeaningfulPaintingAreaAlpha(sign), 0.f);
    assertEquals(1.f, mSpyPlatformContext.getMeaningfulPaintingAreaScaleX(sign), 0.f);
    assertEquals(1.f, mSpyPlatformContext.getMeaningfulPaintingAreaScaleY(sign), 0.f);
  }

  @Test
  public void platformRendererContextMeaningfulPaintingAreaProperties_withHostUseViewProperties() {
    int sign = 2002;
    installRendererHost(sign, View.INVISIBLE, 0.4f, 1.5f, 0.6f);

    assertEquals(View.INVISIBLE, mSpyPlatformContext.getMeaningfulPaintingAreaVisibleStatus(sign));
    assertEquals(0.4f, mSpyPlatformContext.getMeaningfulPaintingAreaAlpha(sign), 0.f);
    assertEquals(1.5f, mSpyPlatformContext.getMeaningfulPaintingAreaScaleX(sign), 0.f);
    assertEquals(0.6f, mSpyPlatformContext.getMeaningfulPaintingAreaScaleY(sign), 0.f);
  }

  @Test
  public void buildMeaningfulPaintingAreas_emptyRecordsReturnsEmptyList() {
    assertTrue(MeaningfulPaintingAreaHelper
                   .buildMeaningfulPaintingAreas(null, mSpyPlatformContext, mContext)
                   .isEmpty());
    assertTrue(MeaningfulPaintingAreaHelper
                   .buildMeaningfulPaintingAreas(new int[0], mSpyPlatformContext, mContext)
                   .isEmpty());
  }

  @Test
  public void buildMeaningfulPaintingArea_imageWithoutContentReturnsPendingInvalidArea() {
    int sign = 3001;
    LynxImageManager imageManager = mock(LynxImageManager.class);
    when(imageManager.getHasContent()).thenReturn(Boolean.FALSE);
    doReturn(imageManager).when(mSpyPlatformContext).getImage(sign);
    int[] records = {sign, PlatformRendererContext.PlatformRendererType.kImage, 1, 2, 30, 40};

    MeaningfulPaintingArea area = MeaningfulPaintingAreaHelper.buildMeaningfulPaintingArea(
        records, 0, mSpyPlatformContext, mContext);

    assertNotNull(area);
    assertFalse(area.isValid());
    assertEquals(ILynxUIMeaningfulContent.MeaningfulContentStatus.PENDING,
        area.getMeaningfulContentStatus());
    assertEquals(View.VISIBLE, area.getVisibleStatus());
    assertEquals(-1, area.getFirstMeaningfulContentPresentedTimestampMicros());
  }

  @Test
  public void buildMeaningfulPaintingArea_imageWithContentReturnsPresentedArea() {
    int sign = 3005;
    installRendererHost(sign, View.VISIBLE, 0.9f, 1.0f, 1.0f);
    LynxImageManager imageManager = mock(LynxImageManager.class);
    when(imageManager.getHasContent()).thenReturn(Boolean.TRUE);
    doReturn(imageManager).when(mSpyPlatformContext).getImage(sign);
    int[] records = {sign, PlatformRendererContext.PlatformRendererType.kImage, 11, 12, 13, 14};

    MeaningfulPaintingArea area = MeaningfulPaintingAreaHelper.buildMeaningfulPaintingArea(
        records, 0, mSpyPlatformContext, mContext);

    assertNotNull(area);
    assertTrue(area.isValid());
    assertEquals(ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED,
        area.getMeaningfulContentStatus());
    assertEquals(11, area.getOffsetX());
    assertEquals(12, area.getOffsetY());
    assertEquals(13, area.getWidth());
    assertEquals(14, area.getHeight());
  }

  @Test
  public void buildMeaningfulPaintingArea_textWithTextServiceReturnsPresentedArea() {
    int sign = 3002;
    installRendererHost(sign, View.GONE, 0.5f, 1.2f, 0.8f);
    when(mContext.isTextServiceModeOn()).thenReturn(true);
    doReturn(mock(Page.class)).when(mSpyPlatformContext).getTextBundle(sign);
    int[] records = {sign, PlatformRendererContext.PlatformRendererType.kText, 3, 4, 50, 60};

    MeaningfulPaintingArea area = MeaningfulPaintingAreaHelper.buildMeaningfulPaintingArea(
        records, 0, mSpyPlatformContext, mContext);

    assertNotNull(area);
    assertTrue(area.isValid());
    assertEquals(ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED,
        area.getMeaningfulContentStatus());
    assertEquals(View.GONE, area.getVisibleStatus());
    assertEquals(0.5f, area.getAlpha(), 0.f);
    assertEquals(1.2f, area.getScaleX(), 0.f);
    assertEquals(0.8f, area.getScaleY(), 0.f);
  }

  @Test
  public void buildMeaningfulPaintingArea_textWithoutLayoutReturnsPendingArea() {
    int sign = 3003;
    when(mContext.isTextServiceModeOn()).thenReturn(false);
    TextMeasurer textMeasurer = mock(TextMeasurer.class);
    doReturn(textMeasurer).when(mSpyPlatformContext).getTextMeasurer();
    when(textMeasurer.takeTextLayout(sign)).thenReturn(null);
    int[] records = {sign, PlatformRendererContext.PlatformRendererType.kText, 5, 6, 70, 80};

    MeaningfulPaintingArea area = MeaningfulPaintingAreaHelper.buildMeaningfulPaintingArea(
        records, 0, mSpyPlatformContext, mContext);

    assertNotNull(area);
    assertEquals(ILynxUIMeaningfulContent.MeaningfulContentStatus.PENDING,
        area.getMeaningfulContentStatus());
  }

  @Test
  public void buildMeaningfulPaintingAreas_skipInvalidRecords() {
    int sign = 3006;
    LynxImageManager imageManager = mock(LynxImageManager.class);
    when(imageManager.getHasContent()).thenReturn(Boolean.TRUE);
    doReturn(imageManager).when(mSpyPlatformContext).getImage(sign);
    int[] records = {1001, PlatformRendererContext.PlatformRendererType.kView, 0, 0, 20, 20, sign,
        PlatformRendererContext.PlatformRendererType.kImage, 1, 2, 0, 10, sign,
        PlatformRendererContext.PlatformRendererType.kImage, 3, 4, 30, 40};

    List<MeaningfulPaintingArea> areas = MeaningfulPaintingAreaHelper.buildMeaningfulPaintingAreas(
        records, mSpyPlatformContext, mContext);

    assertEquals(1, areas.size());
    assertEquals(3, areas.get(0).getOffsetX());
    assertEquals(4, areas.get(0).getOffsetY());
  }

  @Test
  public void getMeaningfulPaintingAreas_destroyedReturnsEmptyList() {
    mNativePaintingContext.destroy();

    assertTrue(mNativePaintingContext.getMeaningfulPaintingAreas().isEmpty());
  }

  @Test
  public void getMeaningfulPaintingAreas_nativePtrZeroReturnsEmptyList() {
    setNativePtr(0);

    assertTrue(mNativePaintingContext.getMeaningfulPaintingAreas().isEmpty());
  }

  @Test
  public void getMeaningfulPaintingAreas_buildsAreasFromNativeRecords() {
    NativePaintingContext spyPaintingContext = spy(mNativePaintingContext);
    int sign = 3004;
    installRendererHost(sign, View.INVISIBLE, 0.7f, 1.1f, 0.9f);
    LynxImageManager imageManager = mock(LynxImageManager.class);
    when(imageManager.getHasContent()).thenReturn(Boolean.TRUE);
    doReturn(imageManager).when(mSpyPlatformContext).getImage(sign);
    doReturn(new int[] {sign, PlatformRendererContext.PlatformRendererType.kImage, 7, 8, 90, 100})
        .when(spyPaintingContext)
        .nativeGetMeaningfulPaintingAreaRecords(anyLong());

    List<MeaningfulPaintingArea> areas = spyPaintingContext.getMeaningfulPaintingAreas();

    assertEquals(1, areas.size());
    MeaningfulPaintingArea area = areas.get(0);
    assertTrue(area.isValid());
    assertEquals(7, area.getOffsetX());
    assertEquals(8, area.getOffsetY());
    assertEquals(90, area.getWidth());
    assertEquals(100, area.getHeight());
    assertEquals(View.INVISIBLE, area.getVisibleStatus());
    assertEquals(ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED,
        area.getMeaningfulContentStatus());
  }
}
