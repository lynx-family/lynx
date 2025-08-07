// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import { Size } from '@kit.ArkUI';
import { ShadowNode } from './shadow/ShadowNode';
import { LynxContext } from '../LynxContext';
import { BehaviorRegistryMap } from './Behavior';

export class ShadowNodeOwner {
  public readonly native: lynx.ShadowNodeOwner;
  private context: LynxContext;
  nodeHolder: Map<number, ShadowNode> = new Map();
  behaviors: BehaviorRegistryMap;
  rootSize: Size;
  isDestroyed: boolean = false;

  constructor(context: LynxContext, behaviors: BehaviorRegistryMap, rootSize: Size) {
    this.behaviors = behaviors;

    this.native = new lynx.ShadowNodeOwner(this, this.create);
    this.context = context;
    this.rootSize = rootSize;
    context.setShadowNodeOwner(this);
  }

  create(sign: number, tag: string): lynx.ShadowNode | undefined {
    let behavior = this.behaviors.get(tag);
    if (behavior != undefined && behavior.shadowNodeClass != undefined) {
      let node: ShadowNode | undefined = behavior.createShadowNode(this.context);
      node?.setSign(sign, tag);
      node?.createNativeNode();
      return node?.nativeShadowNode;
    }
    return;
  }

  findJSShadowNodeBySign(sign: number) {
    return this.native.findJSShadowNodeBySign(sign);
  }

  measureNativeNode(sign: number,
    width: number,
    widthMode: number,
    height: number,
    heightMode: number,
    finalMeasure: boolean): [number, number, number] {
    if (this.isDestroyed) {
      return [0, 0, 0];
    }
    let resultBuffer: ArrayBuffer =
      this.native.measureNativeNode(sign, width, widthMode, height, heightMode, finalMeasure);
    if (resultBuffer) {
      let resultArray = new Float32Array(resultBuffer);
      return [resultArray[0], resultArray[1], resultArray[2]];
    }
    return [0, 0, 0];
  }

  alignNativeNode(sign: number,
    top: number,
    left: number) {
    if (this.isDestroyed) {
      return;
    }
    this.native.alignNativeNode(sign, top, left);
  }

  destroy() {
    if (this.isDestroyed) {
      return;
    }
    this.native.destroy();
    this.isDestroyed = true;
  }

  updateRootSize(width: number, height: number) {
    this.rootSize.width = width;
    this.rootSize.height = height;
  }
}
