// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routing

import android.content.Intent
import androidx.activity.ComponentActivity
import androidx.activity.OnBackPressedCallback
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import org.robolectric.RuntimeEnvironment
import org.robolectric.Shadows.shadowOf
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(manifest = Config.NONE, sdk = [28])
class RouteCoordinatorTest {
  class BackActivity : ComponentActivity()

  @Test fun sparklingFailureNeverFallsBackToLynx() {
    var lynxCalls = 0; var sparklingCalls = 0
    val engine = RouteCoordinatorEngine(LaunchDescriptorParser(),
      { lynxCalls++; RouteResult.accepted() },
      { sparklingCalls++; RouteResult.failure("sparkling_launch_failed", "failed") })
    val result = engine.open("hybrid://lynxview_page?bundle=main.lynx.bundle", RequestedRuntime.AUTOMATIC, RouteSource.NATIVE_MODULE)
    assertFalse(result.accepted)
    assertEquals("sparkling_launch_failed", result.code)
    assertEquals(0, lynxCalls)
    assertEquals(1, sparklingCalls)
  }

  @Test fun automaticRawRouteUsesOnlyLynx() {
    var lynxCalls = 0; var sparklingCalls = 0
    val engine = RouteCoordinatorEngine(LaunchDescriptorParser(),
      { lynxCalls++; RouteResult.accepted() },
      { sparklingCalls++; RouteResult.accepted() })
    assertTrue(engine.open("https://example.com/a.bundle", RequestedRuntime.AUTOMATIC, RouteSource.SCANNER).accepted)
    assertEquals(1, lynxCalls); assertEquals(0, sparklingCalls)
  }

  @Test fun sparklingRouterExtrasAreNormalizedAndForwarded() {
    var launched: LaunchDescriptor? = null
    val engine = RouteCoordinatorEngine(
      LaunchDescriptorParser(),
      { RouteResult.failure("unexpected_lynx_launch", "failed") },
      { descriptor -> launched = descriptor; RouteResult.accepted() })
    val result = engine.open(
      "hybrid://lynxview_page?bundle=main.lynx.bundle&source=query",
      RequestedRuntime.SPARKLING,
      RouteSource.SPARKLING_ROUTER,
      mapOf<Any?, Any?>(
        "source" to "router",
        "count" to 2,
        "empty" to null,
        null to "ignored"))

    assertTrue(result.accepted)
    assertEquals("router", launched!!.extras["source"])
    assertEquals("2", launched!!.extras["count"])
    assertEquals("", launched!!.extras["empty"])
    assertFalse(launched!!.extras.containsKey("null"))
  }

  @Test fun devToolLaunchPreservesClearTopPolicy() {
    val context = RuntimeEnvironment.getApplication()
    val descriptor = LaunchDescriptorParser().parse(
      "https://example.com/a.bundle",
      RequestedRuntime.AUTOMATIC,
      RouteSource.DEVTOOL)

    assertTrue(LynxContainerLauncher.open(context, descriptor).accepted)
    val intent = shadowOf(context).nextStartedActivity
    val expectedFlags = Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK
    assertEquals(expectedFlags, intent.flags and expectedFlags)
  }

  @Test fun navigateBackUsesComponentActivityDispatcher() {
    val activity = BackActivity()
    var dispatched = false
    activity.onBackPressedDispatcher.addCallback(object : OnBackPressedCallback(true) {
      override fun handleOnBackPressed() { dispatched = true }
    })
    assertTrue(RouteCoordinator.navigateBack(activity).accepted)
    assertTrue(dispatched)
  }
}
