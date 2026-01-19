// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.devtoolwrapper;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.content.SharedPreferences;
import androidx.test.core.app.ApplicationProvider;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class DevToolSettingsTest {
  private DevToolSettings mSettings;
  private Context mContext;

  @Before
  public void setUp() {
    mContext = ApplicationProvider.getApplicationContext();
    // Clear shared preferences before each test
    SharedPreferences sp = mContext.getSharedPreferences("lynx_env_config", Context.MODE_PRIVATE);
    sp.edit().clear().apply();

    mSettings = DevToolSettings.inst();
    mSettings.init(mContext);
  }

  @Test
  public void testSingleton() {
    DevToolSettings anotherInstance = DevToolSettings.inst();
    assertEquals(mSettings, anotherInstance);
  }

  @Test
  public void testDevToolEnabledDefault() {
    // Should be false by default
    assertFalse(mSettings.isDevToolEnabled());
  }

  @Test
  public void testSetDevToolEnabled() {
    mSettings.setDevToolEnabled(true);
    assertTrue(mSettings.isDevToolEnabled());

    // Verify persistence
    SharedPreferences sp = mContext.getSharedPreferences("lynx_env_config", Context.MODE_PRIVATE);
    assertTrue(sp.getBoolean(DevToolSettings.SP_KEY_ENABLE_DEVTOOL, false));

    mSettings.setDevToolEnabled(false);
    assertFalse(mSettings.isDevToolEnabled());
    assertFalse(sp.getBoolean(DevToolSettings.SP_KEY_ENABLE_DEVTOOL, true));
  }

  @Test
  public void testInitWithNullContext() {
    // Should handle null context gracefully without crashing
    // Note: We cannot easily verify the internal state is unaffected since it's a singleton
    // already initialized in setUp(), but we can ensure it doesn't throw.
    mSettings.init(null);
  }
}
