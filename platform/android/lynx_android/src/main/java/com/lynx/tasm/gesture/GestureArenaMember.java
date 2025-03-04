// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture;

import androidx.annotation.Nullable;
import com.lynx.tasm.gesture.detector.GestureDetector;
import com.lynx.tasm.gesture.handler.BaseGestureHandler;
import java.util.Map;

/**
 * Interface representing a member of a gesture arena.
 */
public interface GestureArenaMember {
  /**
   * Called when the gesture should be scrolled by the specified delta values.
   *
   * @param deltaX The delta value for scrolling in the x-axis.
   * @param deltaY The delta value for scrolling in the y-axis.
   */
  void onGestureScrollBy(float deltaX, float deltaY);

  /**
   * Checks if the gesture can consume the specified delta values.
   *
   * @param deltaX The delta value for scrolling in the x-axis.
   * @param deltaY The delta value for scrolling in the y-axis.
   * @return True if the gesture can consume the delta values, false otherwise.
   */
  boolean canConsumeGesture(float deltaX, float deltaY);

  /**
   * Get sign of lynx ui
   * @return
   */
  int getSign();

  /**
   * Retrieves the ID of the gesture arena member.
   *
   * @return The ID of the gesture arena member.
   */
  int getGestureArenaMemberId();

  /**
   * Retrieves the scroll position of the member in the x-axis.
   *
   * @return The scroll position in the x-axis.
   */
  int getMemberScrollX();

  /**
   * get current scroller container is at border edge or not
   * @param isStart if it is at start or end
   * @return true —— at border edge, false —— not at border edge
   */
  boolean isAtBorder(boolean isStart);

  /**
   * Retrieves the scroll position of the member in the y-axis.
   *
   * @return The scroll position in the y-axis.
   */
  int getMemberScrollY();

  /**
   * Called when the member needs to be invalidated.
   */
  void onInvalidate();

  /**
   * Retrieves the map of gesture detectors associated with the member.
   *
   * @return The map of gesture detectors, or null if not available.
   */
  @Nullable Map<Integer, GestureDetector> getGestureDetectorMap();

  /**
   * Retrieves the map of gesture handler associated with th member
   *
   * @return The map of gesture handlers, or null if not available,
   * @see GestureDetector key —— GestureDetector type value —— gesture handler
   */
  @Nullable Map<Integer, BaseGestureHandler> getGestureHandlers();
}
