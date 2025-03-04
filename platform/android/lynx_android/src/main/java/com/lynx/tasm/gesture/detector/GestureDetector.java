// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm.gesture.detector;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.lynx.react.bridge.ReadableArray;
import com.lynx.react.bridge.ReadableMap;
import java.util.*;
/**
 * The GestureDetector class represents a gesture detector used for recognizing and handling
 * different types of gestures. It provides methods to retrieve information about the gesture, such
 * as the gesture ID, gesture type, associated callback names, and the relation map that defines
 * relationships between gestures.
 */
public class GestureDetector {
  // Constants for gesture types

  public static final int GESTURE_TYPE_PAN = 0;
  public static final int GESTURE_TYPE_FLING = 1;
  public static final int GESTURE_TYPE_DEFAULT = 2;
  public static final int GESTURE_TYPE_TAP = 3;
  public static final int GESTURE_TYPE_LONG_PRESS = 4;
  public static final int GESTURE_TYPE_ROTATION = 5;
  public static final int GESTURE_TYPE_PINCH = 6;
  public static final int GESTURE_TYPE_NATIVE = 7;

  // Constant keys for relation map
  public static final String WAIT_FOR = "waitFor";

  public static final String SIMULTANEOUS = "simultaneous";

  public static final String CONTINUE_WITH = "continueWith";

  private final int gestureID;
  private final int gestureType;
  private final List<String> gestureCallbackNames;
  private final Map<String, List<Integer>> relationMap;
  private final ReadableMap configMap;

  /**
   * Constructs a GestureDetector object with the specified properties.
   *
   * @param gestureID           the ID of the gesture
   * @param gestureType         the type of the gesture
   * @param gestureCallbackNames the list of callback names associated with the gesture
   * @param relationMap         the map representing the relationships between gestures
   */
  public GestureDetector(int gestureID, int gestureType,
      @Nullable List<String> gestureCallbackNames,
      @Nullable Map<String, List<Integer>> relationMap) {
    this.gestureID = gestureID;
    this.gestureType = gestureType;
    if (gestureCallbackNames != null) {
      this.gestureCallbackNames = new ArrayList<>(gestureCallbackNames);
    } else {
      this.gestureCallbackNames = new ArrayList<>();
    }
    if (relationMap != null) {
      this.relationMap = new HashMap<>(relationMap);
    } else {
      this.relationMap = new HashMap<>();
    }
    this.configMap = null;
  }

  /**
   * Constructs a GestureDetector object with the specified properties.
   *
   * @param gestureID           the ID of the gesture
   * @param gestureType         the type of the gesture
   * @param gestureCallbackNames the list of callback names associated with the gesture
   * @param relationMap         the map representing the relationships between gestures
   */
  public GestureDetector(int gestureID, int gestureType,
      @Nullable List<String> gestureCallbackNames, @Nullable Map<String, List<Integer>> relationMap,
      @Nullable ReadableMap configMap) {
    this.gestureID = gestureID;
    this.gestureType = gestureType;
    if (gestureCallbackNames != null) {
      this.gestureCallbackNames = new ArrayList<>(gestureCallbackNames);
    } else {
      this.gestureCallbackNames = new ArrayList<>();
    }
    if (relationMap != null) {
      this.relationMap = new HashMap<>(relationMap);
    } else {
      this.relationMap = new HashMap<>();
    }
    this.configMap = configMap;
  }

  /**
   * Returns the ID of the gesture.
   *
   * @return the gesture ID
   */
  public int getGestureID() {
    return gestureID;
  }

  /**
   * Returns the type of the gesture.
   *
   * @return the gesture type
   */
  public int getGestureType() {
    return gestureType;
  }

  /**
   * Returns the config of gesture, such as tapSlop, minDuration, etc.
   * @return the gesture config
   */
  @Nullable
  public ReadableMap getConfigMap() {
    return configMap;
  }

  /**
   * Returns the list of callback names associated with the gesture.
   *
   * @return the list of callback names
   */
  @NonNull
  public List<String> getGestureCallbackNames() {
    return gestureCallbackNames;
  }

  /**
   * Returns the map representing the relationships between gestures.
   *
   * @return the relation map
   */
  @NonNull
  public Map<String, List<Integer>> getRelationMap() {
    return relationMap;
  }

  /**
   * Converts a ReadableArray of gesture detector data to a map of GestureDetectors
   * with the gesture IDs as keys and the corresponding GestureDetectors as values.
   *
   * @param gestureDetectors a ReadableArray of gesture detector data
   * @return a Map of GestureDetectors with the gesture IDs as keys and the corresponding
   *     GestureDetectors as values
   */
  public static Map<Integer, GestureDetector> convertGestureDetectors(
      ReadableArray gestureDetectors) {
    if (gestureDetectors == null) {
      return null;
    }
    Map<Integer, GestureDetector> result = new HashMap<>();

    for (int i = 0; i < gestureDetectors.size(); i++) {
      ReadableMap gestureDetector = gestureDetectors.getMap(i);

      // Extract the properties of the gesture detector
      int id = gestureDetector.getInt("id");
      int type = gestureDetector.getInt("type");
      ReadableArray callbackNamesArray = gestureDetector.getArray("callbackNames");

      // Extract the relation map and convert it to a Map<String, List<Integer>>
      List<String> callbackNames = new ArrayList<>();
      if (callbackNamesArray != null) {
        for (int j = 0; j < callbackNamesArray.size(); j++) {
          callbackNames.add(callbackNamesArray.getString(j));
        }
      }

      Map<String, List<Integer>> relationMap = new HashMap<>();
      ReadableMap relationMapData = gestureDetector.getMap("relationMap");
      if (relationMapData != null) {
        // Extract the "simultaneous" relation and convert it to a List<Integer>
        ReadableArray simultaneous = relationMapData.getArray(SIMULTANEOUS);
        if (simultaneous != null) {
          List<Integer> simultaneousList = new ArrayList<>();
          for (int j = 0; j < simultaneous.size(); j++) {
            simultaneousList.add(simultaneous.getInt(j));
          }
          relationMap.put(SIMULTANEOUS, simultaneousList);
        }

        // Extract the "waitFor" relation and convert it to a List<Integer>
        ReadableArray waitFor = relationMapData.getArray(WAIT_FOR);
        if (waitFor != null) {
          List<Integer> waitForList = new ArrayList<>();
          for (int j = 0; j < waitFor.size(); j++) {
            waitForList.add(waitFor.getInt(j));
          }
          relationMap.put(WAIT_FOR, waitForList);
        }

        // Extract the "continueWith" relation and convert it to a List<Integer>
        ReadableArray continueWith = relationMapData.getArray(CONTINUE_WITH);
        if (continueWith != null) {
          List<Integer> continueWithList = new ArrayList<>();
          for (int j = 0; j < continueWith.size(); j++) {
            continueWithList.add(continueWith.getInt(j));
          }
          relationMap.put(CONTINUE_WITH, continueWithList);
        }
      }

      // Extract the config map
      ReadableMap configMap = gestureDetector.getMap("configMap");

      // Create a new GestureDetector object with the extracted properties and add it to the result
      // Map
      GestureDetector detector =
          new GestureDetector(id, type, callbackNames, relationMap, configMap);
      result.put(id, detector);
    }

    return result;
  }
}
