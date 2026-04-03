// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { LynxModule } from '../jsbridge/LynxModule';
import { AsyncCallback, BusinessError } from '@ohos.base';
import { LynxGenericResourceFetcher } from '../provider/LynxGenericResourceFetcher';
import { LynxResourceRequest, LynxResourceType } from '../provider/LynxResourceRequest';
import { LynxSubErrorCode } from '../tasm/gen/LynxSubErrorCode';
import { LLog } from '../base/LLog';
import { LynxContext } from '../tasm/LynxContext';

const TAG = 'LynxResourceModule';

export interface ResourcePrefetchParams {
  // for image, optional parameter, indicates the priority of the image.
  // It supports three values: 'high', 'medium', and 'low', with a default value of 'low'.
  priority?: string;

  // for image, optional parameter, indicates whether to save the image as a bitmap cache or a disk cache.
  // It supports two values: 'disk' and 'bitmap', with a default value of 'disk'.
  cacheTarget?: string;

  // for video and audio, it is a required parameter that represents the unique identifier to consume this resource.
  preloadKey?: string;

  // for video, optional parameter, indicates the preloading size.
  size?: number;
}

export interface Resource {
  uri: string;
  type: string;
  params?: ResourcePrefetchParams;
}

export interface ResourcePrefetchData {
  data: Resource[];
}

export interface ResourcePrefetchConfig {
  awaitComplete?: boolean;
  awaitTimeout?: number;
}

export class ResourcePrefetchDetail {
  public code: number;
  public msg: string;
  public uri?: string;
  public type?: string;

  constructor(code: number, msg: string) {
    this.code = code;
    this.msg = msg;
  }
}

export interface ResourcePrefetchCallback {
  code: number;
  msg: string;
  details?: ResourcePrefetchDetail[];
}

export class LynxResourceModule extends LynxModule {
  private static readonly IMAGE_TYPE = 'image';
  private static readonly VIDEO_TYPE = 'video';
  private static readonly AUDIO_TYPE = 'audio';
  private static readonly FONT_TYPE = 'font';

  requestResourcePrefetch(data: ResourcePrefetchData, callback: (res: ResourcePrefetchCallback) => void,
    config?: ResourcePrefetchConfig): void {
    const awaitComplete = config?.awaitComplete ?? false;
    const awaitTimeout = config?.awaitTimeout ?? 60000;

    if (!awaitComplete) {
      let details: ResourcePrefetchDetail[] = [];
      let globalRes: [number, string] = this.resourcePrefetch(data, false, details);
      if (callback) {
        callback({
          code: globalRes[0], msg: globalRes[1],
          details: details
        });
      }
    } else {
      this.resourcePrefetchAwaitComplete(data, callback, awaitTimeout);
    }
  }

  cancelResourcePrefetch(data: ResourcePrefetchData, callback: (res: ResourcePrefetchCallback) => void): void {
    let details = [];
    let globalRes: [number, string] = this.resourcePrefetch(data, true, details);
    if (callback) {
      callback({
        code: globalRes[0], msg: globalRes[1],
        details: details
      });
    }
  }

  requestResourcePrefetchImage(data: Resource, callback: (res: ResourcePrefetchCallback) => void): void {
    const uri = data.uri;
    if (uri === undefined || uri.length === 0) {
      let code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
      let msg = 'Parameters error in Lynx resource prefetch module! uri is null.';
      if (callback) {
        callback({
          code: code, msg: msg,
        });
      }
      return;
    }
    let genericFetcher: LynxGenericResourceFetcher | undefined = this.context.getGenericResourceFetcher();
    if (genericFetcher === undefined) {
      let code = LynxSubErrorCode.E_RESOURCE_MODULE_RESOURCE_SERVICE_NOT_EXIST;
      let msg = 'Generic resource fetcher is undefined';
      if (callback) {
        callback({
          code: code, msg: msg,
        });
      }
      return;
    }
    const imageCallback: AsyncCallback<string> = (error: BusinessError, result: string) => {
      let code = 0;
      let msg = "";
      if (error) {
        LLog.e(TAG, `request resource failed` + JSON.stringify(error));
        code = error.code;
        msg = error.message;
      }
      if (callback) {
        callback({
          code: code, msg: msg,
        });
      }
    };
    let request: LynxResourceRequest = new LynxResourceRequest(uri, LynxResourceType.LYNX_RESOURCE_TYPE_IMAGE);
    genericFetcher.fetchResourcePath(request, imageCallback);
  }

  private prefetchFont(uri: string, callback?: (code: number, msg: string) => void): void {
    if (this.context instanceof LynxContext) {
      (this.context as LynxContext).prefetchFont(uri, callback);
    }
  }

  private requestResourcePrefetchInternal(genericFetcher: LynxGenericResourceFetcher, uri: string, type: string,
    params?: ResourcePrefetchParams): [number, string] {
    let code: number = 0;
    let msg = '';
    const callback: AsyncCallback<string> = (error: BusinessError, result: string) => {
      if (error) {
        LLog.e(TAG, `request resource failed` + JSON.stringify(error));
      }
    };
    switch (type) {
      case LynxResourceModule.IMAGE_TYPE: {
        let request: LynxResourceRequest = new LynxResourceRequest(uri, LynxResourceType.LYNX_RESOURCE_TYPE_IMAGE);
        genericFetcher.fetchResourcePath(request, callback);
        break;
      }
      case LynxResourceModule.FONT_TYPE: {
        this.prefetchFont(uri);
        break;
      }
      case LynxResourceModule.AUDIO_TYPE:
      case LynxResourceModule.VIDEO_TYPE: {
        code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        msg = `Does not support: ${type}, support for it will be available later.`;
        break;
      }
      default: {
        code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        msg = `Parameters error! Unknown resource type: ${type}`;
        break;
      }
    }
    return [code, msg];
  }

  private cancelResourcePrefetchInternal(genericFetcher: LynxGenericResourceFetcher, uri: string, type: string,
    params?: ResourcePrefetchParams): [number, string] {
    let code: number = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
    let msg = 'Does not support cancelResourcePrefetch.';
    switch (type) {
      case LynxResourceModule.IMAGE_TYPE:
      case LynxResourceModule.AUDIO_TYPE:
      case LynxResourceModule.VIDEO_TYPE: {
        break;
      }
      default: {
        code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        msg = `Parameters error! Unknown resource type: ${type}`;
        break;
      }
    }
    return [code, msg];
  }

  private resourcePrefetch(prefetchData: ResourcePrefetchData, isCancel: boolean,
    details: ResourcePrefetchDetail[]): [number, string] {
    let globalCode: number = LynxSubErrorCode.E_SUCCESS;
    let globalMsg = '';
    let data: Resource[] = prefetchData.data;
    let genericFetcher: LynxGenericResourceFetcher | undefined = this.context.getGenericResourceFetcher();
    if (genericFetcher === undefined) {
      globalCode = LynxSubErrorCode.E_RESOURCE_MODULE_RESOURCE_SERVICE_NOT_EXIST;
      globalMsg = 'Generic resource fetcher is undefined';
      return [globalCode, globalMsg];
    }

    if (!Array.isArray(data)) {
      globalCode = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
      globalMsg = 'Parameters error in Lynx resource prefetch module! Value of data should be an array.';
      return [globalCode, globalMsg];
    }
    for (const obj of data) {
      let code: number = LynxSubErrorCode.E_SUCCESS;
      let msg = '';
      let result: ResourcePrefetchDetail = new ResourcePrefetchDetail(code, msg);
      if (obj === undefined) {
        code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        msg = 'Parameters error in Lynx resource prefetch module! The prefetch data should be a map.';
      } else {
        const uri = obj.uri;
        const type = obj.type;
        const params = obj.params;
        if (uri === undefined || type === undefined) {
          code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
          msg = 'Parameters error in Lynx resource prefetch module! uri or type is undefined.'
        } else {
          let res: [number, string] = isCancel ? this.cancelResourcePrefetchInternal(genericFetcher, uri, type, params)
            : this.requestResourcePrefetchInternal(genericFetcher, uri, type, params);
          code = res[0];
          msg = res[1];
        }
        result.uri = uri;
        result.type = type;
      }
      result.code = code;
      result.msg = msg;
      details.push(result);
    }
    return [globalCode, globalMsg];
  }

  private resourcePrefetchAwaitComplete(prefetchData: ResourcePrefetchData,
    callback: (res: ResourcePrefetchCallback) => void, timeout: number): void {
    const allResults: ResourcePrefetchCallback = {
      code: LynxSubErrorCode.E_SUCCESS,
      msg: '',
      details: []
    };

    const data: Resource[] = prefetchData.data;
    if (!Array.isArray(data) || data.length === 0) {
      if (!Array.isArray(data)) {
        allResults.code = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        allResults.msg = 'Parameters error in Lynx resource prefetch module! Value of data should be an array.';
      }
      if (callback) {
        callback(allResults);
      }
      return;
    }

    const genericFetcher: LynxGenericResourceFetcher | undefined = this.context.getGenericResourceFetcher();
    if (genericFetcher === undefined) {
      allResults.code = LynxSubErrorCode.E_RESOURCE_MODULE_RESOURCE_SERVICE_NOT_EXIST;
      allResults.msg = 'Generic resource fetcher is undefined';
      if (callback) {
        callback(allResults);
      }
      return;
    }

    const totalCount = data.length;
    let completedCount = 0;
    let isCallbackInvoked = false;

    const timeoutId = setTimeout(() => {
      if (!isCallbackInvoked) {
        isCallbackInvoked = true;
        allResults.code = LynxSubErrorCode.E_RESOURCE_MODULE_AWAIT_TIMEOUT;
        allResults.msg = 'The prefetch task did not complete within the specified timeout.';
        if (callback) {
          callback(allResults);
        }
      }
    }, timeout);

    for (const obj of data) {
      let uri = '';
      let type = '';
      let params: ResourcePrefetchParams | undefined = undefined;
      let initialCode = LynxSubErrorCode.E_SUCCESS;
      let initialMsg = '';

      if (obj === undefined) {
        initialCode = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
        initialMsg = 'Parameters error in Lynx resource prefetch module! The prefetch data should be a map.';
      } else {
        uri = obj.uri;
        type = obj.type;
        params = obj.params;
        if (uri === undefined || type === undefined) {
          initialCode = LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR;
          initialMsg = 'Parameters error in Lynx resource prefetch module! uri or type is undefined.';
        }
      }

      const internalCallback = (code: number, msg: string) => {
        if (isCallbackInvoked) {
          return;
        }

        const result = new ResourcePrefetchDetail(code, msg);
        result.uri = uri;
        result.type = type;
        allResults.details?.push(result);

        completedCount++;
        if (completedCount === totalCount) {
          isCallbackInvoked = true;
          clearTimeout(timeoutId);
          if (callback) {
            callback(allResults);
          }
        }
      };

      if (initialCode !== LynxSubErrorCode.E_SUCCESS) {
        internalCallback(initialCode, initialMsg);
      } else {
        this.requestResourcePrefetchInternalAwaitComplete(genericFetcher, uri, type, internalCallback, params);
      }
    }
  }

  private requestResourcePrefetchInternalAwaitComplete(genericFetcher: LynxGenericResourceFetcher,
    uri: string, type: string, callback: (code: number, msg: string) => void,
    params?: ResourcePrefetchParams): void {
    switch (type) {
      case LynxResourceModule.IMAGE_TYPE: {
        let request: LynxResourceRequest = new LynxResourceRequest(uri, LynxResourceType.LYNX_RESOURCE_TYPE_IMAGE);
        genericFetcher.fetchResourcePath(request, (error: BusinessError, result: string) => {
          if (error) {
            LLog.e(TAG, `request resource failed` + JSON.stringify(error));
            callback(error.code, error.message);
          } else {
            callback(LynxSubErrorCode.E_SUCCESS, '');
          }
        });
        break;
      }
      case LynxResourceModule.FONT_TYPE: {
        this.prefetchFont(uri, callback);
        break;
      }
      case LynxResourceModule.AUDIO_TYPE:
      case LynxResourceModule.VIDEO_TYPE: {
        callback(LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR,
          `Does not support: ${type}, support for it will be available later.`);
        break;
      }
      default: {
        callback(LynxSubErrorCode.E_RESOURCE_MODULE_PARAMS_ERROR,
          `Parameters error! Unknown resource type: ${type}`);
        break;
      }
    }
  }
}
