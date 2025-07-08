// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx, { JSAlignLayoutFunc, JSMeasureFunc } from 'liblynx.so';
import { IContext } from '../../IContext';

export abstract class ShadowNode {
  nativeShadowNode?: lynx.ShadowNode;
  sign!: number;
  tag!: string;
  context: IContext;
  props?: Record<string, Object>;
  logicalPixels: boolean = true;
  measureFunc: JSMeasureFunc | null = null;
  alignFunc: JSAlignLayoutFunc | null = null;
  extraDataFunc?: () => Object;

  constructor(context: IContext) {
    this.context = context;
  }

  setSign(sign: number, tag: string) {
    this.sign = sign;
    this.tag = tag;
  }

  createNativeNode() {
    this.nativeShadowNode = new lynx.ShadowNode(this, this.sign, this.tag);
    if (this.measureFunc || this.alignFunc) {
      this.nativeShadowNode.setMeasureFunc(this.measureFunc, this.alignFunc);
    }
    if (this.extraDataFunc) {
      this.nativeShadowNode.setExtraDataFunc(this.extraDataFunc);
    }
  }

  // Only constructor
  setMeasureFunc(measureFunc: JSMeasureFunc | null, alignFunc: JSAlignLayoutFunc | null) {
    this.measureFunc = measureFunc;
    this.alignFunc = alignFunc;
  }

  markDirty() {
    this.nativeShadowNode?.markDirty();
  }

  getChildren(): ArrayBuffer {
    return this.nativeShadowNode?.getChildren() ?? new ArrayBuffer(0);
  }

  // Only constructor
  setExtraDataFunc(func: () => Object) {
    this.extraDataFunc = func;
  }

  // Internal use
  updateProps(props: Record<string, Object>) {
    this.props = props
  }

  requestLayout(): void {
    this.nativeShadowNode?.requestLayout();
  }

  px2vpIfNeeded(px: number): number {
    if (this.logicalPixels) {
      return px;
    }
    return px2vp(px);
  }
}
