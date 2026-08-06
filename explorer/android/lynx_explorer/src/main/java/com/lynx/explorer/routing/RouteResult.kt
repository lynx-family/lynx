// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
package com.lynx.explorer.routing

data class RouteResult(val accepted: Boolean, val code: String, val message: String) {
  companion object {
    @JvmStatic fun accepted() = RouteResult(true, "accepted", "Route accepted.")
    @JvmStatic fun failure(code: String, message: String) = RouteResult(false, code, message)
  }
}

class RouteException(val code: String, override val message: String) : IllegalArgumentException(message)
