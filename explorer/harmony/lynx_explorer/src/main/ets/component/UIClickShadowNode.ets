// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { IContext, ShadowNode } from '@lynx/lynx';

export class ClickShadowNode extends ShadowNode {
  constructor(context: IContext) {
    super(context);
    // Tell Lynx framework that your shadow node has extra data needs to send to UI.
    // You can get the extra data by override 'UIBase.updateExtraData'.
    this.setExtraDataFunc(this.getExtraBundle);
    // Tell Lynx framework that you have a customized measure calculation function.
    this.setMeasureFunc(this.measure, null);
  }

  getExtraBundle(): Object {
    return "This is from extra bundle"
  }

  measureChildren(width: number, widthMode: number, height: number, heightMode: number) {
    let children = Array.from(new Int32Array(this.getChildren()));
    // you should measure your children by context.measureNativeNode function if you'd like to support customized children
    children.forEach(element => {
      this.context.measureNativeNode(element, width, widthMode, height, heightMode, true);
    });
  }

  measure(width: number, widthMode: number, height: number, heightMode: number): [number, number, number] {
    // your customized measure here.
    this.measureChildren(width, widthMode, height, heightMode);
    return [300, 300, 0];
  }
}