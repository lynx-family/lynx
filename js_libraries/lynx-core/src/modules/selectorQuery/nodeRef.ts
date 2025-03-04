// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { uiMethodOptions } from '@lynx-js/types';
import { NativeApp } from '../../app';
import { NativeLynxUIModule } from '../nativeModules';
import { ErrorCode } from './interface';
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
      (res) => {
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
      }
    );
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
