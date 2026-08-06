// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling

import android.content.Context
import com.lynx.explorer.BuildConfig
import com.lynx.explorer.LynxViewShellActivity

object ExplorerRuntimePreferences {
  const val KEY = "preferredContainer"

  @JvmStatic
  fun read(context: Context): String {
    val preferred = context.getSharedPreferences(
      LynxViewShellActivity.PREFERENCES,
      Context.MODE_PRIVATE
    ).getString(KEY, "legacy")
    return if (BuildConfig.ENABLE_SPARKLING && preferred == "sparkling") {
      "sparkling"
    } else {
      "legacy"
    }
  }
}
