// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routing

import android.net.Uri

object SparklingSchemeAdapter {
  fun canonicalScheme(descriptor: LaunchDescriptor): String {
    descriptor.canonicalSparklingScheme?.let { scheme ->
      val uri = Uri.parse(scheme)
      val bundle = uri.getQueryParameter("bundle")
      if (bundle?.startsWith("./Resource/") != true) return scheme
      return buildScheme(descriptor.queryItems.map { item ->
        if (item.name == "bundle") item.copy(value = item.value?.removePrefix("./Resource/")) else item
      })
    }
    if (descriptor.queryItems.any { it.name == "orientation" }) {
      throw RouteException("sparkling_option_unsupported", "Sparkling does not support the orientation launch option.")
    }
    val items = mutableListOf<LaunchQueryItem>()
    when (val resource = descriptor.resource) {
      is LaunchResource.LocalBundle -> items += LaunchQueryItem("bundle", resource.path)
      is LaunchResource.RemoteBundle -> items += LaunchQueryItem("url", resource.url)
      is LaunchResource.Recorder -> throw RouteException("recorder_unsupported_in_sparkling", "Recorder routes can only be opened with Lynx.")
    }
    items += descriptor.queryItems
    if (descriptor.navigation.hidden && descriptor.queryItems.none { it.name == "hide_nav_bar" }) items += LaunchQueryItem("hide_nav_bar", "1")
    if (descriptor.appearance.statusBarHidden && descriptor.queryItems.none { it.name == "hide_status_bar" }) items += LaunchQueryItem("hide_status_bar", "1")
    return buildScheme(items)
  }

  private fun buildScheme(items: List<LaunchQueryItem>): String =
    "hybrid://lynxview_page?" + items.joinToString("&") { item ->
      Uri.encode(item.name) + (item.value?.let { "=" + Uri.encode(it) } ?: "")
    }
}
