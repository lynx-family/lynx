// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.devtool.recorder;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;

public class LynxRecorderReplayOptionsTest {
  private Boolean parseEnableBTSOverride(String query) {
    QueryMapUtils queryMap = new QueryMapUtils();
    queryMap.parse("sslocal://arkview?url=https://example.com/replay.json" + query);
    return LynxRecorderActionManager.parseEnableBTSOverride(queryMap);
  }

  @Test
  public void missingEnableBTSDoesNotOverrideRecording() {
    assertNull(parseEnableBTSOverride(""));
    assertTrue(LynxRecorderActionManager.resolveEnableBTS(null, true, false));
    assertFalse(LynxRecorderActionManager.resolveEnableBTS(null, false, false));
  }

  @Test
  public void enableBTSParsesZeroAndOne() {
    assertFalse(parseEnableBTSOverride("&enable_bts=0"));
    assertTrue(parseEnableBTSOverride("&enable_bts=1"));
  }

  @Test
  public void enableBTSOverrideTakesPrecedenceOverRecordingAndLegacyAirStrictMode() {
    assertTrue(LynxRecorderActionManager.resolveEnableBTS(true, false, true));
    assertFalse(LynxRecorderActionManager.resolveEnableBTS(false, true, false));
  }

  @Test
  public void legacyAirStrictModeStillDisablesRecordedBTS() {
    assertFalse(LynxRecorderActionManager.resolveEnableBTS(null, true, true));
  }
}
