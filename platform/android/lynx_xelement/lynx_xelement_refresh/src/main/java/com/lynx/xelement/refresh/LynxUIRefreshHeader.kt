// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement.refresh

import android.content.Context
import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName
import com.lynx.tasm.behavior.ui.UIGroup

@LynxGeneratorName(packageName = "com.lynx.xelement.refresh")
@LynxBehavior(tagName = ["refresh-header"], isCreateAsync = false)
open class LynxUIRefreshHeader(context: LynxContext?, params: Any?): UIGroup<LynxRefreshHeaderView>(context, params) {
  constructor(context: LynxContext) : this(context, null)

  override fun createView(context: Context?): LynxRefreshHeaderView {
    return LynxRefreshHeaderView(context)
  }
}
