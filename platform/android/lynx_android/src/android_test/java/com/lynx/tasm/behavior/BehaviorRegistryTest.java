// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.tasm.behavior;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class BehaviorRegistryTest {
  private List<Behavior> behaviorList;
  private Map<String, Behavior> behaviorMap;
  @Before
  public void setUp() {
    behaviorList = new ArrayList<>();
    behaviorMap = new HashMap<>();
    List<Behavior> builtInBehaviors = new BuiltInBehavior().create();
    for (Behavior behavior : builtInBehaviors) {
      behaviorMap.put(behavior.getName(), behavior);
    }
    behaviorList.addAll(builtInBehaviors);
    List<Behavior> otherBehaviors = createOtherBehaviors();
    for (Behavior behavior : otherBehaviors) {
      behaviorMap.put(behavior.getName(), behavior);
    }
    behaviorList.addAll(otherBehaviors);
  }
  @Test
  public void testBehaviorRegistryByMap() {
    BehaviorRegistry listBehaviorRegistry = new BehaviorRegistry(behaviorList);
    BehaviorRegistry mapBehaviorRegistry = new BehaviorRegistry(behaviorMap);
    Set<String> listSet = listBehaviorRegistry.getAllBehaviorRegistryName();
    Set<String> mapSet = mapBehaviorRegistry.getAllBehaviorRegistryName();
    Assert.assertTrue(listSet.equals(mapSet));
  }

  private List<Behavior> createOtherBehaviors() {
    List<Behavior> behaviorsList = new ArrayList<>();
    // Add 100 behaviors
    for (int i = 1; i <= 100; i++) {
      behaviorsList.add(new Behavior("behavior_" + i));
    }
    // Add some repeated behaviors
    for (int i = 1; i <= 10; i++) {
      behaviorsList.add(new Behavior("behavior_" + i));
    }
    return behaviorsList;
  }
}
