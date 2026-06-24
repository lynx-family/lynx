// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.animax.setter;

import androidx.annotation.NonNull;
import com.lynx.animax.AnimaXPlayer;
import com.lynx.animax.util.AnimaXLog;
import java.lang.ref.WeakReference;
import java.util.PriorityQueue;

public class UIAnimaXPropsPrioritySetter {
  private static final String TAG = "UIAnimaXPropsPrioritySetter";
  private final PriorityQueue<PrioritizedTask> prioritizedTasks = new PriorityQueue<>();
  private WeakReference<AnimaXPlayer> mPlayer;

  @FunctionalInterface
  public interface PlayerTask {
    void run(@NonNull AnimaXPlayer player) throws Exception;
  }

  private static class PrioritizedTask
      implements Comparable<UIAnimaXPropsPrioritySetter.PrioritizedTask> {
    final PlayerTask task;
    final ExecutionPriority priority;
    PrioritizedTask(PlayerTask task, ExecutionPriority priority) {
      this.task = task;
      this.priority = priority;
    }
    @Override
    public int compareTo(UIAnimaXPropsPrioritySetter.PrioritizedTask other) {
      return Integer.compare(this.priority.getValue(), other.priority.getValue());
    }
  }

  public UIAnimaXPropsPrioritySetter() {}

  public void init(AnimaXPlayer player) {
    mPlayer = new WeakReference<>(player);
  }

  public enum ExecutionPriority {
    HIGH(-100),
    DEFAULT(0),
    LOW(100);

    private final int value;
    ExecutionPriority(int value) {
      this.value = value;
    }

    public int getValue() {
      return this.value;
    }
  }

  public void enqueueTask(PlayerTask task) {
    enqueueTask(task, ExecutionPriority.DEFAULT);
  }

  public void enqueueTask(PlayerTask task, ExecutionPriority priority) {
    PrioritizedTask prioritizedTask = new PrioritizedTask(task, priority);
    this.prioritizedTasks.add(prioritizedTask);
  }

  public void flush() {
    final AnimaXPlayer player = mPlayer.get();
    if (player == null) {
      return;
    }
    while (!prioritizedTasks.isEmpty()) {
      PrioritizedTask prioritizedTask = prioritizedTasks.poll();
      try {
        if (prioritizedTask != null) {
          prioritizedTask.task.run(player);
        }
      } catch (Exception e) {
        AnimaXLog.e(TAG, "Failed to run pending tasks with exception: " + e.getMessage());
      }
    }
  }
}
