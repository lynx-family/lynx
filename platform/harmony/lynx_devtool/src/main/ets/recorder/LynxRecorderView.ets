// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import http from '@ohos.net.http';
import { AsyncCallback, BusinessError } from '@ohos.base';
import Url from '@ohos.url';
import {
  LynxContext,
  LynxView,
  MetaData,
  ModuleClassWrapper,
  TemplateBundle,
  TemplateBundleOption,
  TemplateData,
  LynxEnv,
  LynxGenericResourceFetcher,
  LynxMediaResourceFetcher,
  LynxResourceRequest,
  LynxStreamDelegate,
  LynxTemplateResourceFetcher,
  TemplateProviderResult,
  LLog,
  LynxExtraTiming,
  LynxUIRendererCreator,
  LynxUpdateMeta,
  LynxUpdateMode
} from '@lynx/lynx';
import { LynxRecorderReplayOptions } from './LynxRecorderReplayOptions';
import LynxRecorderReplayDataModule, { ReplaySharedData, ReplaySharedDataValue } from './ReplayDataModule';
import {
  LoadComponentWithCallbackParams,
  LoadTemplateParams,
  SendGlobalEventParams,
  SetGlobalPropsParams,
  SetThreadStrategyParams,
  UpdateDataByPreParsedDataParams,
  UpdateMetaDataParams,
  UpdateViewPortParams
} from './DataType';
import { util } from '@kit.ArkTS';
import LynxRecorderEnv from './LynxRecorderEnv';

const TAG = 'LynxRecorderView';

class ReplayGenericResourceFetcher extends LynxGenericResourceFetcher {
  private readonly fallback?: LynxGenericResourceFetcher;
  private readonly recordedResources: Map<string, ArrayBuffer>;

  constructor(recordedResources: Map<string, ArrayBuffer>, fallback?: LynxGenericResourceFetcher) {
    super();
    this.recordedResources = recordedResources;
    this.fallback = fallback;
  }

  private cloneRecordedResource(url: string): ArrayBuffer | undefined {
    let resource = this.recordedResources.get(url);
    if (!resource) {
      return undefined;
    }
    return resource.slice(0, resource.byteLength);
  }

  private buildMissingResourceError(url: string): BusinessError {
    return {
      code: 0,
      name: 'ReplayResourceError',
      message: `No recorded generic resource for url: ${url}`
    };
  }

  fetchResource(request: LynxResourceRequest, callback: AsyncCallback<ArrayBuffer, void>): void {
    let resource = this.cloneRecordedResource(request.url);
    if (resource) {
      callback(undefined, resource);
      return;
    }
    if (this.fallback) {
      this.fallback.fetchResource(request, callback);
      return;
    }
    callback(this.buildMissingResourceError(request.url), new ArrayBuffer(0));
  }

  fetchResourcePath(request: LynxResourceRequest, callback: AsyncCallback<string, void>): void {
    if (this.fallback) {
      this.fallback.fetchResourcePath(request, callback);
      return;
    }
    callback(this.buildMissingResourceError(request.url), '');
  }

  fetchStream(request: LynxResourceRequest, delegate: LynxStreamDelegate): void {
    let resource = this.cloneRecordedResource(request.url);
    if (resource) {
      delegate.onStart(resource.byteLength);
      delegate.onData(resource, 0, resource.byteLength);
      delegate.onEnd();
      return;
    }
    if (this.fallback) {
      this.fallback.fetchStream(request, delegate);
      return;
    }
    delegate.onError(this.buildMissingResourceError(request.url).message);
  }

  cancel(request: LynxResourceRequest): void {
    this.fallback?.cancel(request);
  }
}

class ReplayTemplateResourceFetcher extends LynxTemplateResourceFetcher {
  private readonly fallback?: LynxTemplateResourceFetcher;
  private readonly recordedResources: Map<string, ArrayBuffer>;

  constructor(recordedResources: Map<string, ArrayBuffer>, fallback?: LynxTemplateResourceFetcher) {
    super();
    this.recordedResources = recordedResources;
    this.fallback = fallback;
  }

  private cloneRecordedResource(url: string): ArrayBuffer | undefined {
    let resource = this.recordedResources.get(url);
    if (!resource) {
      return undefined;
    }
    return resource.slice(0, resource.byteLength);
  }

  private buildMissingResourceError(url: string): BusinessError {
    return {
      code: 0,
      name: 'ReplayTemplateResourceError',
      message: `No recorded template resource for url: ${url}`
    };
  }

  fetchTemplate(request: LynxResourceRequest,
    callback: AsyncCallback<TemplateProviderResult, void>): void {
    let resource = this.cloneRecordedResource(request.url);
    if (resource) {
      callback(undefined, { binary: resource });
      return;
    }
    if (this.fallback) {
      this.fallback.fetchTemplate(request, callback);
      return;
    }
    callback(this.buildMissingResourceError(request.url), {});
  }

  fetchSSRData(request: LynxResourceRequest, callback: AsyncCallback<ArrayBuffer, void>): void {
    if (this.fallback) {
      this.fallback.fetchSSRData(request, callback);
      return;
    }
    callback(this.buildMissingResourceError(request.url), new ArrayBuffer(0));
  }
}

@Component
export default struct LynxRecorderView {
  public url?: string;
  public genericFetcher?: LynxGenericResourceFetcher;
  public mediaFetcher?: LynxMediaResourceFetcher;
  public templateFetcher?: LynxTemplateResourceFetcher;
  public uiRendererCreator?: LynxUIRendererCreator;
  private data: string = '';
  private pageUrl: string = LynxRecorderEnv.RECORDER_URL_PREFIX;
  private buffer: ArrayBuffer | undefined;
  private templateBundle: TemplateBundle | undefined;
  private preloadedSource: ArrayBuffer | undefined;
  private bufferReady: boolean = false;
  private loadFromUrl: boolean = false;
  private templateData: TemplateData | undefined;
  private templateDataReady: boolean = false;
  private globalProps: TemplateData | undefined;
  private upstreamGenericFetcher?: LynxGenericResourceFetcher;
  private upstreamTemplateFetcher?: LynxTemplateResourceFetcher;
  private recordedGenericResources: Map<string, ArrayBuffer> = new Map();
  private jsbModule: Map<string, ModuleClassWrapper> = new Map();
  private jsbModuleReady: boolean = false;
  @State ready: boolean = false;
  @State statusText: string = 'Loading...';
  private localLynxContext: LynxContext | undefined;
  private extraTiming: LynxExtraTiming = new LynxExtraTiming();
  private replayOptions: LynxRecorderReplayOptions = new LynxRecorderReplayOptions();
  private templateBundleOption: TemplateBundleOption = new TemplateBundleOption(5, true);
  private replayGeneration: number = 0;

  private isReplaySharedData(value: Object | string | number | boolean | null | undefined): boolean {
    return value !== null && value !== undefined && typeof value === 'object' && !Array.isArray(value);
  }

  private download(replayGeneration: number) {
    let httpRequest = http.createHttp();
    let urlObject = Url.URL.parseURL(this.url);
    let url = this.sanitizeRecordedUrl(urlObject.params.get('url'));
    if (url.length === 0) {
      LLog.e(TAG, 'Unsupported artifact link：' + this.url);
      return;
    }

    let res = new ArrayBuffer(0);
    httpRequest.on('dataReceive', (data: ArrayBuffer) => {
      const newRes = new ArrayBuffer(res.byteLength + data.byteLength);
      const resView = new Uint8Array(newRes);
      resView.set(new Uint8Array(res));
      resView.set(new Uint8Array(data), res.byteLength);
      res = newRes;
    });

    httpRequest.requestInStream(
      url, {
      expectDataType: http.HttpDataType.STRING,
    }, (err: BusinessError, data: number) => {
      if (replayGeneration !== this.replayGeneration) {
        httpRequest.off('dataReceive');
        httpRequest.destroy();
        return;
      }
      if (data != 200) {
        LLog.e(TAG, 'Unsupported artifact link：' + this.url);
      } else {
        let decoder = util.TextDecoder.create('utf-8');
        let stringResult = decoder.decodeToString(new Uint8Array(res));
        if (stringResult.startsWith('{')) {
          this.data = stringResult;
        } else {
          this.data = LynxEnv.parserTestBenchRecordData(stringResult);
        }
        this.registerJSBModule();
        this.handleActionList(replayGeneration);
      }
      httpRequest.off('dataReceive');
      httpRequest.destroy();
    });
  }

  private downloadSourceOverride(replayGeneration: number) {
    let sourceUrl = this.sanitizeRecordedUrl(this.replayOptions.sourceUrl);
    if (sourceUrl.length === 0) {
      this.download(replayGeneration);
      return;
    }

    let httpRequest = http.createHttp();
    httpRequest.request(
      sourceUrl, {
      maxLimit: 20 * 1024 * 1024,
      expectDataType: http.HttpDataType.ARRAY_BUFFER,
    }, (err: BusinessError, data: http.HttpResponse) => {
      if (replayGeneration !== this.replayGeneration) {
        httpRequest.destroy();
        return;
      }
      let source = data?.result as ArrayBuffer | undefined;
      if (err || data?.responseCode !== 200 || source === undefined || source.byteLength === 0) {
        this.statusText = 'Replay failed: source template download failed.';
        LLog.e(TAG, 'source override download failed. url=' + sourceUrl +
          ', code=' + data?.responseCode + ', error=' + JSON.stringify(err));
      } else {
        this.preloadedSource = source.slice(0, source.byteLength);
        this.download(replayGeneration);
      }
      httpRequest.destroy();
    });
  }

  private changeReadyState() {
    this.ready = (this.bufferReady || this.loadFromUrl) && this.templateDataReady && this.jsbModuleReady;
    if (this.ready) {
      this.statusText = '';
    }
  }

  aboutToDisappear(): void {
    this.replayGeneration++;
    if (LynxRecorderReplayDataModule.jsbInvokers) {
      LynxRecorderReplayDataModule.jsbInvokers = []
    }
    if (LynxRecorderReplayDataModule.jsbCallbacks) {
      LynxRecorderReplayDataModule.jsbCallbacks = new Map();
    }
    LynxRecorderReplayDataModule.sharedData = {};
    if (this.jsbModule) {
      this.jsbModule.clear()
    }
    this.recordedGenericResources.clear();
    this.preloadedSource = undefined;
    this.templateBundle = undefined;
    this.genericFetcher = this.upstreamGenericFetcher;
    this.mediaFetcher = undefined;
    this.templateFetcher = this.upstreamTemplateFetcher;
    this.localLynxContext = undefined
  }

  private registerJSBModule() {
    let recordData: Map<string, object> = JSON.parse(this.data);
    let rawSharedData: ReplaySharedData | ReplaySharedDataValue =
      (recordData['SharedData'] as ReplaySharedData | ReplaySharedDataValue | undefined) ?? {};
    LynxRecorderReplayDataModule.jsbInvokers = recordData['Invoked Method Data'];
    LynxRecorderReplayDataModule.jsbCallbacks = recordData['Callback'];
    LynxRecorderReplayDataModule.config = recordData['Config'];
    LynxRecorderReplayDataModule.sharedData = this.isReplaySharedData(rawSharedData)
      ? rawSharedData as ReplaySharedData
      : {};
    LLog.i(TAG,
      `[replay-shared-data] registerJSBModule | keys=${JSON.stringify(Object.keys(LynxRecorderReplayDataModule.sharedData))}`);
    this.jsbModule.set('LynxRecorderReplayDataModule', {
      moduleClass: LynxRecorderReplayDataModule
    });
    this.jsbModuleReady = true;
  }

  private dispatchMessage(that: LynxRecorderView, replayGeneration: number, name: string, params: object) {
    if (replayGeneration !== that.replayGeneration) {
      return;
    }
    if (name == 'loadTemplate') {
      if (that.replayOptions.enablePreDecode) {
        that.loadTemplateBundle(params as LoadTemplateParams);
      } else {
        that.loadTemplate(params as LoadTemplateParams);
      }
    } else if (name == 'loadTemplateBundle') {
      that.loadTemplateBundle(params as LoadTemplateParams);
    } else if (name == 'setGlobalProps') {
      that.setGlobalProps(params as SetGlobalPropsParams);
    } else if (name == 'sendGlobalEvent') {
      that.sendGlobalEvent(params as SendGlobalEventParams);
    } else if (name == 'updateMetaData') {
      that.updateMetaData(params as UpdateMetaDataParams);
    } else if (name == 'updateDataByPreParsedData') {
      that.updateDataByPreParsedData(params as UpdateDataByPreParsedDataParams);
    } else if (name == 'updateViewPort') {
      that.updateViewPort(params as UpdateViewPortParams);
    }
    that.changeReadyState();
  }

  private registerRecordedGenericResources(actions: Map<string, object>[]) {
    this.recordedGenericResources.clear();
    for (const action of actions) {
      if ((action['Function Name'] as string) !== 'LoadComponentWithCallback') {
        continue;
      }
      let params = action['Params'] as LoadComponentWithCallbackParams | undefined;
      if (!params?.url || !params.source) {
        continue;
      }
      let rawSource = this.sanitizeBase64Source(params.source);
      if (rawSource.length === 0) {
        continue;
      }
      try {
        let decoded: Uint8Array = new util.Base64Helper().decodeSync(rawSource, util.Type.BASIC);
        this.recordedGenericResources.set(params.url, this.sliceDecodedBuffer(decoded));
      } catch (error) {
        LLog.e(TAG, `skip recorded generic resource because base64 decode failed. url=${params.url}, error=${error}`);
      }
    }

    if (this.recordedGenericResources.size === 0) {
      this.genericFetcher = this.upstreamGenericFetcher;
      this.templateFetcher = this.upstreamTemplateFetcher;
      return;
    }

    this.genericFetcher = new ReplayGenericResourceFetcher(this.recordedGenericResources, this.upstreamGenericFetcher);
    this.templateFetcher = new ReplayTemplateResourceFetcher(this.recordedGenericResources, this.upstreamTemplateFetcher);
  }

  private async handleActionList(replayGeneration: number) {
    let recordData: Map<string, object> = JSON.parse(this.data);
    let actions: Map<string, object>[] = recordData['Action List'];
    let threadStrategyAction = actions.find((action) => action['Function Name'] === 'setThreadStrategy');
    let threadStrategyParams = threadStrategyAction?.['Params'] as SetThreadStrategyParams | undefined;
    this.replayOptions.applyRecordedEnableJSRuntime(threadStrategyParams?.enableJSRuntime);
    this.registerRecordedGenericResources(actions);
    if (this.replayOptions.enablePreDecode) {
      let loadAction = actions.find((action) => {
        let functionName = action['Function Name'] as string;
        return functionName === 'loadTemplate' || functionName === 'loadTemplateBundle';
      });
      if (loadAction === undefined ||
        !await this.preDecodeTemplate(loadAction['Params'] as LoadTemplateParams, replayGeneration) ||
        replayGeneration !== this.replayGeneration) {
        return;
      }
    }
    let startTime: number | undefined;
    for (const action of actions) {
      if (startTime == undefined) {
        startTime = action['RecordMillisecond'] as number;
      }
      let delayTime = action['RecordMillisecond'] as number - startTime;
      let function_name: string = action['Function Name'] as string;
      if (function_name === 'LoadComponentWithCallback') {
        continue;
      }
      setTimeout(this.dispatchMessage, delayTime, this, replayGeneration, function_name, action['Params']);
    }
  }

  private setGlobalProps(params: SetGlobalPropsParams) {
    let nextGlobalProps = new TemplateData(params.global_props);
    this.globalProps = nextGlobalProps;
    if (this.ready) {
      this.localLynxContext?.updateMetaData(new MetaData(undefined, nextGlobalProps));
    }
  }

  private buildRecordedTemplateData(value?: object | string | null, preprocessorName: string = '',
    readOnly: boolean = false): TemplateData | undefined {
    if (value === null || value === undefined) {
      return undefined;
    }
    let templateData = new TemplateData(value, preprocessorName);
    templateData.readOnly = readOnly;
    return templateData;
  }

  private updateMetaData(params: UpdateMetaDataParams) {
    let nextTemplateData = this.buildRecordedTemplateData(
      params.templateData?.value,
      params.templateData?.preprocessorName ?? '',
      params.templateData?.readOnly ?? false
    );
    let nextGlobalProps: TemplateData | undefined = undefined;
    if (nextTemplateData) {
      this.templateData = nextTemplateData;
    }
    if (params.global_props !== null && params.global_props !== undefined) {
      nextGlobalProps = new TemplateData(params.global_props);
      this.globalProps = nextGlobalProps;
    }
    if (!this.localLynxContext) {
      return;
    }
    if (!nextTemplateData && !nextGlobalProps) {
      return;
    }
    this.localLynxContext.updateMetaData(new MetaData(nextTemplateData, nextGlobalProps));
  }

  private updateDataByPreParsedData(params: UpdateDataByPreParsedDataParams) {
    let nextTemplateData = this.buildRecordedTemplateData(
      params.value,
      params.preprocessorName ?? '',
      params.readOnly ?? false
    );
    if (!nextTemplateData) {
      return;
    }
    this.templateData = nextTemplateData;
    let updateMode = params.updatePageOption?.reset_page_data ? LynxUpdateMode.RESET : LynxUpdateMode.UPDATE;
    this.localLynxContext?.updateMetaData(new LynxUpdateMeta(this.templateData, this.globalProps, updateMode));
  }

  private sendGlobalEvent(params: SendGlobalEventParams) {
    if (params.arguments.length !== 2) {
      return;
    }
    let name: string = params.arguments[0] as string;
    if (name === 'exposure' || name === 'disexposure') {
      return;
    }
    let args: Object[] = params.arguments[1] as object[];
    this.localLynxContext?.sendGlobalEvent(name, args);
  }

  private updateViewPort(params: UpdateViewPortParams) {
    this.localLynxContext?.updateViewport(params.preferredLayoutWidth, params.layoutWidthMode,
      params.preferredLayoutHeight,
      params.layoutHeightMode);
  }

  /*
   * 1. Function: Remove the invalid character '\u0000' at the end of the base64 result.
   * 2. Reason for the presence of invalid characters:
   *    During the recording phase in LynxRecorder, an incorrect estimation of the buffer length for base64 encoding
   *    led to the presence of invalid characters at the end.
   */
  private removeIllegalChar(source: string) {
    if (source.length === 0) {
      return source;
    }
    let lastIndex = source.length - 1;
    while (source[lastIndex] === '\u0000') {
      lastIndex--;
    }
    return source.substring(0, lastIndex + 1);
  }

  private sanitizeBase64Source(source: string): string {
    let normalizedSource = this.removeIllegalChar(source).trim().replace(/\s+/g, '');
    if (normalizedSource.length === 0) {
      return normalizedSource;
    }

    let paddingSize = normalizedSource.length % 4;
    if (paddingSize !== 0) {
      normalizedSource += '='.repeat(4 - paddingSize);
    }
    return normalizedSource;
  }

  private isWrappedUrlChar(char?: string): boolean {
    return char === '`' || char === '\'' || char === '"';
  }

  private sanitizeRecordedUrl(url?: string | null | Object): string {
    if (typeof url !== 'string') {
      return '';
    }

    let normalizedUrl = url.trim();
    while (normalizedUrl.length > 0) {
      let firstChar = normalizedUrl[0];
      let changed = false;
      if (this.isWrappedUrlChar(firstChar)) {
        normalizedUrl = normalizedUrl.substring(1).trim();
        changed = true;
      }
      let lastChar = normalizedUrl.length > 0 ? normalizedUrl[normalizedUrl.length - 1] : undefined;
      if (normalizedUrl.length > 0 && this.isWrappedUrlChar(lastChar)) {
        normalizedUrl = normalizedUrl.substring(0, normalizedUrl.length - 1).trim();
        changed = true;
      }
      if (!changed) {
        break;
      }
    }
    return normalizedUrl;
  }

  private sliceDecodedBuffer(decoded: Uint8Array): ArrayBuffer {
    return decoded.buffer.slice(decoded.byteOffset, decoded.byteOffset + decoded.byteLength);
  }

  private getTemplateBuffer(params: LoadTemplateParams): ArrayBuffer | undefined {
    if (this.preloadedSource !== undefined) {
      return this.preloadedSource.slice(0, this.preloadedSource.byteLength);
    }
    let rawSource = this.sanitizeBase64Source(params.source);
    if (rawSource.length === 0) {
      return undefined;
    }
    let source: Uint8Array = new util.Base64Helper().decodeSync(rawSource, util.Type.BASIC);
    return this.sliceDecodedBuffer(source);
  }

  private setTemplateBundle(templateBundle: TemplateBundle): boolean {
    let errorMessage = templateBundle.getErrorMessage();
    if (errorMessage.length > 0) {
      this.templateBundle = undefined;
      this.bufferReady = false;
      this.templateDataReady = false;
      this.statusText = 'Replay failed: template PreDecode failed.';
      LLog.e(TAG, 'template PreDecode failed: ' + errorMessage);
      return false;
    }
    this.templateBundle = templateBundle;
    return true;
  }

  private async preDecodeTemplate(params: LoadTemplateParams, replayGeneration: number): Promise<boolean> {
    try {
      let buffer = this.getTemplateBuffer(params);
      if (buffer === undefined) {
        this.bufferReady = false;
        this.templateDataReady = false;
        this.statusText = 'Replay failed: PreDecode requires template source.';
        LLog.e(TAG, 'template PreDecode failed: template source is unavailable.');
        return false;
      }
      let templateBundle = await TemplateBundle.fromTemplateAsyncWithOption(buffer, this.templateBundleOption);
      if (replayGeneration !== this.replayGeneration) {
        return false;
      }
      if (!this.setTemplateBundle(templateBundle)) {
        return false;
      }
      this.buffer = buffer;
      this.bufferReady = true;
      LLog.i(TAG, `template PreDecode succeeded. size=${buffer.byteLength}, url=${this.pageUrl}`);
      return true;
    } catch (error) {
      if (replayGeneration !== this.replayGeneration) {
        return false;
      }
      this.bufferReady = false;
      this.templateDataReady = false;
      this.statusText = 'Replay failed: template PreDecode failed.';
      LLog.e(TAG, 'template PreDecode failed: ' + error);
      return false;
    }
  }

  private resetReplayLoadState() {
    this.ready = false;
    this.pageUrl = LynxRecorderEnv.RECORDER_URL_PREFIX;
    this.buffer = undefined;
    this.templateBundle = undefined;
    this.bufferReady = false;
    this.loadFromUrl = false;
    this.templateData = undefined;
    this.templateDataReady = false;
    this.statusText = 'Loading...';
  }

  private loadTemplate(params: LoadTemplateParams) {
    this.loadFromUrl = false;
    this.templateBundle = undefined;
    let sanitizedUrl = this.sanitizeRecordedUrl(params.url);
    this.pageUrl = sanitizedUrl.length > 0 ? sanitizedUrl : LynxRecorderEnv.RECORDER_URL_PREFIX;
    let base64Engine = new util.Base64Helper();
    let rawSource = this.sanitizeBase64Source(params.source);
    if (this.preloadedSource !== undefined) {
      this.buffer = this.preloadedSource.slice(0, this.preloadedSource.byteLength);
      this.bufferReady = true;
      this.templateData = new TemplateData(params.templateData);
      this.templateDataReady = true;
      return;
    }
    if (rawSource.length === 0) {
      this.buffer = undefined;
      this.bufferReady = false;
      this.templateDataReady = false;
      this.statusText = 'Replay failed: template source is empty.';
      LLog.e(TAG, 'loadTemplate failed: source is empty after sanitizing.');
      return;
    }

    try {
      let source: Uint8Array = base64Engine.decodeSync(rawSource, util.Type.BASIC);
      this.buffer = this.sliceDecodedBuffer(source);
      this.bufferReady = true;
    } catch (error) {
      this.buffer = undefined;
      this.bufferReady = false;
      this.templateDataReady = false;
      this.statusText = 'Replay failed: invalid template source.';
      LLog.e(TAG, `loadTemplate decodeSync failed: ${error}`);
      return;
    }
    this.templateData = new TemplateData(params.templateData);
    this.templateDataReady = true;
  }

  private loadTemplateBundle(params: LoadTemplateParams) {
    if (!this.replayOptions.enablePreDecode) {
      this.templateBundle = undefined;
    }
    let sanitizedUrl = this.sanitizeRecordedUrl(params.url);
    this.pageUrl = sanitizedUrl.length > 0 ? sanitizedUrl : LynxRecorderEnv.RECORDER_URL_PREFIX;
    this.templateData = new TemplateData(params.templateData);
    this.templateDataReady = true;
    if (this.replayOptions.enablePreDecode && this.templateBundle !== undefined) {
      this.loadFromUrl = false;
      this.bufferReady = true;
      return;
    }

    let rawSource = this.sanitizeBase64Source(params.source);
    if (this.preloadedSource === undefined && rawSource.length === 0) {
      this.buffer = undefined;
      this.bufferReady = false;
      if (this.replayOptions.enablePreDecode) {
        this.loadFromUrl = false;
        this.templateDataReady = false;
        this.statusText = 'Replay failed: PreDecode requires template source.';
        LLog.e(TAG, 'loadTemplateBundle failed: PreDecode requires template source bytes.');
        return;
      }
      if (sanitizedUrl.length === 0) {
        this.loadFromUrl = false;
        this.templateDataReady = false;
        this.statusText = 'Replay failed: template source and url are empty.';
        LLog.e(TAG, 'loadTemplateBundle failed: source and url are both empty.');
        return;
      }
      if (!this.templateFetcher) {
        this.loadFromUrl = false;
        this.templateDataReady = false;
        this.statusText = 'Replay failed: template fetcher is unavailable.';
        LLog.e(TAG, `loadTemplateBundle failed: source is empty and template fetcher is unavailable. url=${sanitizedUrl}`);
        return;
      }
      this.loadFromUrl = true;
      return;
    }

    this.loadFromUrl = false;
    let buffer: ArrayBuffer | undefined;
    try {
      buffer = this.getTemplateBuffer(params);
    } catch (error) {
      this.buffer = undefined;
      this.bufferReady = false;
      this.templateDataReady = false;
      this.statusText = 'Replay failed: invalid template source.';
      LLog.e(TAG, 'loadTemplateBundle decode failed: ' + error);
      return;
    }
    if (buffer === undefined) {
      this.bufferReady = false;
      this.templateDataReady = false;
      this.statusText = 'Replay failed: template source is unavailable.';
      LLog.e(TAG, 'loadTemplateBundle failed: template source buffer is unavailable.');
      return;
    }
    try {
      let templateBundle = TemplateBundle.fromTemplateWithOption(buffer, this.templateBundleOption);
      if (!this.setTemplateBundle(templateBundle)) {
        return;
      }
      this.buffer = buffer;
      this.bufferReady = true;
      LLog.i(TAG, `template PreDecode succeeded. size=${buffer.byteLength}, url=${this.pageUrl}`);
    } catch (error) {
      this.buffer = undefined;
      this.bufferReady = false;
      this.templateDataReady = false;
      this.statusText = 'Replay failed: template PreDecode failed.';
      LLog.e(TAG, 'loadTemplateBundle PreDecode failed: ' + error);
      return;
    }
  }

  async aboutToAppear() {
    // mock extraTiming here
    const replayGeneration = ++this.replayGeneration;
    this.resetReplayLoadState();
    this.upstreamGenericFetcher = this.genericFetcher;
    this.upstreamTemplateFetcher = this.templateFetcher;
    this.replayOptions = new LynxRecorderReplayOptions(this.url);
    this.extraTiming.openTime = new Date().getTime();
    this.extraTiming.containerInitStart = new Date().getTime();
    this.extraTiming.containerInitEnd = new Date().getTime();
    this.extraTiming.prepareTemplateStart = new Date().getTime();
    this.downloadSourceOverride(replayGeneration);
    this.extraTiming.prepareTemplateEnd = new Date().getTime();
  }

  build() {
    Column() {
      Row() {
        Row() {
          Image("resources/rawfile/lynx_recorder.svg").width(28).height(28)
          Text('LynxRecorder').fontSize(20).fontStyle(FontStyle.Italic).fontWeight('800').margin({ bottom: 5, left: 5 })
        }

        Stack()
          .width(24)
          .height(24)
          .borderRadius(12)
          .borderWidth(3)
          .onClick(() => {
            const replayGeneration = ++this.replayGeneration;
            this.resetReplayLoadState();
            this.handleActionList(replayGeneration);
          })
          .stateStyles({
            pressed: {
              .borderColor(Color.Green)
            },
            normal: {
              .borderColor(Color.Black)
            }
          })
          .margin({ right: 15 })
      }
      .width('100%')
      .height(50)
      .padding({ left: 10, right: 10 })
      .justifyContent(FlexAlign.SpaceBetween)
      .borderWidth({ bottom: 1 })

      if (this.ready) {
        LynxView({
          url: this.pageUrl,
          buffer: this.buffer,
          templateBundle: this.templateBundle,
          enableAirStrictMode: this.replayOptions.enableAirStrictMode,
          uiRendererCreator: this.uiRendererCreator,
          genericResourceFetcher: this.genericFetcher,
          mediaResourceFetcher: this.mediaFetcher,
          templateResourceFetcher: this.templateFetcher,
          modules: this.jsbModule,
          metaData: new MetaData(this.templateData, this.globalProps),
          onCreate: (context) => {
            this.localLynxContext = context;
            this.localLynxContext.setExtraTiming(this.extraTiming);
          }
        })
      } else {
          Text(this.statusText).fontSize(24).textAlign(TextAlign.Center).padding(24)
      }
    }.size({ width: '100%', height: '100%' })
  }
}
