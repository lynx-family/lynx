// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;

import com.lynx.tasm.image.LynxImageLoadInfo;
import com.lynx.tasm.resourceprovider.LynxResourceRequest;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;

public class LynxImageLoadInfoTest {
  @Before
  public void setUp() {}

  @After
  public void tearDown() {}

  @Test
  public void LynxImageLoadInfoFailedTest() {
    int errCode = -1011;
    String errMsg = "server response code (504) is not 2xx.";
    LynxImageLoadInfo imageLoadInfo = new LynxImageLoadInfo(
        LynxResourceRequest.LynxResourceType.LynxResourceTypeImage, errCode, errMsg);

    assertEquals(LynxResourceRequest.LynxResourceType.LynxResourceTypeImage,
        imageLoadInfo.getResourceType());
    assertEquals(errCode, imageLoadInfo.getErrCode());
    assertEquals(errMsg, imageLoadInfo.getErrMsg());
  }

  @Test
  public void LynxImageLoadInfoSuccessTest() {
    // test image info
    String src = "https://example/test.png";
    int imageWidth = 953;
    int imageHeight = 984;
    int viewWidth = 984;
    int viewHeight = 984;
    long startTime = 1772262699308L;
    long endTime = 1772262699579L;
    int origin = 2;

    LynxImageLoadInfo imageLoadInfo =
        new LynxImageLoadInfo(LynxResourceRequest.LynxResourceType.LynxResourceTypeImage, 0, null);
    imageLoadInfo.setSrc(src);
    imageLoadInfo.setWidth(imageWidth)
        .setHeight(imageHeight)
        .setViewWidth(viewWidth)
        .setViewHeight(viewHeight)
        .setLoadStart(startTime)
        .setLoadFinish(endTime)
        .setOrigin(origin);

    assertEquals(LynxResourceRequest.LynxResourceType.LynxResourceTypeImage,
        imageLoadInfo.getResourceType());
    assertEquals(0, imageLoadInfo.getErrCode());
    assertNull(imageLoadInfo.getErrMsg());
    assertEquals(src, imageLoadInfo.getSrc());
    assertEquals(imageWidth, imageLoadInfo.getWidth());
    assertEquals(imageHeight, imageLoadInfo.getHeight());
    assertEquals(viewWidth, imageLoadInfo.getViewWidth());
    assertEquals(viewHeight, imageLoadInfo.getViewHeight());
    assertEquals(startTime, imageLoadInfo.getLoadStart());
    assertEquals(endTime, imageLoadInfo.getLoadFinish());
    assertEquals(origin, imageLoadInfo.getOrigin());
  }
}
