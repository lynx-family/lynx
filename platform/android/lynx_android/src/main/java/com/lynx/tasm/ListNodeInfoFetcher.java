// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static androidx.annotation.RestrictTo.Scope.LIBRARY;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxTemplateRender;

public class ListNodeInfoFetcher {
  private LynxTemplateRender mRenderer;

  public ListNodeInfoFetcher(LynxTemplateRender renderer) {
    mRenderer = renderer;
  }

  public JavaOnlyMap getPlatformInfo(int listSign) {
    if (mRenderer != null) {
      return mRenderer.getListPlatformInfo(listSign);
    }
    return null;
  }

  public void renderChild(int listSign, int index, long operationId) {
    if (mRenderer != null) {
      mRenderer.renderChild(listSign, index, operationId);
    }
  }

  public void updateChild(int listSign, int oldSign, int newIndex, long operationId) {
    if (mRenderer != null) {
      mRenderer.updateChild(listSign, oldSign, newIndex, operationId);
    }
  }

  public void removeChild(int listSign, int childSign) {
    if (mRenderer != null) {
      mRenderer.removeChild(listSign, childSign);
    }
  }

  public int obtainChild(
      int listSign, int index, long operationId, boolean enableReuseNotification) {
    if (mRenderer != null) {
      return mRenderer.obtainChild(listSign, index, operationId, enableReuseNotification);
    }
    return -1;
  }

  public void recycleChild(int listSign, int childSign) {
    if (mRenderer != null) {
      mRenderer.recycleChild(listSign, childSign);
    }
  }

  public void obtainChildAsync(int listSign, int index, long operationId) {
    if (mRenderer != null) {
      mRenderer.obtainChildAsync(listSign, index, operationId);
    }
  }

  public void recycleChildAsync(int listSign, int childSign) {
    if (mRenderer != null) {
      mRenderer.recycleChildAsync(listSign, childSign);
    }
  }

  /**
   *  notify the scrolled distance to C++
   */
  @RestrictTo(LIBRARY)
  public void scrollByListContainer(
      int containerSign, float x, float y, float originalX, float originalY) {
    if (mRenderer != null) {
      mRenderer.scrollByListContainer(containerSign, x, y, originalX, originalY);
    }
  }

  /**
   *  notify the target scroll position to C++
   *
   */
  @RestrictTo(LIBRARY)
  public void scrollToPosition(
      int containerSign, int position, float offset, int align, boolean smooth) {
    if (mRenderer != null) {
      mRenderer.scrollToPosition(containerSign, position, offset, align, smooth);
    }
  }

  /**
   *
   * notify the  stopped status to C++
   */
  @RestrictTo(LIBRARY)
  public void scrollStopped(int containerSign) {
    if (mRenderer != null) {
      mRenderer.scrollStopped(containerSign);
    }
  }
}
