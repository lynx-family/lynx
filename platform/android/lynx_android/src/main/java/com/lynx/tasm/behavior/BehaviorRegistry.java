// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import android.util.Log;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class BehaviorRegistry {
  private final Map<String, Behavior> mBehaviorMap;

  public BehaviorRegistry() {
    mBehaviorMap = new HashMap<>();
  }

  public BehaviorRegistry(Map<String, Behavior> behaviorMap) {
    if (behaviorMap == null) {
      mBehaviorMap = new HashMap<>();
    } else {
      mBehaviorMap = new HashMap<>(behaviorMap);
    }
  }

  /**
   * @deprecated This constructor is not performance-friendly.
   * Please use {@link #BehaviorRegistry(Map)} instead.
   */
  @Deprecated
  public BehaviorRegistry(List<Behavior> behaviorList) {
    mBehaviorMap = new HashMap<>();
    addBehaviors(behaviorList);
  }

  @Deprecated
  public void add(List<Behavior> behaviorList) {
    addBehaviors(behaviorList);
  }

  public void addBehaviors(List<Behavior> behaviorList) {
    if (behaviorList == null) {
      return;
    }
    for (Behavior behavior : behaviorList) {
      addBehavior(behavior);
    }
  }

  public void addBehavior(Behavior behavior) {
    if (behavior == null) {
      return;
    }
    String name = behavior.getName();
    Behavior oldBehavior = mBehaviorMap.get(name);
    if (oldBehavior != null) {
      Log.d("LynxError",
          "Duplicated Behavior For Name: " + name + ", " + oldBehavior + " will be override");
    }
    mBehaviorMap.put(name, behavior);
  }

  public Behavior get(String className) {
    Behavior viewManager = mBehaviorMap.get(className);
    if (viewManager != null) {
      return viewManager;
    }
    throw new RuntimeException("No BehaviorController defined for class " + className);
  }

  public Set<String> getAllBehaviorRegistryName() {
    Set<String> registeredBehavior = new HashSet<String>();
    for (Behavior behavior : mBehaviorMap.values()) {
      registeredBehavior.add(behavior.getName());
    }
    return registeredBehavior;
  }
}
