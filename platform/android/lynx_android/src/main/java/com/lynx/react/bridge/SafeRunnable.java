/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.react.bridge;

import com.lynx.tasm.behavior.ExceptionHandler;

/**
 * A decorator class that wraps a {@link Runnable} implementation and provides
 * a safe execution environment by handling any {@link RuntimeException} that
 * may be thrown during the execution of the {@link Runnable}'s {@code run()} method.
 */
public abstract class SafeRunnable implements Runnable {
  private final ExceptionHandler mExceptionHandler;

  public SafeRunnable(ExceptionHandler context) {
    mExceptionHandler = context;
  }

  @Override
  public final void run() {
    try {
      unsafeRun();
    } catch (RuntimeException e) {
      mExceptionHandler.handleException(e);
    }
  }

  public abstract void unsafeRun();
}
