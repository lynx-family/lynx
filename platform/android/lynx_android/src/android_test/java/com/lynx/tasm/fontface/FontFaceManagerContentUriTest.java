// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.fontface;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Application;
import android.graphics.Typeface;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.LynxError;
import com.lynx.tasm.LynxSubErrorCode;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.loader.LynxFontFaceLoader;
import com.lynx.tasm.provider.LynxProviderRegistry;
import com.lynx.tasm.provider.LynxResourceCallback;
import com.lynx.tasm.provider.LynxResourceProvider;
import com.lynx.tasm.provider.LynxResourceRequest;
import com.lynx.tasm.provider.LynxResourceResponse;
import com.lynx.tasm.service.ILynxResourceService;
import com.lynx.tasm.service.ILynxResourceServiceRequestOperation;
import com.lynx.tasm.service.ILynxResourceServiceResponse;
import com.lynx.tasm.service.LynxResourceServiceCallback;
import com.lynx.tasm.service.LynxResourceServiceRequestParams;
import com.lynx.tasm.service.LynxServiceCenter;
import java.lang.reflect.Method;
import java.util.Iterator;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

@RunWith(AndroidJUnit4.class)
public class FontFaceManagerContentUriTest {
  private static final String CONTENT_URI = "content://com.example.fontprovider/fonts/font.ttf";
  private static final String FILE_URI = "file:///missing-font.ttf";
  private static final String ORIGINAL_SRC = "file:///original-font.ttf";

  private LynxContext mContext;
  private LynxProviderRegistry mProviderRegistry;
  private ContentTypefaceService mResourceService;

  @Before
  public void setUp() {
    LynxServiceCenter.inst().unregisterAllService();
    Application application = ApplicationProvider.getApplicationContext();
    LynxServiceCenter.inst().initialize(application);

    mContext = mock(LynxContext.class);
    mProviderRegistry = new LynxProviderRegistry();
    when(mContext.getProviderRegistry()).thenReturn(mProviderRegistry);
    mProviderRegistry.addLynxResourceProvider(
        LynxProviderRegistry.LYNX_PROVIDER_TYPE_FONT, new ReturningFontProvider(CONTENT_URI));

    mResourceService = new ContentTypefaceService();
    LynxServiceCenter.inst().registerService(mResourceService);
  }

  @After
  public void tearDown() {
    LynxFontFaceLoader.setLoader(null);
    LynxServiceCenter.inst().unregisterAllService();
  }

  @Test
  public void contentUriDelegatesUnchangedToResourceServiceWithoutReadinessGate() throws Exception {
    Typeface expected = Typeface.create("serif", Typeface.NORMAL);
    mResourceService.typeface = expected;

    FontFace fontFace = new FontFace();
    fontFace.addLocal(ORIGINAL_SRC);
    loadTypeface(fontFace);

    assertEquals(CONTENT_URI, mResourceService.requestedUri);
    assertFalse(mResourceService.isReadyCalled);
    assertNotNull(fontFace.getTypeface());
    assertSame(expected, fontFace.getTypeface().getStyledTypeFace(Typeface.NORMAL));
  }

  @Test
  public void nullResourceServiceTypefaceReportsFailureAndFallsBackToOriginalSource()
      throws Exception {
    Typeface fallback = Typeface.create("monospace", Typeface.NORMAL);
    LynxFontFaceLoader.setLoader(new LynxFontFaceLoader.Loader() {
      @Override
      protected Typeface onLoadFontFace(LynxContext context, FontFace.TYPE type, String src) {
        return fallback;
      }
    });

    FontFace fontFace = new FontFace();
    fontFace.addLocal(ORIGINAL_SRC);
    loadTypeface(fontFace);

    assertEquals(CONTENT_URI, mResourceService.requestedUri);
    assertRegisterFailureReported();
    assertNotNull(fontFace.getTypeface());
    assertSame(fallback, fontFace.getTypeface().getStyledTypeFace(Typeface.NORMAL));
  }

  @Test
  public void resourceServiceExceptionReportsFailureAndFallsBackToOriginalSource()
      throws Exception {
    Typeface fallback = Typeface.create("sans", Typeface.NORMAL);
    mResourceService.exception = new IllegalStateException("content URI failed");
    LynxFontFaceLoader.setLoader(new LynxFontFaceLoader.Loader() {
      @Override
      protected Typeface onLoadFontFace(LynxContext context, FontFace.TYPE type, String src) {
        return fallback;
      }
    });

    FontFace fontFace = new FontFace();
    fontFace.addLocal(ORIGINAL_SRC);
    loadTypeface(fontFace);

    assertEquals(CONTENT_URI, mResourceService.requestedUri);
    LynxError error = assertRegisterFailureReported();
    assertTrue(error.getMsg().contains("\"error_stack\""));
    assertNotNull(fontFace.getTypeface());
    assertSame(fallback, fontFace.getTypeface().getStyledTypeFace(Typeface.NORMAL));
  }

  @Test
  public void missingResourceServiceReportsFailureAndFallsBackToOriginalSource() throws Exception {
    Typeface fallback = Typeface.create("cursive", Typeface.NORMAL);
    LynxServiceCenter.inst().unregisterService(ILynxResourceService.class);
    LynxFontFaceLoader.setLoader(new LynxFontFaceLoader.Loader() {
      @Override
      protected Typeface onLoadFontFace(LynxContext context, FontFace.TYPE type, String src) {
        return fallback;
      }
    });

    FontFace fontFace = new FontFace();
    fontFace.addLocal(ORIGINAL_SRC);
    loadTypeface(fontFace);

    assertRegisterFailureReported();
    assertNotNull(fontFace.getTypeface());
    assertSame(fallback, fontFace.getTypeface().getStyledTypeFace(Typeface.NORMAL));
  }

  @Test
  public void fileUriDoesNotDelegateToResourceService() throws Exception {
    Typeface fallback = Typeface.create("serif-monospace", Typeface.NORMAL);
    mProviderRegistry.addLynxResourceProvider(
        LynxProviderRegistry.LYNX_PROVIDER_TYPE_FONT, new ReturningFontProvider(FILE_URI));
    LynxFontFaceLoader.setLoader(new LynxFontFaceLoader.Loader() {
      @Override
      protected Typeface onLoadFontFace(LynxContext context, FontFace.TYPE type, String src) {
        return fallback;
      }
    });

    FontFace fontFace = new FontFace();
    fontFace.addLocal(ORIGINAL_SRC);
    loadTypeface(fontFace);

    assertNull(mResourceService.requestedUri);
    assertNotNull(fontFace.getTypeface());
    assertSame(fallback, fontFace.getTypeface().getStyledTypeFace(Typeface.NORMAL));
  }

  private LynxError assertRegisterFailureReported() {
    ArgumentCaptor<LynxError> errorCaptor = ArgumentCaptor.forClass(LynxError.class);
    verify(mContext).reportResourceError(eq(CONTENT_URI), eq("font"), errorCaptor.capture());
    LynxError error = errorCaptor.getValue();
    assertEquals(LynxSubErrorCode.E_RESOURCE_FONT_REGISTER_FAILED, error.getSubCode());
    return error;
  }

  private void loadTypeface(FontFace fontFace) throws Exception {
    FontFaceGroup group = new FontFaceGroup();
    group.addFontFace(fontFace);
    Method method = FontFaceManager.class.getDeclaredMethod(
        "loadTypeface", LynxContext.class, FontFaceGroup.class, Iterator.class, Handler.class);
    method.setAccessible(true);
    method.invoke(FontFaceManager.getInstance(), mContext, group, fontFace.getSrc().iterator(),
        new Handler(Looper.getMainLooper()));
  }

  private static class ReturningFontProvider extends LynxResourceProvider<Bundle, String> {
    private final String mPath;

    ReturningFontProvider(String path) {
      mPath = path;
    }

    @Override
    public void request(
        LynxResourceRequest<Bundle> request, LynxResourceCallback<String> callback) {
      callback.onResponse(LynxResourceResponse.success(mPath));
    }
  }

  private static class ContentTypefaceService implements ILynxResourceService {
    private Typeface typeface;
    private RuntimeException exception;
    private String requestedUri;
    private boolean isReadyCalled;

    @Override
    public Typeface createTypeFace(String uri) {
      requestedUri = uri;
      if (exception != null) {
        throw exception;
      }
      return typeface;
    }

    @Override
    public boolean isReady() {
      isReadyCalled = true;
      return false;
    }

    @Override
    public int isLocalResource(String url) {
      return RESULT_IS_NOT_LOCAL_RESOURCE;
    }

    @Override
    public void preloadMedia(
        String url, String preloadKey, String videoID, long size, PreloadMediaCallback callback) {}

    @Override
    public void cancelPreloadMedia(String preloadKey, String videoID) {}

    @Override
    public void addResourceLoader(Object loader, String templateUrl) {}

    @Override
    public ILynxResourceServiceRequestOperation fetchResourceAsync(String url,
        LynxResourceServiceRequestParams lynxResourceServiceRequestParams,
        LynxResourceServiceCallback callback) {
      return null;
    }

    @Override
    public ILynxResourceServiceResponse fetchResourceSync(
        String url, LynxResourceServiceRequestParams lynxResourceServiceRequestParams) {
      return null;
    }
  }
}
