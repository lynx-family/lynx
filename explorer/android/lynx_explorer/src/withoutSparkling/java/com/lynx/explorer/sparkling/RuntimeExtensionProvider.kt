// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling

import android.app.Application
import android.content.Context
import com.lynx.explorer.routing.LaunchDescriptor
import com.lynx.explorer.routing.RouteResult

object RuntimeExtensionProvider : SparklingRuntimeExtension {
  override val supported = false
  override fun initialize(application: Application) = Unit
  override fun open(context: Context, descriptor: LaunchDescriptor) =
    RouteResult.failure("sparkling_unavailable", "Sparkling is not available in this Lynx Explorer build.")
}
