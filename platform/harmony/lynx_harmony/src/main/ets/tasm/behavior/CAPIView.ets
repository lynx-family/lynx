// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { HarmonyUIRenderer } from './HarmonyUIRenderer';
import { NativeContent } from 'liblynx.so';

@Component
export struct CAPIView {
  renderer!: HarmonyUIRenderer;
  nativeContent!: NativeContent;

  aboutToAppear() {
    this.nativeContent = this.renderer.uiOwner.createNativeContent();
    this.renderer?.uiOwner.attachPageRoot(this.nativeContent)
  }

  build() {
    ContentSlot(this.nativeContent.content);
  }
}