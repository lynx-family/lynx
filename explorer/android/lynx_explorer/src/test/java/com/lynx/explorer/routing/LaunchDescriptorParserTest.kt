// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routing

import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class LaunchDescriptorParserTest {
  private val parser = LaunchDescriptorParser()

  @Test fun rawRemoteDefaultsToLynxAndPreservesUnknownItems() {
    val route = parser.parse("https://example.com/main.lynx.bundle?x=1&x=2&flag", RequestedRuntime.AUTOMATIC, RouteSource.MANUAL_INPUT)
    assertEquals(ResolvedRuntime.LYNX, route.runtime)
    assertEquals(listOf("1", "2"), route.queryItems.filter { it.name == "x" }.map { it.value })
    assertNull(route.queryItems.last().value)
    assertEquals("2", route.extras["x"])
  }

  @Test fun explicitSparklingMapsRawLocalRouteAndOptions() {
    val route = parser.parse("file://lynx?local://extensions/sparkling-go/main.lynx.bundle?fullscreen=true&title=Go&width=720&height=1280", RequestedRuntime.SPARKLING, RouteSource.NATIVE_MODULE)
    assertEquals(ResolvedRuntime.SPARKLING, route.runtime)
    assertEquals(LaunchResource.LocalBundle("extensions/sparkling-go/main.lynx.bundle"), route.resource)
    assertTrue(route.navigation.hidden)
    assertEquals(ViewportOptions(720, 1280), route.viewport)
    assertTrue(SparklingSchemeAdapter.canonicalScheme(route).startsWith("hybrid://lynxview_page?"))
  }

  @Test fun legacyLocalAmpersandOptionsDoNotBecomePartOfTheBundlePath() {
    val route = parser.parse(
      "file://lynx?local://extensions/sparkling-go/nav-chain.lynx.bundle&fullscreen=true&force_theme_style=light",
      RequestedRuntime.SPARKLING,
      RouteSource.MANUAL_INPUT)
    assertEquals(
      LaunchResource.LocalBundle("extensions/sparkling-go/nav-chain.lynx.bundle"),
      route.resource)
    assertTrue(route.navigation.fullScreen)
    assertEquals("light", route.appearance.forceThemeStyle)
    val canonical = android.net.Uri.parse(SparklingSchemeAdapter.canonicalScheme(route))
    assertEquals(
      "extensions/sparkling-go/nav-chain.lynx.bundle",
      canonical.getQueryParameter("bundle"))
    assertEquals("true", canonical.getQueryParameter("fullscreen"))
    assertEquals("light", canonical.getQueryParameter("force_theme_style"))
  }

  @Test fun canonicalRouteAlwaysOwnsRequest() {
    val route = parser.parse("hybrid://lynxview_page?bundle=main.lynx.bundle&hide_nav_bar=1", RequestedRuntime.LYNX, RouteSource.EXTERNAL_URL)
    assertEquals(ResolvedRuntime.SPARKLING, route.runtime)
    assertEquals("hybrid://lynxview_page?bundle=main.lynx.bundle&hide_nav_bar=1", route.canonicalSparklingScheme)
  }

  @Test fun wrapperUnwrapsCanonicalAndRetainsOriginalInput() {
    val input = "lynx://open?url=hybrid%3A%2F%2Flynxview_page%3Fbundle%3Dmain.lynx.bundle"
    val route = parser.parse(input, RequestedRuntime.AUTOMATIC, RouteSource.DEBUG_BRIDGE)
    assertEquals(input, route.originalInput)
    assertEquals(ResolvedRuntime.SPARKLING, route.runtime)
  }

  @Test fun sharedSparklingGoResourceIsRetargetedToAndroidAssets() {
    val route = parser.parse("hybrid://lynxview_page?bundle=.%2FResource%2Fextensions%2Fsparkling-go%2Fmain.lynx.bundle", RequestedRuntime.SPARKLING, RouteSource.MANUAL_INPUT)
    assertEquals("extensions/sparkling-go/main.lynx.bundle",
      android.net.Uri.parse(SparklingSchemeAdapter.canonicalScheme(route)).getQueryParameter("bundle"))
  }

  @Test fun malformedAndRecorderFailuresAreTyped() {
    assertCode("invalid_percent_encoding") { parser.parse("https://example.com/%ZZ", RequestedRuntime.LYNX, RouteSource.MANUAL_INPUT) }
    assertCode("invalid_percent_encoding") { parser.parse("https://example.com/%C3%28", RequestedRuntime.LYNX, RouteSource.MANUAL_INPUT) }
    assertCode("recorder_unsupported_in_sparkling") { parser.parse("sslocal://arkview?url=x", RequestedRuntime.SPARKLING, RouteSource.RECORDER) }
    assertCode("ambiguous_target") { parser.parse("hybrid://lynxview_page?bundle=a&url=https%3A%2F%2Fe.com%2Fb", RequestedRuntime.AUTOMATIC, RouteSource.MANUAL_INPUT) }
    assertCode("ambiguous_target") { parser.parse("lynx://open?url=https://example.com/a&url=https://example.com/b", RequestedRuntime.AUTOMATIC, RouteSource.MANUAL_INPUT) }
  }

  @Test fun explicitSparklingRejectsEveryOrientationValue() {
    listOf("portrait", "landscape", "upside-down").forEach { orientation ->
      val route = parser.parse(
        "https://example.com/main.lynx.bundle?orientation=$orientation",
        RequestedRuntime.SPARKLING,
        RouteSource.MANUAL_INPUT)
      assertCode("sparkling_option_unsupported") {
        SparklingSchemeAdapter.canonicalScheme(route)
      }
    }
  }

  @Test fun sparklingAdapterPreservesRepeatedValuelessAndEncodedItems() {
    val route = parser.parse(
      "assets://demo/main.lynx.bundle?tag=one&flag&tag=two&encoded=%252F",
      RequestedRuntime.SPARKLING, RouteSource.MANUAL_INPUT)
    val adapted = android.net.Uri.parse(SparklingSchemeAdapter.canonicalScheme(route))
    assertEquals("demo/main.lynx.bundle", adapted.getQueryParameter("bundle"))
    assertEquals(listOf("one", "two"), adapted.getQueryParameters("tag"))
    assertTrue(adapted.encodedQuery!!.split('&').contains("flag"))
    assertEquals("%2F", adapted.getQueryParameter("encoded"))
  }

  private fun assertCode(code: String, block: () -> Unit) {
    try { block(); fail("Expected $code") } catch (error: RouteException) { assertEquals(code, error.code) }
  }
}
