// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routing

import android.content.Context
import com.lynx.explorer.LynxViewShellActivity
import org.json.JSONArray
import org.json.JSONObject

object RecentRouteHistory {
  private const val KEY = "explorerNativeLaunchSessionsV1"
  private const val MAX_ITEMS = 20

  fun record(context: Context, descriptor: LaunchDescriptor) {
    val preferences = context.getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE)
    val existing = try { JSONArray(preferences.getString(KEY, "[]")) } catch (_: Exception) { JSONArray() }
    val runtime = if (descriptor.runtime == ResolvedRuntime.SPARKLING) "sparkling" else "lynx"
    val source = when (descriptor.source) {
      RouteSource.SCANNER -> "scan"
      RouteSource.EXTERNAL_URL, RouteSource.DEBUG_BRIDGE, RouteSource.STARTUP -> "external"
      RouteSource.SPARKLING_ROUTER -> "showcase"
      else -> "input"
    }
    val session = JSONObject()
      .put("id", "${System.currentTimeMillis()}-$runtime-${descriptor.originalInput}")
      .put("url", descriptor.originalInput)
      .put("runtime", runtime)
      .put("source", source)
      .put("openedAt", System.currentTimeMillis())
      .put("fullscreen", descriptor.navigation.fullScreen)
      .put("hiddenNav", descriptor.navigation.hidden)
      .put("theme", descriptor.appearance.forceThemeStyle ?: JSONObject.NULL)
    val next = JSONArray().put(session)
    var index = 0
    while (index < existing.length() && next.length() < MAX_ITEMS) {
      val item = existing.optJSONObject(index++) ?: continue
      if (item.optString("url") == descriptor.originalInput && item.optString("runtime") == runtime) continue
      next.put(item)
    }
    preferences.edit().putString(KEY, next.toString()).apply()
  }
}
