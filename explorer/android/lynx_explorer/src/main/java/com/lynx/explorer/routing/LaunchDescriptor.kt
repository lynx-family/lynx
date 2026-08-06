// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routing

enum class RequestedRuntime { AUTOMATIC, LYNX, SPARKLING }
enum class ResolvedRuntime { LYNX, SPARKLING }
enum class RouteSource { MANUAL_INPUT, SCANNER, STARTUP, EXTERNAL_URL, DEBUG_BRIDGE, DEVTOOL, NATIVE_MODULE, SPARKLING_ROUTER, RECORDER }

sealed class LaunchResource {
  data class LocalBundle(val path: String) : LaunchResource()
  data class RemoteBundle(val url: String) : LaunchResource()
  data class Recorder(val url: String) : LaunchResource()
}

data class LaunchQueryItem(val name: String, val value: String?)
data class ViewportOptions(val widthPixels: Int, val heightPixels: Int)
data class AppearanceOptions(
  val backgroundColor: String?,
  val transparentStatusBar: Boolean,
  val statusBarHidden: Boolean,
  val forceThemeStyle: String?
)
data class NavigationOptions(
  val hidden: Boolean,
  val fullScreen: Boolean,
  val title: String?,
  val titleColor: String?,
  val barColor: String?,
  val backButtonStyle: String?,
  val orientation: String?
)

/** Immutable, SDK-neutral meaning of one Explorer launch request. */
data class LaunchDescriptor(
  val originalInput: String,
  val resource: LaunchResource,
  val pageName: String?,
  val viewport: ViewportOptions?,
  val appearance: AppearanceOptions,
  val navigation: NavigationOptions,
  val animated: Boolean,
  val enableNapiAddon: Boolean,
  val queryItems: List<LaunchQueryItem>,
  val extras: Map<String, String>,
  val pageGlobalProps: Map<String, Any>,
  val requestedRuntime: RequestedRuntime,
  val runtime: ResolvedRuntime,
  val source: RouteSource,
  val canonicalSparklingScheme: String?
) {
  fun withExtras(values: Map<*, *>?): LaunchDescriptor {
    val merged = extras.toMutableMap()
    values?.forEach { (key, value) ->
      val normalizedKey = key?.toString() ?: return@forEach
      merged[normalizedKey] = value?.toString() ?: ""
    }
    return copy(extras = merged)
  }
}
