// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior.ui;

import com.lynx.react.bridge.JavaOnlyArray;
import com.lynx.react.bridge.JavaOnlyMap;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import com.lynx.tasm.base.CalledByNative;

public class PropBundle {
  JavaOnlyMap props_map_ = new JavaOnlyMap();
  JavaOnlyArray event_handler_array_ = new JavaOnlyArray();
  JavaOnlyArray gesture_detector_array_ = new JavaOnlyArray();

  PropBundle() {}

  @CalledByNative
  public ReadableMap getProps() {
    return props_map_;
  }

  @CalledByNative
  public ReadableArray getEventHandlers() {
    return event_handler_array_;
  }

  @CalledByNative
  public ReadableArray getGestures() {
    return gesture_detector_array_;
  }

  @CalledByNative
  static PropBundle createPropBundle() {
    return new PropBundle();
  }

  @CalledByNative
  void putNull(String key) {
    props_map_.putNull(key);
  }

  @CalledByNative
  void putInt(String key, int value) {
    props_map_.putInt(key, value);
  }

  @CalledByNative
  void putLong(String key, long value) {
    props_map_.putLong(key, value);
  }

  @CalledByNative
  void putString(String key, String value) {
    props_map_.putString(key, value);
  }

  @CalledByNative
  void putDouble(String key, double value) {
    props_map_.putDouble(key, value);
  }

  @CalledByNative
  void putBool(String key, boolean value) {
    props_map_.putBoolean(key, value);
  }

  @CalledByNative
  void putMap(String key, JavaOnlyMap map) {
    props_map_.putMap(key, map);
  }

  @CalledByNative
  void putArray(String key, JavaOnlyArray array) {
    props_map_.putArray(key, array);
  }

  @CalledByNative
  boolean contains(String key) {
    return props_map_.containsKey(key);
  }

  @CalledByNative
  void putEventHandler(JavaOnlyMap array) {
    event_handler_array_.pushMap(array);
  }

  @CalledByNative
  void resetEventHandler() {
    event_handler_array_.clear();
  }

  @CalledByNative
  void putGesture(JavaOnlyMap map) {
    gesture_detector_array_.pushMap(map);
  }

  @CalledByNative
  PropBundle shallowCopy() {
    PropBundle bundle = new PropBundle();
    bundle.props_map_ = JavaOnlyMap.shallowCopy(getProps());
    bundle.event_handler_array_ = JavaOnlyArray.shallowCopy(getEventHandlers());
    bundle.gesture_detector_array_ = JavaOnlyArray.shallowCopy(getGestures());
    return bundle;
  }
}
