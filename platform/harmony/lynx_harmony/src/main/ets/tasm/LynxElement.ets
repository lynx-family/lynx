// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxTemplateRenderer } from './LynxTemplateRenderer';

export type LynxElementStatus = 'alive' | 'detached';
export type LynxValue = null | boolean | number | string | Object | Array<LynxValue>;
export type LynxElementCallback<T> = (result: T) => void;
export type LynxElementFilter = (node: LynxElement) => boolean;
export type LynxRecord = Record<string, Object>;
export type NativeQueryCallback = (result: LynxValue | undefined) => void;

const QUERY_ROOT = 0;
const QUERY_STATUS = 1;
const QUERY_IDENTITY = 2;
const QUERY_PARENT = 3;
const QUERY_CHILDREN = 4;
const QUERY_FIND_BY_ID = 5;
const QUERY_DATASET = 6;
const QUERY_ATTRIBUTES = 7;
const QUERY_ATTRIBUTE = 8;
const QUERY_JSON = 9;
const QUERY_POSITION = 10;
const INVALID_LYNX_ELEMENT_SIGN = -1;

interface NativeQueryRenderer {
  nativeQueryLynxElement(sign: number, queryType: number, argument: string | null, callback: NativeQueryCallback): void;
}

function isRecord(value: LynxValue | undefined): boolean {
  return value !== null && value !== undefined && typeof value === 'object' && !Array.isArray(value);
}

function isNumberArray(value: LynxValue | undefined): boolean {
  return Array.isArray(value) && value.every((item: LynxValue) => typeof item === 'number');
}

export class LynxElementOwner {
  private rendererRef?: WeakRef<LynxTemplateRenderer>;

  constructor(renderer?: LynxTemplateRenderer) {
    this.rendererRef = renderer ? new WeakRef(renderer) : undefined;
  }

  query(sign: number, type: number, argument: string | null, callback?: NativeQueryCallback): void {
    if (!callback) {
      return;
    }
    const renderer = this.rendererRef?.deref() as NativeQueryRenderer | undefined;
    if (!renderer) {
      callback(undefined);
      return;
    }
    renderer.nativeQueryLynxElement(sign, type, argument, callback);
  }

  getRoot(callback?: LynxElementCallback<LynxElement | null>): void {
    if (!callback) {
      return;
    }
    this.query(0, QUERY_ROOT, null, result => callback(this.element(typeof result === 'number' ? result : INVALID_LYNX_ELEMENT_SIGN)));
  }

  element(sign: number): LynxElement | null {
    if (sign === INVALID_LYNX_ELEMENT_SIGN) {
      return null;
    }
    return new LynxElement(this, sign);
  }

  asRecord(value: LynxValue | undefined): LynxRecord {
    return isRecord(value) ? value as LynxRecord : {};
  }

  asSignList(value: LynxValue | undefined): Array<number> {
    return isNumberArray(value) ? value as Array<number> : [];
  }
}

export class LynxElement {
  private owner: LynxElementOwner;
  private sign: number;

  constructor(owner: LynxElementOwner, sign: number) {
    this.owner = owner;
    this.sign = sign;
  }

  findNodeById(id: string, callback?: LynxElementCallback<LynxElement | null>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_FIND_BY_ID, id, result => callback(this.owner.element(typeof result === 'number' ? result : INVALID_LYNX_ELEMENT_SIGN)));
  }

  findNodeByFilter(filter: LynxElementFilter | null, callback?: LynxElementCallback<LynxElement | null>): void {
    if (!callback) {
      return;
    }
    this.findNodesByFilter(filter, nodes => callback(nodes.length > 0 ? nodes[0] : null));
  }

  findNodesByFilter(filter: LynxElementFilter | null, callback?: LynxElementCallback<readonly LynxElement[]>): void {
    if (!callback) {
      return;
    }
    const matches: LynxElement[] = [];
    this.collectMatches(filter, matches, () => callback(matches));
  }

  getStatus(callback?: LynxElementCallback<LynxElementStatus>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_STATUS, null, result => callback(result === true ? 'alive' : 'detached'));
  }

  getSign(callback?: LynxElementCallback<number>): void {
    callback?.(this.sign);
  }

  getTagName(callback?: LynxElementCallback<string | null>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_IDENTITY, null, result => {
      const tagName = this.owner.asRecord(result)['tagName'];
      callback(typeof tagName === 'string' ? tagName : null);
    });
  }

  getId(callback?: LynxElementCallback<string | null>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_IDENTITY, null, result => {
      const id = this.owner.asRecord(result)['id'];
      callback(typeof id === 'string' ? id : null);
    });
  }

  getParent(callback?: LynxElementCallback<LynxElement | null>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_PARENT, null, result => callback(this.owner.element(typeof result === 'number' ? result : INVALID_LYNX_ELEMENT_SIGN)));
  }

  getChildren(callback?: LynxElementCallback<readonly LynxElement[]>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_CHILDREN, null, result => {
      callback(this.owner.asSignList(result)
        .map(sign => this.owner.element(sign))
        .filter((element: LynxElement | null) => element !== null) as Array<LynxElement>);
    });
  }

  getChildCount(callback?: LynxElementCallback<number>): void {
    if (!callback) {
      return;
    }
    this.getChildren(children => callback(children.length));
  }

  getRoot(callback?: LynxElementCallback<LynxElement | null>): void {
    this.owner.getRoot(callback);
  }

  getPositionInfo(callback?: LynxElementCallback<LynxRecord | null>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_POSITION, null, result => callback(isRecord(result) ? result as LynxRecord : null));
  }

  getDataset(callback?: LynxElementCallback<LynxRecord>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_DATASET, null, result => callback(this.owner.asRecord(result)));
  }

  getAttribute(name: string, callback?: LynxElementCallback<LynxValue | null>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_ATTRIBUTE, name, result => callback(result === undefined ? null : result));
  }

  getAttributes(callback?: LynxElementCallback<LynxRecord>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_ATTRIBUTES, null, result => callback(this.owner.asRecord(result)));
  }

  /**
   * Serializes this LynxElement tree to a JSON string.
   */
  toJSONString(callback?: LynxElementCallback<string | null>): void {
    if (!callback) {
      return;
    }
    this.owner.query(this.sign, QUERY_JSON, null, result => callback(typeof result === 'string' && result.length > 0 ? result : null));
  }

  private collectMatches(filter: LynxElementFilter | null, matches: LynxElement[], complete: () => void): void {
    if (filter && filter(this)) {
      matches.push(this);
    }
    this.getChildren(children => this.collectChildMatches(children, 0, filter, matches, complete));
  }

  private collectChildMatches(children: readonly LynxElement[], index: number, filter: LynxElementFilter | null,
    matches: LynxElement[], complete: () => void): void {
    if (index >= children.length) {
      complete();
      return;
    }
    children[index].collectMatches(filter, matches, () => {
      this.collectChildMatches(children, index + 1, filter, matches, complete);
    });
  }
}
