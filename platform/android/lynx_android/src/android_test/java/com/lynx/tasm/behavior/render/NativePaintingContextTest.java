// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.view.MotionEvent;
import android.view.View;
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
import org.mockito.ArgumentCaptor;
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
  public void getMeaningfulPaintingAreasReturnsEmptyWhenDestroyed() {
    setBooleanField(mNativePaintingContext, "mDestroyed", true);
    assertTrue(mNativePaintingContext.getMeaningfulPaintingAreas().isEmpty());
  }

  @Test
  public void getMeaningfulPaintingAreasReturnsEmptyWhenNativeReturnsNull() {
    NativePaintingContext contextSpy = spy(mNativePaintingContext);
    doReturn(null).when(contextSpy).nativeGetMeaningfulPaintingAreaRecords(anyLong());

    assertTrue(contextSpy.getMeaningfulPaintingAreas().isEmpty());
  }

  @Test
  public void getMeaningfulPaintingAreasBuildsPresentedImageAreaAndSkipsInvalidRects() {
    NativePaintingContext contextSpy = spy(mNativePaintingContext);
    doReturn(new int[] {101, PlatformRendererContext.PlatformRendererType.kImage, 10, 20, 30, 40,
                 102, PlatformRendererContext.PlatformRendererType.kImage, 1, 2, 0, 4})
        .when(contextSpy)
        .nativeGetMeaningfulPaintingAreaRecords(anyLong());

    LynxImageManager imageManager = mock(LynxImageManager.class);
    when(imageManager.getHasContent()).thenReturn(true);
    doReturn(imageManager).when(mSpyPlatformContext).getImage(101);
    doReturn(View.INVISIBLE).when(mSpyPlatformContext).getMeaningfulPaintingAreaVisibleStatus(101);
    doReturn(0.5f).when(mSpyPlatformContext).getMeaningfulPaintingAreaAlpha(101);
    doReturn(0.8f).when(mSpyPlatformContext).getMeaningfulPaintingAreaScaleX(101);
    doReturn(0.9f).when(mSpyPlatformContext).getMeaningfulPaintingAreaScaleY(101);

    List<MeaningfulPaintingArea> areas = contextSpy.getMeaningfulPaintingAreas();

    assertEquals(1, areas.size());
    MeaningfulPaintingArea area = areas.get(0);
    assertEquals(10, area.getOffsetX());
    assertEquals(20, area.getOffsetY());
    assertEquals(30, area.getWidth());
    assertEquals(40, area.getHeight());
    assertTrue(area.isValid());
    assertEquals(ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED,
        area.getMeaningfulContentStatus());
    assertEquals(-1, area.getFirstMeaningfulContentPresentedTimestampMicros());
    assertEquals(View.INVISIBLE, area.getVisibleStatus());
    assertEquals(0.5f, area.getAlpha(), 0.001f);
    assertEquals(0.8f, area.getScaleX(), 0.001f);
    assertEquals(0.9f, area.getScaleY(), 0.001f);
  }

  @Test
  public void getMeaningfulPaintingAreasBuildsPendingImageAreaWhenContentMissing() {
    NativePaintingContext contextSpy = spy(mNativePaintingContext);
    doReturn(new int[] {103, PlatformRendererContext.PlatformRendererType.kImage, 1, 2, 3, 4})
        .when(contextSpy)
        .nativeGetMeaningfulPaintingAreaRecords(anyLong());

    LynxImageManager imageManager = mock(LynxImageManager.class);
    when(imageManager.getHasContent()).thenReturn(false);
    doReturn(imageManager).when(mSpyPlatformContext).getImage(103);

    List<MeaningfulPaintingArea> areas = contextSpy.getMeaningfulPaintingAreas();

    assertEquals(1, areas.size());
    MeaningfulPaintingArea area = areas.get(0);
    assertFalse(area.isValid());
    assertEquals(ILynxUIMeaningfulContent.MeaningfulContentStatus.PENDING,
        area.getMeaningfulContentStatus());
  }

  @Test
  public void getMeaningfulPaintingAreasMarksTextAsPresentedInTextServiceMode() {
    NativePaintingContext contextSpy = spy(mNativePaintingContext);
    doReturn(new int[] {201, PlatformRendererContext.PlatformRendererType.kText, 4, 5, 6, 7})
        .when(contextSpy)
        .nativeGetMeaningfulPaintingAreaRecords(anyLong());
    when(mContext.isTextServiceModeOn()).thenReturn(true);

    Page page = mock(Page.class);
    doReturn(page).when(mSpyPlatformContext).getTextBundle(201);

    List<MeaningfulPaintingArea> areas = contextSpy.getMeaningfulPaintingAreas();

    assertEquals(1, areas.size());
    assertEquals(ILynxUIMeaningfulContent.MeaningfulContentStatus.PRESENTED,
        areas.get(0).getMeaningfulContentStatus());
    assertTrue(areas.get(0).isValid());
  }

  @Test
  public void getMeaningfulPaintingAreasMarksTextAsPendingWithoutLegacyLayout() {
    NativePaintingContext contextSpy = spy(mNativePaintingContext);
    doReturn(new int[] {202, PlatformRendererContext.PlatformRendererType.kText, 8, 9, 10, 11})
        .when(contextSpy)
        .nativeGetMeaningfulPaintingAreaRecords(anyLong());
    when(mContext.isTextServiceModeOn()).thenReturn(false);

    TextMeasurer textMeasurer = mock(TextMeasurer.class);
    doReturn(textMeasurer).when(mSpyPlatformContext).getTextMeasurer();
    when(textMeasurer.takeTextLayout(202)).thenReturn(null);

    List<MeaningfulPaintingArea> areas = contextSpy.getMeaningfulPaintingAreas();

    assertEquals(1, areas.size());
    assertEquals(ILynxUIMeaningfulContent.MeaningfulContentStatus.PENDING,
        areas.get(0).getMeaningfulContentStatus());
    assertTrue(areas.get(0).isValid());
  }

  @Test
  public void getPlatformEventHandlerStateDelegatesToNativeWhenAlive() {
    NativePaintingContext contextSpy = spy(mNativePaintingContext);
    doReturn(7).when(contextSpy).nativeGetPlatformEventHandlerState(anyLong());

    assertEquals(7, contextSpy.getPlatformEventHandlerState());
  }

  @Test
  public void dispatchPlatformMotionEventBuildsNativePayload() {
    NativePaintingContext contextSpy = spy(mNativePaintingContext);
    doReturn(true)
        .when(contextSpy)
        .nativeDispatchPlatformInputEvent(anyLong(), any(int[].class), any(float[].class));

    MotionEvent event = MotionEvent.obtain(1L, 2L, MotionEvent.ACTION_MOVE, 11f, 12f, 0);
    event.setSource(7);

    assertTrue(contextSpy.dispatchPlatformMotionEvent(event));

    ArgumentCaptor<int[]> intCaptor = ArgumentCaptor.forClass(int[].class);
    ArgumentCaptor<float[]> floatCaptor = ArgumentCaptor.forClass(float[].class);
    verify(contextSpy)
        .nativeDispatchPlatformInputEvent(anyLong(), intCaptor.capture(), floatCaptor.capture());
    assertArrayEquals(new int[] {0, MotionEvent.ACTION_MOVE, 7, 1}, intCaptor.getValue());
    assertArrayEquals(new float[] {0f, 11f, 12f}, floatCaptor.getValue(), 0.001f);

    event.recycle();
  }

  private void setBooleanField(Object target, String fieldName, boolean value) {
    try {
      Field field = NativePaintingContext.class.getDeclaredField(fieldName);
      field.setAccessible(true);
      field.setBoolean(target, value);
    } catch (Exception e) {
      fail("Failed to update field " + fieldName + ": " + e.getMessage());
    }
  }
}
