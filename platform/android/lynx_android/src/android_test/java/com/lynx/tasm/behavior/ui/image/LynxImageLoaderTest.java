// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import android.app.Application;
import android.content.Context;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.service.ILynxImageService;
import com.lynx.tasm.service.LynxServiceCenter;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxImageLoaderTest {
  @Before
  public void setUp() {
    LynxServiceCenter.inst().unregisterService(ILynxImageService.class);
  }

  @After
  public void tearDown() {
    LynxServiceCenter.inst().unregisterService(ILynxImageService.class);
  }

  @Test
  public void releaseImageWithoutServiceDoesNotThrow() {
    LynxImageLoader imageLoader = new LynxImageLoader(null);

    imageLoader.releaseImage(mock(ImageRequestInfo.class));
  }

  @Test
  public void releaseImageUsesServiceRegisteredAfterConstruction() {
    LynxImageLoader imageLoader = new LynxImageLoader(null);
    ILynxImageService imageService = mock(ILynxImageService.class);
    doReturn((Class) ILynxImageService.class).when(imageService).getServiceClass();
    LynxServiceCenter.inst().registerService(imageService);
    Application application = ApplicationProvider.getApplicationContext();
    LynxServiceCenter.inst().initialize(application);
    ImageRequestInfo imageRequestInfo = mock(ImageRequestInfo.class);

    imageLoader.releaseImage(imageRequestInfo);

    verify(imageService).releaseImage(imageRequestInfo);
  }

  @Test
  public void fetchImageWithoutServiceReportsFailure() {
    LynxImageLoader imageLoader = new LynxImageLoader(null);
    ImageLoadListener loadListener = mock(ImageLoadListener.class);

    imageLoader.fetchImage(mock(ImageRequestInfo.class), loadListener, null, mock(Context.class));

    verify(loadListener)
        .onFailure(
            eq(LynxSubErrorCode.E_RESOURCE_IMAGE_EXCEPTION), any(IllegalStateException.class));
  }
}
