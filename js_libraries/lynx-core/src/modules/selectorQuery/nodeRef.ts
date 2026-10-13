// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { uiMethodOptions } from '@lynx-js/types';
import { NativeApp } from '../../app';
import { NativeLynxUIModule } from '../nativeModules';
import { ErrorCode, IdentifierType } from './interface';
import { InvokeError, reportError } from '../report';

interface NodeRefProxy {
  nativeLynxUIModule: NativeLynxUIModule;
  nativeApp: NativeApp;
  disableWarningWhenFailed: boolean;
}

export default class NodeRef {
  private readonly _rootComponentId: string;
  private readonly _selectorName: string;
  private readonly _ancestorSelectorNames: string[];
  private readonly _proxy: NodeRefProxy;
  private readonly _isCallByRefId?: boolean;

  constructor(
    nodeRefProxy: NodeRefProxy,
    rootComponentId: string,
    name: string,
    ancestorSelectorNames?: string[],
    isCallByRefId?: boolean
  ) {
    this._selectorName = name;
    if (ancestorSelectorNames && ancestorSelectorNames.length > 0) {
      this._ancestorSelectorNames = [...ancestorSelectorNames];
    } else {
      this._ancestorSelectorNames = [];
    }
    this._rootComponentId = rootComponentId;
    this._proxy = nodeRefProxy;
    this._isCallByRefId = isCallByRefId;
  }

  getNodeRef(name: string): NodeRef {
    return new NodeRef(
      this._proxy,
      this._rootComponentId,
      name,
      [...this._ancestorSelectorNames, this._selectorName],
      this._isCallByRefId
    );
  }

  invoke(options: uiMethodOptions): void {
    /*
            _nativeApp.invokeUIMethod signature:
            (componentId:string, selectorPath:string[], methodName:string, params:object, callback:Function):void

            Note:
            - All 5 parameters need to be passed.
            - componentId: Card instance passes empty string.''
            - params: SDK will add one _isCallByRefId to identify whether to use refId to find the node.

            callback Function params types
            {
              code: number,
              data?: object
            }
    */
    let errorStack;
    if (NODE_ENV === 'development' || NODE_ENV === 'test') {
      errorStack = new Error('');
    }

    const callback = (res) => {
      if (res.code === ErrorCode.SUCCESS) {
        options.success && options.success(res.data);
      } else {
        if (options.fail) {
          options.fail(res);
        } else {
          // enable warning in development and test
          if (NODE_ENV === 'development' || NODE_ENV === 'test') {
            if (!this._proxy.disableWarningWhenFailed) {
              const errorMessage = `Failed to exec NodeRef.invoke() on NodeRef '${[
                ...this._ancestorSelectorNames,
                this._selectorName,
              ]}'. Add a fail callback to suppress this warning.  Msg: ${JSON.stringify(
                res
              )}`;
              nativeConsole.warn(errorMessage);
              reportError(
                new InvokeError(errorMessage, errorStack.stack),
                this._proxy.nativeApp
              );
            }
          }
        }
      }
    };

    // Fragment-layer nodes are managed by the element tree instead of the
    // legacy LynxUI tree. Use the selector path directly for FLR pages.
    if (this._proxy.nativeApp.isFragmentLayerRender === true) {
      this.invokeBySelectorQuery(options, callback);
      return;
    }

    this._proxy.nativeLynxUIModule.invokeUIMethod(
      this._rootComponentId,
      [...this._ancestorSelectorNames, this._selectorName],
      options.method,
      Object.assign(
        {
          _isCallByRefId: this._isCallByRefId,
        },
        options.params
      ),
      callback
    );
  }

  private invokeBySelectorQuery(
    options: uiMethodOptions,
    callback: Function
  ): void {
    const selectors = [...this._ancestorSelectorNames, this._selectorName];
    const type = this._isCallByRefId
      ? IdentifierType.REF_ID
      : IdentifierType.ID_SELECTOR;

    const resolveTarget = (index: number, rootUniqueId?: number): void => {
      if (index === selectors.length - 1) {
        this._proxy.nativeApp.invokeUIMethod(
          type,
          selectors[index],
          this._rootComponentId,
          options.method,
          options.params ?? {},
          callback,
          rootUniqueId
        );
        return;
      }

      const selector = selectors[index];
      this._proxy.nativeApp.getFields(
        type,
        selector,
        this._rootComponentId,
        true,
        ['unique_id'],
        (res: {
          data?: { unique_id?: number };
          status?: { code?: number; data?: string };
        }) => {
          const statusCode = res?.status?.code ?? ErrorCode.UNKNOWN;
          const uniqueId = res?.data?.unique_id;
          if (statusCode !== ErrorCode.SUCCESS || uniqueId == null) {
            callback({
              code:
                statusCode === ErrorCode.SUCCESS
                  ? ErrorCode.NODE_NOT_FOUND
                  : statusCode,
              data:
                statusCode === ErrorCode.SUCCESS
                  ? `not found ${selector}`
                  : res?.status?.data ?? `not found ${selector}`,
            });
            return;
          }
          resolveTarget(index + 1, Number(uniqueId));
        },
        rootUniqueId
      );
    };

    resolveTarget(0);
  }

  scrollIntoView(params: boolean | object = true): void {
    let scrollIntoViewOptions = {};
    if (typeof params === 'boolean') {
      if (params) {
        scrollIntoViewOptions = {
          behavior: 'auto',
          block: 'start',
          inline: 'nearest',
        };
      } else {
        scrollIntoViewOptions = {
          behavior: 'auto',
          block: 'end',
          inline: 'nearest',
        };
      }
    } else if (typeof params === 'object') {
      scrollIntoViewOptions = params;
    } else {
      throw new Error('scrollIntoView only support boolean or object');
    }
    this.invoke({
      method: 'scrollIntoView',
      params: {
        scrollIntoViewOptions,
      },
      fail(res) {
        nativeConsole.error(
          'NodeRef.scrollIntoView failed',
          `ErrorCode: ${res.code}`
        );
      },
    });
  }
}
