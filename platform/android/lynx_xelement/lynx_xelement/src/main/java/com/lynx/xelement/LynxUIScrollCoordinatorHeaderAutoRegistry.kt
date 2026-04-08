// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement

import com.lynx.tasm.behavior.LynxBehavior
import com.lynx.tasm.behavior.LynxContext
import com.lynx.tasm.behavior.LynxGeneratorName

@LynxGeneratorName(packageName = "com.lynx.xelement")
@LynxBehavior(tagName = ["scroll-coordinator-header"], isCreateAsync = true)
open class LynxUIScrollCoordinatorHeaderAutoRegistry(context: LynxContext) :
  com.lynx.xelement.scroll.coordinator.childitem.LynxUIScrollCoordinatorHeader(context) {}
