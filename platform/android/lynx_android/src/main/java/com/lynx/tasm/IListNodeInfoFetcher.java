// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm;

import com.lynx.react.bridge.JavaOnlyMap;

public interface IListNodeInfoFetcher {
  public JavaOnlyMap getPlatformInfo(int listSign);

  public void renderChild(int listSign, int index, long operationId);

  public void updateChild(int listSign, int oldSign, int newIndex, long operationId);

  public void removeChild(int listSign, int childSign);

  public int obtainChild(
      int listSign, int index, long operationId, boolean enableReuseNotification);

  public void recycleChild(int listSign, int childSign);

  public void obtainChildAsync(int listSign, int index, long operationId);

  public void recycleChildAsync(int listSign, int childSign);

  /**
   *  notify the scrolled distance to C++
   */
  public void scrollByListContainer(
      int containerSign, float x, float y, float originalX, float originalY);

  /**
   *  notify the target scroll position to C++
   *
   */
  public void scrollToPosition(
      int containerSign, int position, float offset, int align, boolean smooth);

  /**
   *
   * notify the  stopped status to C++
   */
  public void scrollStopped(int containerSign);
}
