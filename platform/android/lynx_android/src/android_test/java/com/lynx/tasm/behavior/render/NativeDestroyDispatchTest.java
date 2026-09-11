// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertTrue;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class NativeDestroyDispatchTest {
  @Test
  public void nativeDestroyMethodsArePrivateToAvoidVirtualDispatch() throws Exception {
    Method paintingContextDestroy =
        NativePaintingContext.class.getDeclaredMethod("nativeDestroy", long.class);
    Method platformRendererContextDestroy =
        PlatformRendererContext.class.getDeclaredMethod("nativeDestroy", long.class);

    assertTrue(Modifier.isPrivate(paintingContextDestroy.getModifiers()));
    assertTrue(Modifier.isPrivate(platformRendererContextDestroy.getModifiers()));
  }
}
