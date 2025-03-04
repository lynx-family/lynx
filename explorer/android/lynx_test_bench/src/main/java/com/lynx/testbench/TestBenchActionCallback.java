// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.testbench;

import android.content.Context;
import android.content.Intent;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import com.lynx.tasm.LynxView;
import com.lynx.tasm.LynxViewBuilder;

public interface TestBenchActionCallback {
  /**
   * LynxView will be created..
   */
  default void onLynxViewWillBuild(TestBenchActionManager manager, LynxViewBuilder builder) {}

  /**
   * LynxView has been created.
   */
  default void onLynxViewDidBuild(@NonNull LynxView kitView, @NonNull Intent intent,
      @NonNull Context context, @NonNull ViewGroup view) {}

  /**
   * Callback indicating the completion of TestBench playback.
   * @param endType The type of playback completion:
   * 1. sEndForFirstScreen(0): LynxView first screen rendering completed.
   * 2. sEndForAll(1): The first screen has finished loading, and related events (sendGlobalEvent,
   * touchEvent, updateDataByPreParsedData) have been dispatched.
   */
  default void onReplayFinish(int endType) {}

  /**
   * All actions playback in TestBench have been completed.
   */
  default void onTestBenchComplete() {}
}
