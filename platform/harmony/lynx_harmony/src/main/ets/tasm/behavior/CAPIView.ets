// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { HarmonyUIRenderer } from './HarmonyUIRenderer';
import { NativeContent } from 'liblynx.so';

@Component
export struct CAPIView {
  renderer!: HarmonyUIRenderer;
  nativeContent!: NativeContent;
  @State private rawInputEnabled: boolean = false;

  aboutToAppear() {
    this.renderer.uiOwner.setRawInputEnabledListener((enabled: boolean) => {
      this.rawInputEnabled = enabled;
    });
    this.nativeContent = this.renderer.uiOwner.createNativeContent();
    this.renderer?.uiOwner.attachPageRoot(this.nativeContent)
  }

  aboutToDisappear() {
    this.renderer.uiOwner.setRawInputEnabledListener(undefined);
  }

  build() {
    if (this.rawInputEnabled) {
      // Hit-test shielding excludes components; monopolization excludes ancestors.
      Stack() {
        ContentSlot(this.nativeContent.content);
      }
      .monopolizeEvents(true)
    } else {
      ContentSlot(this.nativeContent.content);
    }
  }
}
