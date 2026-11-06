// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import lynx from 'liblynx.so';
import util from '@ohos.util';
import { LynxBackgroundRuntime, LynxBackgroundRuntimeClient } from './LynxBackgroundRuntime';
import { LynxBackgroundRuntimeOptions } from '../jsbridge/LynxBackgroundRuntimeOptions';
import { ModuleClassWrapper } from '../jsbridge/LynxModule';
import { LynxGenericResourceFetcher } from '../provider/LynxGenericResourceFetcher';
import { LynxMediaResourceFetcher } from '../provider/LynxMediaResourceFetcher';
import { LynxTemplateResourceFetcher } from '../provider/LynxTemplateResourceFetcher';
import { LynxResourceRequest, LynxResourceType } from '../provider/LynxResourceRequest';
import { LynxContext } from './LynxContext';
import { LynxViewClient } from './LynxViewClient';
import { LynxError } from './LynxError';
import { MetaData, TemplateData } from './MetaData';

/** Options for a debug-only, standalone Host Script runtime. */
export interface LynxHostScriptRuntimeOptions {
  backgroundRuntimeOptions?: LynxBackgroundRuntimeOptions;
  genericResourceFetcher?: LynxGenericResourceFetcher;
  mediaResourceFetcher?: LynxMediaResourceFetcher;
  templateResourceFetcher?: LynxTemplateResourceFetcher;
  modules?: Map<string, ModuleClassWrapper>;
}

export class LynxHostScriptError extends Error {
  readonly code: string;
  constructor(code: string, message: string) {
    super(message);
    this.code = code;
    this.name = 'LynxHostScriptError';
  }
}

export interface LynxHostScriptCallback {
  onSuccess(): void;
  onFailure(error: LynxHostScriptError): void;
}

class RuntimeClient extends LynxBackgroundRuntimeClient {
  private owner: LynxHostScriptRuntime;
  constructor(owner: LynxHostScriptRuntime) { super(); this.owner = owner; }
  onEvaluateJavaScriptEnd(url: string): void { this.owner.evaluationEnded(url); }
  onReceivedError(error: LynxError): void {
    this.owner.evaluationFailed(error.getSummaryMessage());
  }
}

class ViewClient extends LynxViewClient {
  private owner: LynxHostScriptRuntime;
  constructor(owner: LynxHostScriptRuntime) { super(); this.owner = owner; }
  onLoadSuccess(url: string): void { this.owner.notify('loadSuccess'); }
  onFirstScreen(): void { this.owner.notify('firstScreen'); }
  onPageUpdate(): void { this.owner.notify('pageUpdate'); }
  onDataUpdated(): void { this.owner.notify('dataUpdated'); }
  onUpdateDataWithoutChange(): void { this.owner.notify('dataUpdated'); }
  onReceivedError(error: LynxError): void {
    this.owner.notify('error', error.getErrorCode(), error.getSummaryMessage());
  }
  // Renderer teardown also happens during loadTemplate. View lifetime is
  // observed through LynxContext instead of the renderer's onDestroy callback.
}

/**
 * Executes one bundled Host Script on its own JS runtime. Create, bind and
 * destroy this object on UI. It never owns or destroys its bound LynxView.
 * Available only with a Debug native library. SSR is currently unsupported.
 */
export class LynxHostScriptRuntime {
  private runtime: LynxBackgroundRuntime;
  private binding: lynx.LynxHostScriptBinding;
  private state: string = 'created';
  private view?: WeakRef<LynxContext>;
  private bound: boolean = false;
  private viewClient: ViewClient = new ViewClient(this);
  private runtimeClient: RuntimeClient = new RuntimeClient(this);
  private startCallback?: LynxHostScriptCallback;
  private evaluationCallback?: LynxHostScriptCallback;
  private sourceRequest?: LynxResourceRequest;
  private startUrl: string = '';
  private evaluationUrl: string = '';
  private evaluationSequence: number = 0;
  private registered: boolean = false;
  private entryReady: boolean = false;
  private evaluationComplete: boolean = false;
  private onViewDestroyed: () => void = () => {
    this.unbind();
    this.notify('destroyed');
  };

  static create(options: LynxHostScriptRuntimeOptions = {}): LynxHostScriptRuntime {
    if (typeof lynx.LynxHostScriptBinding !== 'function') {
      throw new LynxHostScriptError('UNSUPPORTED', 'Host Script requires a Debug native library');
    }
    return new LynxHostScriptRuntime(options);
  }

  private options: LynxHostScriptRuntimeOptions;
  private constructor(options: LynxHostScriptRuntimeOptions) {
    this.options = options;
    this.runtime = new LynxBackgroundRuntime(
      options.backgroundRuntimeOptions ?? new LynxBackgroundRuntimeOptions(),
      options.genericResourceFetcher, options.mediaResourceFetcher,
      options.templateResourceFetcher, options.modules, undefined, undefined);
    this.runtime.addLifecycleClient(this.runtimeClient);
    this.binding = new lynx.LynxHostScriptBinding(this.runtime, this);
  }

  bindLynxView(context: LynxContext): void {
    if (this.state === 'destroyed' || this.bound || !context.getLynxViewClient()) {
      throw new LynxHostScriptError('INVALID_STATE', 'Bind exactly one live LynxView');
    }
    this.view = new WeakRef(context);
    context.getLynxViewClient()?.push(this.viewClient);
    context.addViewDestroyListener(this.onViewDestroyed);
    if (!this.binding.bind()) {
      this.unbind();
      throw new LynxHostScriptError('INVALID_STATE', 'Cannot bind LynxView');
    }
    this.bound = true;
  }

  startJavaScript(url: string, callback: LynxHostScriptCallback): void;
  startJavaScript(url: string, source: string, callback: LynxHostScriptCallback): void;
  startJavaScript(url: string, sourceOrCallback: string | LynxHostScriptCallback,
    callback?: LynxHostScriptCallback): void {
    const completion = typeof sourceOrCallback === 'string' ? callback : sourceOrCallback;
    if (!completion) {
      throw new LynxHostScriptError('INVALID_STATE', 'A callback is required');
    }
    if (this.state !== 'created' || !url) {
      completion.onFailure(new LynxHostScriptError('INVALID_STATE', 'Start exactly one main script with a URL'));
      return;
    }
    this.startCallback = completion;
    this.startUrl = this.normalizeUrl(url);
    if (typeof sourceOrCallback === 'string') {
      this.startSource(sourceOrCallback);
      return;
    }
    this.state = 'loading';
    const fetcher = this.options.genericResourceFetcher;
    if (!fetcher) {
      this.failStart('SOURCE_LOAD', 'No generic resource fetcher is configured');
      return;
    }
    const request = new LynxResourceRequest(url, LynxResourceType.LYNX_RESOURCE_TYPE_EXTERNAL_JS_SOURCE);
    this.sourceRequest = request;
    try {
      fetcher.fetchResource(request, (error, bytes) => {
        if (this.state !== 'loading' || this.sourceRequest !== request) return;
        this.sourceRequest = undefined;
        if (error || !bytes || bytes.byteLength === 0) {
          this.failStart('SOURCE_LOAD', error?.message ?? 'Empty Host Script source');
          return;
        }
        try {
          const decoder = util.TextDecoder.create('utf-8', { fatal: true, ignoreBOM: true });
          this.startSource(decoder.decodeToString(new Uint8Array(bytes)));
        } catch (error) {
          this.failStart('SOURCE_LOAD', String(error));
        }
      });
    } catch (error) {
      this.failStart('SOURCE_LOAD', String(error));
    }
  }

  evaluateJavaScript(url: string, source: string, callback: LynxHostScriptCallback): void {
    if (this.state !== 'ready' || this.evaluationCallback || !url) {
      callback.onFailure(new LynxHostScriptError('INVALID_STATE', 'Evaluation requires an idle, ready runtime'));
      return;
    }
    this.evaluationCallback = callback;
    // Standalone runtime script caches are keyed by URL. Each evaluation must
    // execute the supplied source, including repeated evaluations of one URL.
    const normalized = this.normalizeUrl(url);
    this.evaluationUrl = normalized + (normalized.includes('?') ? '&' : '?') +
      'host_script_evaluation=' + (++this.evaluationSequence);
    this.runtime.evaluateJavaScript(this.evaluationUrl, this.wrapSource(source));
  }

  destroy(): void {
    if (this.state === 'destroyed') return;
    this.state = 'destroyed';
    const start = this.startCallback;
    const evaluation = this.evaluationCallback;
    this.startCallback = undefined;
    this.evaluationCallback = undefined;
    const request = this.sourceRequest;
    this.sourceRequest = undefined;
    try {
      if (request) this.options.genericResourceFetcher?.cancel(request);
    } catch (_) {}
    this.unbind();
    this.binding.destroy();
    this.runtime.removeLifecycleClient(this.runtimeClient);
    this.runtime.destroy();
    const error = new LynxHostScriptError('INVALID_STATE', 'Host Script runtime was destroyed');
    start?.onFailure(error);
    evaluation?.onFailure(error);
  }

  private unbind(): void {
    const context = this.view?.deref();
    const clients = context?.getLynxViewClient();
    const index = clients?.indexOf(this.viewClient) ?? -1;
    if (index >= 0) clients?.splice(index, 1);
    context?.removeViewDestroyListener(this.onViewDestroyed);
    this.view = undefined;
  }

  private startSource(source: string): void {
    this.state = 'starting';
    this.runtime.evaluateJavaScript(this.startUrl, this.wrapSource(source));
  }

  private normalizeUrl(url: string): string { return url.startsWith('/') ? url : '/' + url; }
  private wrapSource(source: string): string {
    return '(function(){var g=(0,eval)("this");g.initBundle=function(__hostScriptRuntime){' +
      'var lynx=__hostScriptRuntime.tt.lynx;\n' + source + '\n};})();';
  }

  /** Internal platform callback; always called on UI. */
  entryResult(args: string[]): void {
    if (this.state !== 'starting') return;
    switch (args[0]) {
      case 'REGISTERED':
        if (this.registered) this.failStart('INVALID_ENTRY', 'defineHostScript was called more than once');
        this.registered = true;
        break;
      case 'READY': this.entryReady = true; break;
      case 'ERROR': this.failStart('ENTRY_REJECTED', args[1]); break;
      default: this.failStart('INVALID_ENTRY', args[1]); break;
    }
    this.finishStart();
  }

  evaluationEnded(url: string): void {
    if (this.state === 'starting' && url === this.startUrl) {
      this.evaluationComplete = true;
      if (!this.registered) this.failStart('INVALID_ENTRY', 'defineHostScript must be called synchronously');
      this.finishStart();
    } else if (this.state === 'ready' && url === this.evaluationUrl) {
      const callback = this.evaluationCallback;
      this.evaluationCallback = undefined;
      callback?.onSuccess();
    }
  }

  evaluationFailed(message: string): void {
    if (this.state === 'starting') this.failStart('SCRIPT_EVALUATION', message);
    else if (this.state === 'ready') {
      const callback = this.evaluationCallback;
      this.evaluationCallback = undefined;
      callback?.onFailure(new LynxHostScriptError('SCRIPT_EVALUATION', message));
    }
  }

  private finishStart(): void {
    if (this.state !== 'starting' || !this.registered || !this.entryReady || !this.evaluationComplete) return;
    this.state = 'ready';
    const callback = this.startCallback;
    this.startCallback = undefined;
    callback?.onSuccess();
  }

  private failStart(code: string, message: string): void {
    if (this.state === 'destroyed') return;
    this.state = 'failed';
    const callback = this.startCallback;
    this.startCallback = undefined;
    callback?.onFailure(new LynxHostScriptError(code, message));
  }

  notify(event: string, code: number = 0, message: string = ''): void {
    if (this.state !== 'destroyed') this.binding.notify(event, code, message);
  }

  /** Internal platform operation. Arguments were copied on the JS owner. */
  dispatch(args: string[], bytes: ArrayBuffer): void {
    const context = this.view?.deref();
    if (this.state === 'destroyed') return;
    if (!context || !context.getLynxViewClient()) {
      this.onViewDestroyed();
      return;
    }
    try {
      switch (args[0]) {
        case 'loadURL':
        case 'loadTemplate': {
          const initial = this.data(args[2]);
          if (initial) {
            initial.processor = args[4];
            initial.readOnly = args[5] === 'true';
          }
          context.loadTemplate(args[1], undefined, args[0] === 'loadTemplate' ? bytes : undefined,
            new MetaData(initial, this.data(args[3])));
          break;
        }
        case 'updateMetaData':
          context.updateMetaData(new MetaData(this.data(args[1]), this.data(args[2])));
          break;
        case 'setGlobalProps':
          context.setGlobalProps(new TemplateData(args[1]));
          break;
        case 'reloadTemplate':
          context.reload(new MetaData(this.data(args[1]), this.data(args[2])));
          break;
        case 'sendGlobalEvent': context.sendGlobalEvent(args[1], JSON.parse(args[2]) as Object[]); break;
      }
    } catch (error) {
      this.notify('error', -1, String(error));
    }
  }

  private data(json: string): TemplateData | undefined { return json ? new TemplateData(json) : undefined; }
}
