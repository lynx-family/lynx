// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.sparkling

import android.app.Application
import android.content.Context
import com.lynx.explorer.routing.LaunchDescriptor
import com.lynx.explorer.routing.RouteResult

interface SparklingRuntimeExtension {
  val supported: Boolean
  fun initialize(application: Application)
  fun open(context: Context, descriptor: LaunchDescriptor): RouteResult
}
