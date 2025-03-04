// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import {
  NodesRef as INodesRef,
  SelectorQuery as ISelectorQuery,
  uiFieldsOptions,
  uiMethodOptions,
} from '@lynx-js/types';
import {
  ErrorCode,
  IdentifierType,
  NodeSelectToken,
  SelectorQueryNativeProxy,
} from './interface';
import { InvokeError, reportError } from '../report';

/**
 * SelectorQuery is a query object that can be used to select nodes in the Virtual DOM tree.
 *
 * Example:
 * this.createSelectorQuery()
 *   .select('#video')
 *   .invoke({
 *     method: 'seekTo',
 *     params: {
 *       duration: 1000,
 *     },
 *     success: function (res) {
 *       console.log(res);
 *     },
 *     fail: function (res) {
 *       console.log(res.code, res.data);
 *     },
 *   })
 *   .exec();
 */
export default class SelectorQuery implements ISelectorQuery {
  private readonly _component: string;
  private readonly _taskQueue: Function[];
  private readonly _native_proxy: SelectorQueryNativeProxy;
  private _root_unique_id?: number;

  /**
   * Normally, a query is executed after calling exec().
   * However, when `_fire_immediately` is set to true,
   * the query will be executed immediately after task committed (when calling `invoke()`, etc.)
   * without the need of calling `exec()` explicitly.
   *
   * This is used when SelectorQuery is used as ReactRef.
   */
  private _fire_immediately: boolean;

  private constructor(
    component: string,
    taskQueue: Function[],
    proxy: SelectorQueryNativeProxy
  ) {
    this._component = component;
    this._taskQueue = taskQueue;
    this._native_proxy = proxy;
    this._fire_immediately = false;
    this._root_unique_id = undefined;
  }

  static fromQuery(
    prevQuery: SelectorQuery,
    component?: string
  ): SelectorQuery {
    return new SelectorQuery(
      component ?? prevQuery._component,
      prevQuery._taskQueue.slice(),
      prevQuery._native_proxy
    );
  }

  static newEmptyQuery(
    proxy: SelectorQueryNativeProxy,
    component?: string
  ): SelectorQuery {
    return new SelectorQuery(component ?? '', [], proxy);
  }

  /**
   * According to `this._fire_immediately`,
   * either execute the query immediately or add it to the task queue of the SelectorQuery.
   * In the latter case, a new query is returned, and `this` is not modified.
   * @param task the task to commit
   */
  commitTask(task: Function): ISelectorQuery {
    let new_query = SelectorQuery.fromQuery(this, this._component);
    new_query._taskQueue.push(task);

    if (this._fire_immediately) {
      new_query.exec();
      return undefined;
    }
    return new_query;
  }

  in(component: { createSelectorQuery: Function }): ISelectorQuery {
    return component.createSelectorQuery(this);
  }

  /**
   * Selects a single node by CSS selector.
   * @param selector CSS selector
   */
  select(selector: string): INodesRef {
    return new NodesRef(this, {
      type: IdentifierType.ID_SELECTOR,
      identifier: selector,
      component_id: this._component,
      root_unique_id: this._root_unique_id,
      first_only: true,
    });
  }

  /**
   * Selects all nodes satisfying CSS selector.
   * @param selector CSS selector
   */
  selectAll(selector: string): INodesRef {
    return new NodesRef(this, {
      type: IdentifierType.ID_SELECTOR,
      identifier: selector,
      component_id: this._component,
      root_unique_id: this._root_unique_id,
      first_only: false,
    });
  }

  /**
   * Selects a single node as React ref.
   * When works as ReactRef, SelectorQuery should act like getNodeRef, which means:
   * 1. cascade query is disabled.
   * 2. tasks are executed immediately without calling exec().
   */
  selectReactRef(ref_string: string): INodesRef {
    if (this._taskQueue.length) {
      const errorMessage =
        'selectReactRef() should be called before any other selector query methods';
      nativeConsole.warn(errorMessage);
      const error = new Error(errorMessage);
      reportError(
        new InvokeError(errorMessage, error.stack),
        this._native_proxy.nativeApp
      );
      return;
    }

    this._fire_immediately = true;
    return new NodesRef(this, {
      type: IdentifierType.REF_ID,
      identifier: ref_string,
      component_id: this._component,
      root_unique_id: this._root_unique_id,
      first_only: true,
    });
  }

  /**
   * Select root node of the component.
   */
  selectRoot(): INodesRef {
    return this.select('');
  }

  /**
   * Selects a single node by element id.
   * When a touch event is triggered, the element id of the node is passed to the event handler as 'uid',
   * by which can a node be selected in its event handler.
   */
  selectUniqueID(uniqueId: string | number): INodesRef {
    return new NodesRef(this, {
      type: IdentifierType.UNIQUE_ID,
      identifier: uniqueId.toString(),
      component_id: this._component,
      root_unique_id: this._root_unique_id,
      first_only: true,
    });
  }

  /**
   * Execute all tasks in the task queue.
   * When `this._fire_immediately` is set to true, this method is called automatically.
   */
  exec(): void {
    for (let i = 0; i < this._taskQueue.length; ++i) {
      this._taskQueue[i](this._native_proxy);
    }
  }

  setRoot(uniqueId: string | number): SelectorQuery {
    this._root_unique_id = Number(uniqueId);
    return this;
  }
}

export class NodesRef implements INodesRef {
  private static nodePool = {};

  private readonly _nodeSelectToken: NodeSelectToken;
  private readonly _selectorQuery: SelectorQuery;

  constructor(selectorQuery: SelectorQuery, nodeSelectToken: NodeSelectToken) {
    this._nodeSelectToken = nodeSelectToken;
    this._selectorQuery = selectorQuery;
  }
  invoke(options: uiMethodOptions): ISelectorQuery {
    let errorStack;
    if (NODE_ENV === 'development' || NODE_ENV === 'test') {
      errorStack = new Error('');
    }

    let task = (proxy: SelectorQueryNativeProxy) => {
      let callback = (res) => {
        if (res.code === ErrorCode.SUCCESS) {
          options.success && options.success(res.data);
        } else {
          if (options.fail) {
            options.fail(res);
          } else {
            // enable warning in development and test
            if (NODE_ENV === 'development' || NODE_ENV === 'test') {
              if (!proxy.lynx._switches.disableSelectorQueryWarningWhenFailed) {
                const errorMessage = `Failed to exec createSelectorQuery().invoke() on NodesRef ${JSON.stringify(
                  this._nodeSelectToken
                )}. Add a fail callback to suppress this warning. Msg: ${JSON.stringify(
                  res
                )}`;
                nativeConsole.warn(errorMessage);
                reportError(
                  new InvokeError(errorMessage, errorStack.stack),
                  proxy.nativeApp
                );
              }
            }
          }
        }
      };
      if (!this._nodeSelectToken.first_only) {
        callback({
          code: ErrorCode.SELECTOR_NOT_SUPPORTED,
          data: 'selectAll not supported for invoke method',
        });
        return;
      }
      proxy.nativeApp.invokeUIMethod(
        this._nodeSelectToken.type,
        this._nodeSelectToken.identifier,
        this._nodeSelectToken.component_id,
        options.method,
        options.params ?? {},
        callback,
        this._nodeSelectToken.root_unique_id
      );
    };
    return this._selectorQuery.commitTask(task);
  }

  path(cb: Function) {
    let task = (proxy: SelectorQueryNativeProxy) => {
      let callback = (res) => {
        cb && cb(res.data, res.status);
      };
      proxy.nativeApp.getPathInfo(
        this._nodeSelectToken.type,
        this._nodeSelectToken.identifier,
        this._nodeSelectToken.component_id,
        this._nodeSelectToken.first_only,
        callback,
        this._nodeSelectToken.root_unique_id
      );
    };
    return this._selectorQuery.commitTask(task);
  }

  fields(fields: uiFieldsOptions, cb: Function) {
    let task = (proxy: SelectorQueryNativeProxy) => {
      let callback = (res: { data: any; status: any }) => {
        // when 'query' is passed, 'unique_id' is actually returned.
        // should create SelectorQuery using 'unique_id' as root here.
        if (fields.query) {
          const addQueryObject = (result) => {
            result.query = SelectorQuery.newEmptyQuery(proxy);
            result.query.setRoot(result.unique_id.toString());
            if (!fields.unique_id) {
              delete result.unique_id;
            }
          };
          if (this._nodeSelectToken.first_only) {
            let result = res.data;
            if (result) {
              addQueryObject(result);
            }
          } else {
            for (let result of res.data) {
              addQueryObject(result);
            }
          }
        }
        cb && cb(res.data, res.status);
      };
      let fields_array: string[] = [];
      for (let key in fields) {
        // filter 'query'. use 'unique_id' instead.
        if (key == 'query' && fields[key] == true && !fields.unique_id) {
          fields_array.push('unique_id');
          continue;
        }
        if (fields[key]) {
          fields_array.push(key);
        }
      }
      proxy.nativeApp.getFields(
        this._nodeSelectToken.type,
        this._nodeSelectToken.identifier,
        this._nodeSelectToken.component_id,
        this._nodeSelectToken.first_only,
        fields_array,
        callback,
        this._nodeSelectToken.root_unique_id
      );
    };
    return this._selectorQuery.commitTask(task);
  }

  setNativeProps(nativeProps: Record<string, unknown>) {
    let task = (proxy: SelectorQueryNativeProxy) => {
      proxy.nativeApp.setNativeProps(
        this._nodeSelectToken.type,
        this._nodeSelectToken.identifier,
        this._nodeSelectToken.component_id,
        this._nodeSelectToken.first_only,
        nativeProps,
        this._nodeSelectToken.root_unique_id
      );
    };
    return this._selectorQuery.commitTask(task);
  }
}
