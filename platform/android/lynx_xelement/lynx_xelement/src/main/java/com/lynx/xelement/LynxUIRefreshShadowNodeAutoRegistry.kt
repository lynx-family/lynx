// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

package com.lynx.xelement

import com.lynx.tasm.behavior.LynxShadowNode
import com.lynx.xelement.refresh.LynxUIRefreshShadowNode

@LynxShadowNode(tagName = "refresh")
class LynxUIRefreshShadowNodeAutoRegistry: LynxUIRefreshShadowNode() {}
