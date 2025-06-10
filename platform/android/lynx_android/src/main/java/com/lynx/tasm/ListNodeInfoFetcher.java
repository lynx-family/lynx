// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import static androidx.annotation.RestrictTo.Scope.LIBRARY;

import androidx.annotation.RestrictTo;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.tasm.LynxTemplateRender;

public class ListNodeInfoFetcher implements IListNodeInfoFetcher {
  private LynxTemplateRender mRenderer;

  public ListNodeInfoFetcher(LynxTemplateRender renderer) {
    mRenderer = renderer;
  }

  @Override
  public JavaOnlyMap getPlatformInfo(int listSign) {
    if (mRenderer != null) {
      return mRenderer.getListPlatformInfo(listSign);
    }
    return null;
  }

  @Override
  public void renderChild(int listSign, int index, long operationId) {
    if (mRenderer != null) {
      mRenderer.renderChild(listSign, index, operationId);
    }
  }

  @Override
  public void updateChild(int listSign, int oldSign, int newIndex, long operationId) {
    if (mRenderer != null) {
      mRenderer.updateChild(listSign, oldSign, newIndex, operationId);
    }
  }

  @Override
  public void removeChild(int listSign, int childSign) {
    if (mRenderer != null) {
      mRenderer.removeChild(listSign, childSign);
    }
  }

  @Override
  public int obtainChild(
      int listSign, int index, long operationId, boolean enableReuseNotification) {
    if (mRenderer != null) {
      return mRenderer.obtainChild(listSign, index, operationId, enableReuseNotification);
    }
    return -1;
  }

  @Override
  public void recycleChild(int listSign, int childSign) {
    if (mRenderer != null) {
      mRenderer.recycleChild(listSign, childSign);
    }
  }

  @Override
  public void obtainChildAsync(int listSign, int index, long operationId) {
    if (mRenderer != null) {
      mRenderer.obtainChildAsync(listSign, index, operationId);
    }
  }

  @Override
  public void recycleChildAsync(int listSign, int childSign) {
    if (mRenderer != null) {
      mRenderer.recycleChildAsync(listSign, childSign);
    }
  }

  @RestrictTo(LIBRARY)
  @Override
  public void scrollByListContainer(
      int containerSign, float x, float y, float originalX, float originalY) {
    if (mRenderer != null) {
      mRenderer.scrollByListContainer(containerSign, x, y, originalX, originalY);
    }
  }

  @RestrictTo(LIBRARY)
  @Override
  public void scrollToPosition(
      int containerSign, int position, float offset, int align, boolean smooth) {
    if (mRenderer != null) {
      mRenderer.scrollToPosition(containerSign, position, offset, align, smooth);
    }
  }

  @RestrictTo(LIBRARY)
  @Override
  public void scrollStopped(int containerSign) {
    if (mRenderer != null) {
      mRenderer.scrollStopped(containerSign);
    }
  }
}
