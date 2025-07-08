// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { UIBase } from './UIBase';
import lynx from 'liblynx.so';

export class UIGroup extends UIBase {
  children: lynx.NativeContent[] = [];
  hasCustomLayout: boolean = true;

  onCreate() {
    this.nativeUI?.setChildrenManagementFuncs(this.insertChild, this.removeChild);
  }


  layout(x: number, y: number, width: number, height: number, paddingLeft: number, paddingTop: number,
    paddingRight: number, paddingBottom: number, marginLeft: number, marginTop: number, marginRight: number,
    marginBottom: number, sticky?: number[] | undefined): void {
    throw new Error('Method not implemented.');
  }

  insertChild(nativeContent: lynx.NativeContent, index: number) {
    this.children.splice(index, 0, nativeContent);
  }

  removeChild(nativeContent: lynx.NativeContent) {
    const index = this.children.indexOf(nativeContent);
    if (index > 0) {
      this.children.splice(index, 1);
    }
  }

  getContentSize(content: lynx.NativeContent): [number, number] {
    if (this.nativeUI) {
      return lynx.UIBase.getContentSize(content);
    }
    return [0, 0];
  }
}