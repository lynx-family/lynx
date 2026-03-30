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
import com.lynx.tasm.LynxSubErrorCode;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

@RunWith(AndroidJUnit4.class)
public class DevToolSettingsTest {
  private static final String SP_KEY_ACTIVATED_CDP_DOMAINS = "activated_cdp_domains";
  private static final String SP_KEY_IGNORE_ERROR_TYPES = "ignore_error_types";
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
    mSettings.syncToNative();
  }

  @Test
  public void testSingleton() {
    DevToolSettings anotherInstance = DevToolSettings.inst();
    assertEquals(mSettings, anotherInstance);
  }

  @Test
  public void testDevToolEnabled() {
    // Should be false by default
    assertFalse(mSettings.isDevToolEnabled());

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

  @Test
  public void testLogBoxEnabled() {
    // Default true
    assertTrue(mSettings.isLogBoxEnabled());

    mSettings.setLogBoxEnabled(false);
    assertFalse(mSettings.isLogBoxEnabled());

    SharedPreferences sp = mContext.getSharedPreferences("lynx_env_config", Context.MODE_PRIVATE);
    assertFalse(sp.getBoolean(DevToolSettings.SP_KEY_ENABLE_LOGBOX, true));
  }

  @Test
  public void testHighlightTouchEnabled() {
    // Default false
    assertFalse(mSettings.isHighlightTouchEnabled());

    mSettings.setHighlightTouchEnabled(true);
    assertTrue(mSettings.isHighlightTouchEnabled());

    // Should NOT be persisted
    SharedPreferences sp = mContext.getSharedPreferences("lynx_env_config", Context.MODE_PRIVATE);
    assertFalse(sp.contains(DevToolSettings.SP_KEY_ENABLE_HIGHLIGHT_TOUCH));
  }

  @Test
  public void testPreviewScreenshotEnabled() {
    // Default true
    assertTrue(mSettings.isPreviewScreenshotEnabled());

    mSettings.setPreviewScreenshotEnabled(false);
    assertFalse(mSettings.isPreviewScreenshotEnabled());

    // Should NOT be persisted
    SharedPreferences sp = mContext.getSharedPreferences("lynx_env_config", Context.MODE_PRIVATE);
    assertFalse(sp.contains(DevToolSettings.SP_KEY_ENABLE_PREVIEW_SCREEN_SHOT));
  }

  @Test
  public void testV8Enabled() {
    // Default V8_ALIGN_WITH_PROD (2)
    assertEquals(DevToolSettings.V8_ALIGN_WITH_PROD, mSettings.getV8Enabled());

    mSettings.setV8Enabled(DevToolSettings.V8_ON);
    assertEquals(DevToolSettings.V8_ON, mSettings.getV8Enabled());

    SharedPreferences sp = mContext.getSharedPreferences("lynx_env_config", Context.MODE_PRIVATE);
    assertEquals(DevToolSettings.V8_ON, sp.getInt(DevToolSettings.SP_KEY_ENABLE_V8, -1));
  }

  @Test
  public void testCSSErrorIgnored() {
    assertFalse(mSettings.isCSSErrorIgnored());

    mSettings.setCSSErrorIgnored(true);
    assertTrue(mSettings.isCSSErrorIgnored());
    assertTrue(mSettings.isErrorTypeIgnored(LynxSubErrorCode.E_CSS));

    SharedPreferences sp = mContext.getSharedPreferences("lynx_env_config", Context.MODE_PRIVATE);
    assertTrue(new HashSet<>(sp.getStringSet(SP_KEY_IGNORE_ERROR_TYPES, new HashSet<>()))
                   .contains(String.valueOf(LynxSubErrorCode.E_CSS)));
  }

  @Test
  public void testIgnoredErrorTypes() {
    Set<String> ignoredErrorTypes =
        new HashSet<>(Arrays.asList(String.valueOf(LynxSubErrorCode.E_CSS)));
    mSettings.setIgnoredErrorTypes(ignoredErrorTypes);

    assertEquals(ignoredErrorTypes, mSettings.getIgnoredErrorTypes());

    mSettings.setErrorTypeIgnored(LynxSubErrorCode.E_CSS, false);
    assertFalse(mSettings.isErrorTypeIgnored(LynxSubErrorCode.E_CSS));
  }

  @Test
  public void testEnabledCDPDomains() {
    assertTrue(mSettings.getEnabledCDPDomains().isEmpty());

    Set<String> enabledDomains = new HashSet<>(Arrays.asList(
        DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_DOM, DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS,
        DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_PAGE));
    mSettings.setEnabledCDPDomains(enabledDomains);

    assertEquals(enabledDomains, mSettings.getEnabledCDPDomains());

    SharedPreferences sp = mContext.getSharedPreferences("lynx_env_config", Context.MODE_PRIVATE);
    assertEquals(enabledDomains,
        new HashSet<>(sp.getStringSet(SP_KEY_ACTIVATED_CDP_DOMAINS, new HashSet<>())));
  }

  @Test
  public void testSetCDPDomainEnabled() {
    assertFalse(mSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS));

    mSettings.setCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS, true);
    assertTrue(mSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS));

    mSettings.setCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS, false);
    assertFalse(mSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS));

    SharedPreferences sp = mContext.getSharedPreferences("lynx_env_config", Context.MODE_PRIVATE);
    assertFalse(new HashSet<>(sp.getStringSet(SP_KEY_ACTIVATED_CDP_DOMAINS, new HashSet<>()))
                    .contains(DevToolSettings.SP_KEY_ENABLE_CDP_DOMAIN_CSS));
  }

  @Test
  public void testSetEnabledCDPDomainsIgnoresNonCDPKeys() {
    mSettings.setEnabledCDPDomains(
        new HashSet<>(Arrays.asList(DevToolSettings.SP_KEY_ENABLE_DEVTOOL)));

    assertTrue(mSettings.getEnabledCDPDomains().isEmpty());
  }

  @Test
  public void testSetCDPDomainEnabledIgnoresNonCDPKeys() {
    mSettings.setCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_DEVTOOL, true);

    assertTrue(mSettings.getEnabledCDPDomains().isEmpty());
    assertFalse(mSettings.isCDPDomainEnabled(DevToolSettings.SP_KEY_ENABLE_DEVTOOL));
  }
}
