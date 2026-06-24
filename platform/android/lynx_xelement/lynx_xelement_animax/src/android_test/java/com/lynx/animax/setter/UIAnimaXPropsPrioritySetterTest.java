// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.setter;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import com.lynx.animax.AnimaXPlayer;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;

public class UIAnimaXPropsPrioritySetterTest {
  private UIAnimaXPropsPrioritySetter setter;
  private List<String> executedTasks;
  @Before
  public void setUp() {
    setter = new UIAnimaXPropsPrioritySetter();
    setter.init(mock(AnimaXPlayer.class));
    executedTasks = new ArrayList<>();
  }

  @Test
  public void testExecutionPendingOrder() {
    assertTrue(UIAnimaXPropsPrioritySetter.ExecutionPriority.HIGH.getValue()
        < UIAnimaXPropsPrioritySetter.ExecutionPriority.DEFAULT.getValue());
    assertTrue(UIAnimaXPropsPrioritySetter.ExecutionPriority.DEFAULT.getValue()
        < UIAnimaXPropsPrioritySetter.ExecutionPriority.LOW.getValue());
  }

  @Test
  public void testEnqueueTaskAndFlushExecutesTasksInPendingOrder() {
    setter.enqueueTask(player -> executedTasks.add("default"));
    setter.enqueueTask(
        player -> executedTasks.add("high"), UIAnimaXPropsPrioritySetter.ExecutionPriority.HIGH);
    setter.enqueueTask(
        player -> executedTasks.add("low"), UIAnimaXPropsPrioritySetter.ExecutionPriority.LOW);
    setter.enqueueTask(player -> executedTasks.add("default"));
    setter.flush();
    List<String> expectedOrder = List.of("high", "default", "default", "low");
    assertEquals(expectedOrder, executedTasks);
  }
  @Test
  public void testFlushWithNoTasksDoesNotThrow() {
    setter.flush();
    assertTrue(executedTasks.isEmpty());
  }
}
