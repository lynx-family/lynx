// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.render;

import static org.junit.Assert.assertNull;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.doCallRealMethod;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import androidx.test.annotation.UiThreadTest;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.image.LynxImageManager;
import java.lang.ref.WeakReference;
import java.lang.reflect.Field;
import java.util.HashMap;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class PlatformRendererContextImageLifecycleTest {
  private PlatformRendererContext mRendererContext;
  private UIBody.UIBodyView mBodyView;

  @Before
  public void setUp() throws Exception {
    mBodyView = new UIBody.UIBodyView(ApplicationProvider.getApplicationContext());
    // Exercise the Java lifecycle method without constructing the unrelated native context.
    mRendererContext = mock(PlatformRendererContext.class);
    mRendererContext.mRootView = new WeakReference<>(mBodyView);
    for (String name : new String[] {"mImageUIMethodTargets", "mImageUIMethodSignsByKey"}) {
      Field field = PlatformRendererContext.class.getDeclaredField(name);
      field.setAccessible(true);
      field.set(mRendererContext, new HashMap<>());
    }
    doCallRealMethod().when(mRendererContext).destroyImage(anyInt());
  }

  @Test
  @UiThreadTest
  public void destroyImageReleasesRemovedManagerOnlyOnce() {
    LynxImageManager manager = mock(LynxImageManager.class);
    mBodyView.registerImageAccordingToNodeIndex(11, manager);

    mRendererContext.destroyImage(11);
    mRendererContext.destroyImage(11);
    mBodyView.clearNodeIndexImageMap();

    verify(manager).destroy();
    assertNull(mBodyView.peekImageAccordingToNodeIndex(11));
  }

  @Test
  @UiThreadTest
  public void destroyImageAfterMapCleanupDoesNotReleaseManagerAgain() {
    LynxImageManager manager = mock(LynxImageManager.class);
    mBodyView.registerImageAccordingToNodeIndex(11, manager);

    mBodyView.clearNodeIndexImageMap();
    mRendererContext.destroyImage(11);

    verify(manager).destroy();
  }

  @Test
  @UiThreadTest
  public void destroyMissingImageIsSafe() {
    mRendererContext.destroyImage(99);
  }

  @Test
  @UiThreadTest
  public void destroyImageAfterRootIsClearedIsSafe() {
    mRendererContext.mRootView.clear();

    mRendererContext.destroyImage(11);
  }
}
