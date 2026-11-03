// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IContext, PlatformGestureModifier } from '@lynx/lynx';

// Optional host callbacks leave the default renderer's Web events unchanged.
export class WebViewGestureModifier extends PlatformGestureModifier<WebAttribute> {
  private scrollY?: number;

  constructor(context: IContext) {
    super(context);
  }

  applyNormalAttribute(instance: WebAttribute): void {
    super.applyNormalAttribute(instance);
    if (this.hasPlatformScrollBoundaryCallback()) {
      instance.onOverScroll((event: OnOverScrollEvent): void => {
        // Use this as a boundary notification, not as a per-frame delta.
        if (Number.isFinite(event.yOffset) && Math.abs(event.yOffset) > Math.abs(event.xOffset)) {
          this.reportPlatformScrollBoundary(true);
        }
      });
      instance.onScroll((event: OnScrollEvent): void => {
        if (this.scrollY === undefined || this.scrollY !== event.yOffset) {
          this.scrollY = event.yOffset;
          this.reportPlatformScrollBoundary(false);
        }
      });
    }
  }
}
