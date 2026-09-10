// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling

import android.content.Context
import com.lynx.explorer.LynxViewShellActivity

object ExplorerAppearance {
  const val KEY = "preferredTheme"
  @JvmStatic fun read(context: Context): String = context.getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE).getString(KEY, "Auto") ?: "Auto"
  @JvmStatic fun write(context: Context, value: String) { context.getSharedPreferences(LynxViewShellActivity.PREFERENCES, Context.MODE_PRIVATE).edit().putString(KEY, value).apply() }
}
