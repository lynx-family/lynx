// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.animation;

import static com.lynx.tasm.behavior.StyleConstants.ANIMATIONDIRECTION_ALTERNATE;
import static com.lynx.tasm.behavior.StyleConstants.ANIMATIONDIRECTION_ALTERNATEREVERSE;
import static com.lynx.tasm.behavior.StyleConstants.ANIMATIONDIRECTION_REVERSE;
import static com.lynx.tasm.behavior.StyleConstants.ANIMATIONFILLMODE_BACKWARDS;
import static com.lynx.tasm.behavior.StyleConstants.ANIMATIONFILLMODE_BOTH;
import static com.lynx.tasm.behavior.StyleConstants.ANIMATIONFILLMODE_FORWARDS;

import android.util.SparseArray;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.tasm.base.LLog;
import com.lynx.tasm.behavior.StyleConstants;

public class AnimationInfo implements Cloneable {
  private String name;
  private long duration;
  private long delay;
  private int property;
  private int timingType;
  private float x1;
  private float y1;
  private float x2;
  private float y2;
  private int stepsType;
  private int iterationCount;
  private int fillMode = -1;
  private int direction;
  private int playState = -1;
  private int layoutAnimationType;
  private int orderIndex = -1;

  // for animation
  public AnimationInfo(String name, long duration, long delay, int timingType, float x1, float y1,
      float x2, float y2, int stepsType, int iterationCount, int fillMode, int direction,
      int playState) {
    this.name = name;
    this.duration = duration;
    this.delay = delay;
    this.timingType = timingType;
    this.x1 = x1;
    this.y1 = y1;
    this.x2 = x2;
    this.y2 = y2;
    this.stepsType = stepsType;
    this.iterationCount = iterationCount;
    this.fillMode = fillMode;
    this.direction = direction;
    this.playState = playState;
  }

  // for layout animation
  public AnimationInfo(int layoutAnimationType, long duration, long delay, int property,
      int timingType, float x1, float y1, float x2, float y2, int stepsType) {
    this.layoutAnimationType = layoutAnimationType;
    this.duration = duration;
    this.delay = delay;
    this.property = property;
    this.timingType = timingType;
    this.x1 = x1;
    this.y1 = y1;
    this.x2 = x2;
    this.y2 = y2;
    this.stepsType = stepsType;
  }

  // for transition
  public AnimationInfo() {}

  public AnimationInfo(AnimationInfo info) {
    this.name = info.name;
    this.duration = info.duration;
    this.delay = info.delay;
    this.property = info.property;
    this.timingType = info.timingType;
    this.x1 = info.x1;
    this.y1 = info.y1;
    this.x2 = info.x2;
    this.y2 = info.y2;
    this.stepsType = info.stepsType;
    this.iterationCount = info.iterationCount;
    this.fillMode = info.fillMode;
    this.direction = info.direction;
    this.playState = info.playState;
    this.layoutAnimationType = info.layoutAnimationType;
    this.orderIndex = info.orderIndex;
  }

  public void setProperty(int property) {
    this.property = property;
  }

  public void setOrderIndex(int index) {
    this.orderIndex = index;
  }

  public int getOrderIndex() {
    return this.orderIndex;
  }

  public String getName() {
    return name;
  }

  public long getDuration() {
    return duration;
  }

  public long getDelay() {
    return delay;
  }

  public int getProperty() {
    return property;
  }

  public int getTimingType() {
    return timingType;
  }

  public float getX1() {
    return x1;
  }

  public float getY1() {
    return y1;
  }

  public float getX2() {
    return x2;
  }

  public float getY2() {
    return y2;
  }

  public int getStepsType() {
    return stepsType;
  }

  public int getIterationCount() {
    return iterationCount;
  }

  public int getFillMode() {
    return fillMode;
  }

  public int getDirection() {
    return direction;
  }

  public int getPlayState() {
    return playState;
  }

  public int getLayoutAnimationType() {
    return layoutAnimationType;
  }

  public void setCount(float count) {
    this.x1 = count;
  }

  public int getCount() {
    return (int) x1;
  }

  public void setName(String name) {
    this.name = name;
  }

  public void setDuration(long duration) {
    this.duration = duration;
  }

  public void setDelay(long delay) {
    this.delay = delay;
  }

  public void setTimingType(int timingType) {
    this.timingType = timingType;
  }

  public void setX1(float x1) {
    this.x1 = x1;
  }

  public void setY1(float y1) {
    this.y1 = y1;
  }

  public void setX2(float x2) {
    this.x2 = x2;
  }

  public void setY2(float y2) {
    this.y2 = y2;
  }

  public void setStepsType(int stepsType) {
    this.stepsType = stepsType;
  }

  public void setIterationCount(int iterationCount) {
    this.iterationCount = iterationCount;
  }

  public void setFillMode(int fillMode) {
    this.fillMode = fillMode;
  }

  public void setDirection(int direction) {
    this.direction = direction;
  }

  public void setPlayState(int playState) {
    this.playState = playState;
  }

  public void setLayoutAnimationType(int layoutAnimationType) {
    this.layoutAnimationType = layoutAnimationType;
  }

  public int setTimingFunction(ReadableArray array, int startPos) {
    // FIXME(liyanbo): DrawInfo will remove this 6.
    if (null == array || array.size() < 6) {
      setTimingType(AnimationConstant.INTERCEPTOR_LINEAR);
      setStepsType(0);
      setX1(0);
      setY1(0);
      setX2(0);
      setY2(0);
      return startPos;
    } else {
      setTimingType(array.getInt(startPos));
      setStepsType(array.getInt(startPos + 1));
      setX1((float) array.getDouble(startPos + 2));
      setY1((float) array.getDouble(startPos + 3));
      setX2((float) array.getDouble(startPos + 4));
      setY2((float) array.getDouble(startPos + 5));
      return startPos + 6;
    }
  }

  public void setTimingFunction(
      int timingType, float x1, float y1, float x2, float y2, int stepsType) {
    setTimingType(timingType);
    setStepsType(stepsType);
    setX1(x1);
    setY1(y1);
    setX2(x2);
    setY2(y2);
  }

  @Nullable
  public static AnimationInfo toAnimationInfo(ReadableArray array) {
    if (null == array) {
      return null;
    }
    // FIXME(liyanbo): DrawInfo will remove this 13.
    if (array.size() != 13) {
      LLog.DTHROW();
    }
    AnimationInfo info = new AnimationInfo();
    int idx = 0;
    info.setName(array.getString(idx++));
    info.setDuration((long) array.getDouble(idx++));
    idx = info.setTimingFunction(array, idx);
    info.setDelay((long) array.getDouble(idx++));
    info.setIterationCount(array.getInt(idx++) - 1);
    info.setDirection(array.getInt(idx++));
    info.setFillMode(array.getInt(idx++));
    info.setPlayState(array.getInt(idx));
    return info;
  }

  public boolean isEqualTo(AnimationInfo info) {
    return info != null && isEqualExceptPlayState(info) && playState == info.playState;
  }

  public boolean isOnlyPlayStateChanged(AnimationInfo info) {
    return isEqualExceptPlayState(info) && playState != info.playState;
  }

  public static boolean isDirectionReverse(AnimationInfo info) {
    return (info.getDirection() == ANIMATIONDIRECTION_REVERSE
        || info.getDirection() == ANIMATIONDIRECTION_ALTERNATEREVERSE);
  }

  public static boolean isDirectionAlternate(AnimationInfo info) {
    return (info.getDirection() == ANIMATIONDIRECTION_ALTERNATE
        || info.getDirection() == ANIMATIONDIRECTION_ALTERNATEREVERSE);
  }

  public static boolean isFillModeForwards(AnimationInfo info) {
    return (info.getFillMode() == ANIMATIONFILLMODE_FORWARDS
        || info.getFillMode() == StyleConstants.ANIMATIONFILLMODE_BOTH);
  }

  public static boolean isFillModeBackwards(AnimationInfo info) {
    return (info.getFillMode() == ANIMATIONFILLMODE_BACKWARDS
        || info.getFillMode() == ANIMATIONFILLMODE_BOTH);
  }

  // TODO(WUJINTIAN): Add unit test for this method
  // When both lhsKey and rhsKey exist, keep the one that was added to animationInfos later.
  public static void removeDuplicateAnimation(
      SparseArray<AnimationInfo> infos, int lhsKey, int rhsKey) {
    if ((infos.indexOfKey(lhsKey) >= 0) && (infos.indexOfKey(rhsKey) >= 0)) {
      if (infos.get(lhsKey).getOrderIndex() < infos.get(rhsKey).getOrderIndex()) {
        infos.remove(lhsKey);
      } else {
        infos.remove(rhsKey);
      }
    }
  }

  private boolean isEqualExceptPlayState(AnimationInfo info) {
    return info != null && name.equals(info.name) && duration == info.duration
        && delay == info.delay && property == info.property && timingType == info.timingType
        && x1 == info.x1 && y1 == info.y1 && x2 == info.x2 && y2 == info.y2
        && stepsType == info.stepsType && iterationCount == info.iterationCount
        && fillMode == info.fillMode && direction == info.direction
        && layoutAnimationType == info.layoutAnimationType;
  }
}
