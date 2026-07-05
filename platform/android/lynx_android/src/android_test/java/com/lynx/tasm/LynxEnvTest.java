// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.tasm;

import static org.junit.Assert.assertEquals;

import android.app.Application;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class LynxEnvTest {
  @Test
  public void getLynxVersionUsesNativeVersionWhenNativeLibraryLoaded() {
    Application application = ApplicationProvider.getApplicationContext();
    LynxEnv.inst().init(application, System::loadLibrary, null, null, null);

    assertEquals(LynxEnv.inst().nativeGetLynxVersion(), LynxEnv.inst().getLynxVersion());
  }
}
