// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { AnyObject } from '../common';

export enum NODE_REF_INVOKE_ERROR_CODE {
  SUCCESS = 0,
  UNKNOWN = 1,
  NODE_NOT_FOUND = 2,
  METHOD_NOT_FOUND = 3,
  PARAM_INVALID = 4, // Determined by the implementer of the component.
  SELECTOR_NOT_SUPPORTED = 5, // Currently only supports ID selectors.
}

export interface uiMethodOptions {
  method: string;
  params?: AnyObject;
  success?(res: any): void;
  fail?(res: { code: NODE_REF_INVOKE_ERROR_CODE; data?: any }): void;
}

export interface uiFieldsOptions {
  id?: boolean;
  dataset?: boolean;
  tag?: boolean;
  query?: boolean;
  unique_id?: boolean;
}

export interface PathData {
  data: Array<{
    tag: string;
    id: string;
    class: string[];
    dataSet: Record<string, unknown>;
    // index of parent's children
    index: number;
  }>;
}

export interface FieldsParams {
  id?: boolean;
  dataset?: boolean;
  tag?: boolean;
  index?: boolean;
  class?: boolean;
  attribute?: boolean;
}

export interface FieldsData {
  id: string;
  tag: string;
  dataset: Record<string, unknown>;
  index: number;
  class: Array<string>;
  attribute: Record<string, unknown>;
}

export type PathCallback = (data: PathData, status: { data: string; code: number }) => void;

export interface NodesRef {
  invoke(options: uiMethodOptions): SelectorQuery;

  path(callback: PathCallback): SelectorQuery;

  fields<T extends FieldsParams>(
    fields: Required<FieldsParams> extends Record<keyof T, boolean> ? T : FieldsParams,
    callback: (
      data: {
        [K in keyof Required<FieldsParams> as T[K] extends true ? K : never]: FieldsData[K];
      },
      status: { data: string; code: number }
    ) => void
  ): SelectorQuery;

  setNativeProps(nativeProps: Record<string, any>): SelectorQuery;
}

export interface SelectorQuery {
  /**
   * Selects a single node by the uid of node.
   * @param selector CSS selector
   */
  selectUniqueID(uniqueId: string | number): NodesRef;

  /**
   * Selects a single node by CSS selector.
   * @param selector CSS selector
   */
  select(selector: string): NodesRef;

  /**
   * Selects all nodes satisfying CSS selector.
   * @param selector CSS selector
   */
  selectAll(selector: string): NodesRef;

  /**
   * Select root node of the component.
   */
  selectRoot(): NodesRef;

  // /**
  //  * Selects a single node by element id.
  //  * When a touch event is triggered, the element id of the node is passed to the event handler as 'uid',
  //  * by which can a node be selected in its event handler.
  //  */
  // selectUniqueID(uniqueId: string | number): NodesRef;

  /**
   * Execute all tasks in the task queue.
   * When `this._fire_immediately` is set to true, this method is called automatically.
   */
  exec(): void;
}
