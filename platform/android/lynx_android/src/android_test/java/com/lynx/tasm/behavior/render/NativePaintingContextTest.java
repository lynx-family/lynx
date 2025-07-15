// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.INativeLibraryLoader;
import com.lynx.tasm.LynxEnv;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.ui.UIBody;
import java.lang.reflect.Field;
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
    mNativePaintingContext = new NativePaintingContext(mRootView, mContext);

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
}
