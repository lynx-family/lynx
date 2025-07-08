// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { ImageKnife } from '@ohos/imageknife/src/main/ets/ImageKnife';
import { AbilityStage } from '@kit.AbilityKit';

export class LynxAbilityStage extends AbilityStage {
  onCreate() {
    ImageKnife.getInstance().initFileCache(this.context);
  }
}
