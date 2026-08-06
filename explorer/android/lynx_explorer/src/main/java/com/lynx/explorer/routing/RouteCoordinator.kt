// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routing

import android.app.Activity
import android.content.Context
import android.os.Handler
import android.os.Looper
import androidx.activity.ComponentActivity
import com.lynx.explorer.sparkling.RuntimeExtensionProvider

object RouteCoordinator {
  private val parser = LaunchDescriptorParser()

  @JvmStatic @JvmOverloads
  fun open(
    context: Context,
    url: String,
    runtime: RequestedRuntime = RequestedRuntime.AUTOMATIC,
    source: RouteSource = RouteSource.NATIVE_MODULE,
    extras: Map<*, *>? = null
  ): RouteResult {
    if (Looper.myLooper() != Looper.getMainLooper()) return RouteResult.failure("main_thread_required", "Route launches must run on the main thread.")
    val descriptor = try {
      parser.parse(url, runtime, source).withExtras(extras)
    } catch (error: RouteException) {
      return RouteResult.failure(error.code, error.message)
    }
    val result = when (descriptor.runtime) {
      ResolvedRuntime.LYNX -> LynxContainerLauncher.open(context, descriptor)
      ResolvedRuntime.SPARKLING -> RuntimeExtensionProvider.open(context, descriptor)
    }
    if (result.accepted) RecentRouteHistory.record(context, descriptor)
    return result
  }

  @JvmStatic fun openAsync(context: Context, url: String, runtime: RequestedRuntime, source: RouteSource, callback: (RouteResult) -> Unit) {
    val once = OnceCallback(callback)
    Handler(Looper.getMainLooper()).post { once.complete(open(context, url, runtime, source)) }
  }

  @JvmStatic fun navigateBack(context: Context): RouteResult {
    val activity = context as? Activity ?: return RouteResult.failure("route_owner_unavailable", "No owning Activity is available.")
    if (activity.isFinishing) return RouteResult.failure("route_owner_unavailable", "The owning Activity is closing.")
    if (activity is ComponentActivity) {
      activity.onBackPressedDispatcher.onBackPressed()
    } else {
      navigateBackOnPlainActivity(activity)
    }
    return RouteResult.accepted()
  }

  @Suppress("DEPRECATION")
  private fun navigateBackOnPlainActivity(activity: Activity) = activity.onBackPressed()

  private class OnceCallback(private val callback: (RouteResult) -> Unit) {
    private var completed = false
    @Synchronized fun complete(result: RouteResult) { if (!completed) { completed = true; callback(result) } }
  }
}

/** Pure coordinator core used to prove ownership and no-fallback behavior. */
class RouteCoordinatorEngine(
  private val parser: LaunchDescriptorParser,
  private val lynxLauncher: (LaunchDescriptor) -> RouteResult,
  private val sparklingLauncher: (LaunchDescriptor) -> RouteResult
) {
  fun open(
    url: String,
    runtime: RequestedRuntime,
    source: RouteSource,
    extras: Map<*, *>? = null
  ): RouteResult {
    val descriptor = try {
      parser.parse(url, runtime, source).withExtras(extras)
    } catch (error: RouteException) {
      return RouteResult.failure(error.code, error.message)
    }
    return when (descriptor.runtime) {
      ResolvedRuntime.LYNX -> lynxLauncher(descriptor)
      ResolvedRuntime.SPARKLING -> sparklingLauncher(descriptor)
    }
  }
}
