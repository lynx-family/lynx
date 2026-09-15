// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling

import android.content.Context
import android.content.res.Configuration
import com.lynx.explorer.LynxViewShellActivity
import com.lynx.tasm.LynxColorScheme

object ExplorerAppearance {
  const val KEY = "preferredTheme"

  @JvmStatic
  fun read(context: Context): String {
    return context
      .getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE)
      .getString(KEY, "Auto") ?: "Auto"
  }

  @JvmStatic
  fun write(context: Context, value: String) {
    context
      .getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE)
      .edit()
      .putString(KEY, value)
      .apply()
  }

  @JvmStatic
  fun resolveColorScheme(context: Context, preference: String?): LynxColorScheme {
    return when (preference?.lowercase()) {
      "dark" -> LynxColorScheme.DARK
      "light" -> LynxColorScheme.LIGHT
      else -> if (
        context.resources.configuration.uiMode and Configuration.UI_MODE_NIGHT_MASK ==
          Configuration.UI_MODE_NIGHT_YES
      ) {
        LynxColorScheme.DARK
      } else {
        LynxColorScheme.LIGHT
      }
    }
  }
}
