// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routing

import android.content.Context
import android.content.Intent
import android.net.Uri
import com.lynx.explorer.LynxViewShellActivity
import com.lynx.explorer.shell.TemplateDispatcher

object LynxContainerLauncher {
  fun open(context: Context, descriptor: LaunchDescriptor): RouteResult {
    if (descriptor.runtime != ResolvedRuntime.LYNX) return RouteResult.failure("container_mismatch", "The Lynx launcher cannot open a Sparkling route.")
    val url = when (val resource = descriptor.resource) {
      is LaunchResource.LocalBundle -> "file://lynx?local://${resource.path}" + querySuffix(descriptor.queryItems)
      is LaunchResource.RemoteBundle -> resource.url
      is LaunchResource.Recorder -> resource.url
    }
    val flags = when {
      descriptor.source == RouteSource.DEVTOOL ->
        Intent.FLAG_ACTIVITY_CLEAR_TOP or Intent.FLAG_ACTIVITY_NEW_TASK
      context is android.app.Activity -> 0
      else -> Intent.FLAG_ACTIVITY_NEW_TASK
    }
    return if (TemplateDispatcher.dispatchUrl(context, url, flags)) RouteResult.accepted()
    else RouteResult.failure("unsupported_route", "No Lynx loader accepts this route.")
  }

  private fun querySuffix(items: List<LaunchQueryItem>): String {
    if (items.isEmpty()) return ""
    return "?" + items.joinToString("&") { Uri.encode(it.name) + (it.value?.let { value -> "=" + Uri.encode(value) } ?: "") }
  }
}
