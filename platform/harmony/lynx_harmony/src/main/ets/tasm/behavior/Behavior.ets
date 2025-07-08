// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { UIBase } from './ui/UIBase';
import { ShadowNodeCreator, UICreator } from './Util';
import { ShadowNode } from './shadow/ShadowNode';
import { IContext } from '../IContext';

export enum NodeType {
  UNKNOWN = 0,
  // Common UI
  COMMON = 1,
  // The Node has layoutNode but no UI
  VIRTUAL = 1 << 1,
  // Node has custom layout
  CUSTOM = 1 << 2,
}

export class Behavior {
  uiClass?: Function;
  shadowNodeClass?: Function;
  customData?: Object;
  type?: NodeType;

  constructor(uiClass?: Function, shadowNodeClass?: Function,
    customData?: Object, type?: NodeType) {
    this.uiClass = uiClass;
    this.shadowNodeClass = shadowNodeClass;
    this.customData = customData;

    if (type === undefined) {
      this.type = NodeType.COMMON;
      if (shadowNodeClass) {
        this.type |= NodeType.CUSTOM;
      }
    }
  }

  public createUI(context: IContext): UIBase | undefined {
    if (this.uiClass) {
      return (new (this.uiClass as UICreator)(context, this.customData)) as UIBase;
    }
    return;
  }

  public createShadowNode(context: IContext): ShadowNode | undefined {
    if (this.shadowNodeClass) {
      return (new (this.shadowNodeClass as ShadowNodeCreator)(context)) as ShadowNode
    }
    return;
  }
}

export type BehaviorRegistryMap = Map<string, Behavior>

export let BUILTIN_BEHAVIORS = new Map<string, Behavior>();
