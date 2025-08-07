// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxResourceProvider } from '../provider/LynxResourceProvider';
import { BehaviorRegistryMap } from './behavior/Behavior';
import { LynxContext } from './LynxContext';
import { LynxUIRenderer } from './LynxUIRenderer';
import { LynxNodeController } from './LynxView';

export abstract class LynxUIRendererCreator {
  abstract createUIRenderer(context: LynxContext,
    rootSize: Size, providers: (LynxResourceProvider | undefined)[],
    behaviors: BehaviorRegistryMap): LynxUIRenderer | undefined;

  abstract createNodeController(): LynxNodeController;
}
