// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.behavior.ui.image;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.graphics.drawable.ColorDrawable;
import androidx.test.annotation.UiThreadTest;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import com.lynx.tasm.behavior.LynxContext;
import com.lynx.tasm.behavior.LynxUIOwner;
import com.lynx.tasm.behavior.ui.UIBody;
import com.lynx.tasm.behavior.ui.UIParams;
import com.lynx.tasm.image.ImageContent;
import com.lynx.tasm.image.ReleasableImage;
import com.lynx.tasm.image.model.ImageInfo;
import com.lynx.tasm.image.model.ImageLoadListener;
import com.lynx.tasm.image.model.ImageRequestInfo;
import com.lynx.tasm.image.model.LynxImageFetcher;
import com.lynx.testing.base.TestingUtils;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;

@RunWith(AndroidJUnit4.class)
public class ImageManagerLifecycleTest {
  @Test
  @UiThreadTest
  public void clearImageMapReleasesLoadedSourcesAndRequests() {
    LynxContext context = TestingUtils.getLynxContext();
    UIBody.UIBodyView bodyView = new UIBody.UIBodyView(context);
    LynxImageFetcher fetcher = mock(LynxImageFetcher.class);
    context.setImageFetcher(fetcher);
    LynxImageManager manager = new LynxImageManager(context);
    manager.setSrc("https://example.com/image.png");
    manager.setPlaceholder("https://example.com/placeholder.png");
    manager.tryFetchPlaceholderFromService(10, 10);
    manager.tryFetchImageFromService(10, 10);
    ArgumentCaptor<ImageRequestInfo> requests = ArgumentCaptor.forClass(ImageRequestInfo.class);
    ArgumentCaptor<ImageLoadListener> listeners = ArgumentCaptor.forClass(ImageLoadListener.class);
    verify(fetcher, times(2))
        .loadImage(requests.capture(), listeners.capture(), any(), eq(context));
    ReleasableImage placeholder = mock(ReleasableImage.class);
    ReleasableImage source = mock(ReleasableImage.class);
    when(placeholder.getDrawable()).thenReturn(new ColorDrawable());
    when(source.getDrawable()).thenReturn(new ColorDrawable());
    listeners.getAllValues().get(0).onSuccess(new ImageContent(placeholder),
        requests.getAllValues().get(0), new ImageInfo(10, 10, false));
    listeners.getAllValues().get(1).onSuccess(
        new ImageContent(source), requests.getAllValues().get(1), new ImageInfo(10, 10, false));
    assertTrue(manager.getHasContent());
    bodyView.registerImageAccordingToNodeIndex(11, manager);

    bodyView.clearNodeIndexImageMap();
    bodyView.clearNodeIndexImageMap();

    verify(fetcher).releaseImage(requests.getAllValues().get(0));
    verify(fetcher).releaseImage(requests.getAllValues().get(1));
    verify(placeholder).release();
    verify(source).release();
    assertFalse(manager.getHasContent());
    assertNull(bodyView.peekImageAccordingToNodeIndex(11));

    // A completion already queued by the loader must not restore a destroyed manager's content.
    listeners.getAllValues().get(1).onSuccess(new ImageContent(new ColorDrawable()),
        requests.getAllValues().get(1), new ImageInfo(10, 10, false));
    listeners.getAllValues().get(0).onSuccess(new ImageContent(new ColorDrawable()),
        requests.getAllValues().get(0), new ImageInfo(10, 10, false));
    assertFalse(manager.getHasContent());
  }

  @Test
  @UiThreadTest
  public void fallbackPreservesOrdinaryImageManagerUntilItsNewOwnerIsDestroyed() {
    LynxContext context = TestingUtils.getLynxContext();
    context.setLynxUIOwner(mock(LynxUIOwner.class));
    UIBody.UIBodyView bodyView = new UIBody.UIBodyView(context);
    context.setUIBodyView(bodyView);
    context.markFallbackProcess(true);
    LynxImageManager transferred = mock(LynxImageManager.class);
    LynxImageManager remaining = mock(LynxImageManager.class);
    bodyView.registerImageAccordingToNodeIndex(11, transferred);
    bodyView.registerImageAccordingToNodeIndex(12, remaining);
    UIImage image = new UIImage(context, new UIParams(1, 11, false, "image", null, null, null));

    context.markFallbackProcess(false);

    assertSame(transferred, image.mLynxImageManager);
    assertNull(bodyView.peekImageAccordingToNodeIndex(11));
    verify(transferred, never()).destroy();
    verify(remaining).destroy();
    image.destroy();
    verify(transferred).destroy();
  }

  @Test
  @UiThreadTest
  public void fallbackPreservesFlattenedImageManagerUntilItsNewOwnerIsDestroyed() {
    LynxContext context = TestingUtils.getLynxContext();
    context.setLynxUIOwner(mock(LynxUIOwner.class));
    UIBody.UIBodyView bodyView = new UIBody.UIBodyView(context);
    context.setUIBodyView(bodyView);
    context.markFallbackProcess(true);
    LynxImageManager transferred = mock(LynxImageManager.class);
    LynxImageManager remaining = mock(LynxImageManager.class);
    bodyView.registerImageAccordingToNodeIndex(11, transferred);
    bodyView.registerImageAccordingToNodeIndex(12, remaining);
    FlattenUIImage image =
        new FlattenUIImage(context, new UIParams(1, 11, true, "image", null, null, null));

    context.markFallbackProcess(false);

    assertSame(transferred, image.mLynxImageManager);
    assertNull(bodyView.peekImageAccordingToNodeIndex(11));
    verify(transferred, never()).destroy();
    verify(remaining).destroy();
    image.destroy();
    verify(transferred).destroy();
  }
}
