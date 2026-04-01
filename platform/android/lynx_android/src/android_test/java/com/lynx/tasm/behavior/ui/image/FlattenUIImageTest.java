// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.view.View;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.ILynxUIMeaningfulContent.MeaningfulContentStatus;
import com.lynx.tasm.behavior.ui.MeaningfulPaintingArea;
import com.lynx.testing.base.TestingUtils;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class FlattenUIImageTest {
  private LynxContext mContext;

  @Before
  public void setUp() {
    mContext = TestingUtils.getLynxContext();
  }

  @After
  public void tearDown() {}

  @Test
  public void convertToMeaningfulPaintingArea_pendingImageShouldBeInvalid() {
    FlattenUIImage image = new FlattenUIImage(mContext);
    LynxImageManager imageManager = mock(LynxImageManager.class);
    when(imageManager.getHasContent()).thenReturn(false);
    image.mLynxImageManager = imageManager;
    image.setWidth(66);
    image.setHeight(66);

    MeaningfulPaintingArea area = image.convertToMeaningfulPaintingArea(507, 1454);

    assertNotNull(area);
    assertEquals(507, area.getOffsetX());
    assertEquals(1454, area.getOffsetY());
    assertEquals(66, area.getWidth());
    assertEquals(66, area.getHeight());
    assertFalse(area.isValid());
    assertEquals(MeaningfulContentStatus.PENDING, area.getMeaningfulContentStatus());
    assertEquals(View.VISIBLE, area.getVisibleStatus());
    verify(imageManager).tryHandleResult();
  }

  @Test
  public void convertToMeaningfulPaintingArea_presentedImageShouldBeValid() {
    FlattenUIImage image = new FlattenUIImage(mContext);
    LynxImageManager imageManager = mock(LynxImageManager.class);
    when(imageManager.getHasContent()).thenReturn(true);
    image.mLynxImageManager = imageManager;
    image.setWidth(275);
    image.setHeight(275);

    MeaningfulPaintingArea area = image.convertToMeaningfulPaintingArea(403, 474);

    assertNotNull(area);
    assertEquals(403, area.getOffsetX());
    assertEquals(474, area.getOffsetY());
    assertEquals(275, area.getWidth());
    assertEquals(275, area.getHeight());
    assertTrue(area.isValid());
    assertEquals(MeaningfulContentStatus.PRESENTED, area.getMeaningfulContentStatus());
    assertEquals(View.VISIBLE, area.getVisibleStatus());
    verify(imageManager).tryHandleResult();
  }
}
