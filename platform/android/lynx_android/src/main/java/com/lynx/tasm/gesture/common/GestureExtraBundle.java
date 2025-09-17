// Copyright 2025 The Lynx Authors
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.common;

public class GestureExtraBundle {
  private int gestureDirection =
      0; // 1 —— DIRECTION_HORIZONTAL -1 —— DIRECTION_VERTICAL 0 —— DIRECTION_UNDETERMINED

  private float simultaneousDeltaX =
      0; // The offset x that needs to be consumed by the simultaneous gesture

  private float simultaneousDeltaY =
      0; // The offset y that needs to be consumed by the simultaneous gesture

  private boolean isNeedConsumedSimultaneousGesture = false;

  public int getGestureDirection() {
    return gestureDirection;
  }

  public void setGestureDirection(int gestureDirection) {
    this.gestureDirection = gestureDirection;
  }

  public float getSimultaneousDeltaX() {
    return simultaneousDeltaX;
  }

  public void setSimultaneousDeltaX(float simultaneousDeltaX) {
    this.simultaneousDeltaX = simultaneousDeltaX;
  }

  public float getSimultaneousDeltaY() {
    return simultaneousDeltaY;
  }

  public void setSimultaneousDeltaY(float simultaneousDeltaY) {
    this.simultaneousDeltaY = simultaneousDeltaY;
  }

  public void resetSimultaneousDelta() {
    this.simultaneousDeltaX = 0;
    this.simultaneousDeltaY = 0;
    this.isNeedConsumedSimultaneousGesture = false;
  }

  public boolean isNeedConsumedSimultaneousGesture() {
    return isNeedConsumedSimultaneousGesture;
  }

  public void setNeedConsumedSimultaneousGesture(boolean needConsumedSimultaneousGesture) {
    isNeedConsumedSimultaneousGesture = needConsumedSimultaneousGesture;
  }
}
