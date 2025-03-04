// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.testing.base.instrumentation;

import org.junit.runner.Description;
import org.junit.runner.notification.Failure;

public class CaseResult {
  public enum Status { STARTED, FAILURE, IGNORED, PASSED }

  private final Description descriptor;

  private Status status;

  private Failure failure;

  private long startTime = 0;
  private long endTime = 0;

  public CaseResult(Description descriptor) {
    this.descriptor = descriptor;

    status = Status.STARTED;
    startTime = System.currentTimeMillis();
  }

  public void recordFailure(Failure failure) {
    this.failure = failure;
    this.status = Status.FAILURE;
  }

  public void recordTestIgnored() {
    this.status = Status.IGNORED;
  }

  public void recordFinished() {
    if (this.status == Status.STARTED) {
      this.status = Status.PASSED;
    }

    endTime = System.currentTimeMillis();
  }

  public long getElapsedTime() {
    long endTime = this.endTime;

    if (endTime == 0) {
      endTime = System.currentTimeMillis();
    }

    return endTime - startTime;
  }

  public long getEndTime() {
    return this.endTime;
  }

  public Status getStatus() {
    return status;
  }

  public Description getDescriptor() {
    return descriptor;
  }

  public Failure getFailure() {
    return failure;
  }
}
