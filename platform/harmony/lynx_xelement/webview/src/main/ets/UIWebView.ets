// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { webview } from '@kit.ArkWeb';
import { LynxUIMethodConstants, UIBase } from '@lynx/lynx';
import { BuilderNode, NodeController, UIContext } from '@kit.ArkUI';

const TAG = 'x-webview';

interface LynxWebViewErrorEvent {
  errorCode: number,
  errorMsg: string,
}

interface LynxWebViewMessageEvent {
  msg: string,
}

interface LynxWebViewEvalParams {
  func: string
}

class LynxJSBridge {
  private webviewNodeController: LynxWebViewNodeController;

  constructor(webviewNodeController: LynxWebViewNodeController) {
    this.webviewNodeController = webviewNodeController;
  }

  onMessageReceived(message: string): void {
    this.webviewNodeController.onMessage(message);
  }
}

@ObservedV2
export class WebViewParams {
  constructor(ui: UIWebView) {
    this.ui = ui;
  }

  @Trace url: string = '';
  @Trace html: string = '';
  ui: UIWebView;
}

export interface LynxWebViewLoader {
  getController(): webview.WebviewController;

  setParams(params: object): void;

  load(url: string): void;

  loadHtmlString(htmlString: string): void;

  reload(): void;

  evaluateJavaScript(script: string): Promise<string>;
}

export abstract class LynxWebViewNodeController extends NodeController implements LynxWebViewLoader {
  private jsBridge: LynxJSBridge = new LynxJSBridge(this);
  private webviewParams: WebViewParams;

  abstract getController(): webview.WebviewController;

  abstract setParams(params: object): void;

  abstract load(url: string): void;

  abstract loadHtmlString(htmlString: string): void;

  abstract reload(): void;

  abstract evaluateJavaScript(script: string): Promise<string>;

  abstract makeWebViewNode(context: UIContext, javaScriptProxy: JavaScriptProxy,
    javaScriptOnDocumentEnd: Array<ScriptItem>): FrameNode | null;

  constructor(webviewParams: WebViewParams) {
    super();
    this.webviewParams = webviewParams;
  }

  makeNode(uiContext: UIContext): FrameNode | null {
    return this.makeWebViewNode(uiContext, {
      object: this.jsBridge,
      name: "LynxJSBridge",
      methodList: ['onMessageReceived'],
      controller: this.getController()
    }, [{
      script: '(function() {window.addEventListener(\'message\', function(event) { LynxJSBridge.onMessageReceived(event.data);})})()',
      scriptRules: ["*"]
    }]);
  }

  onControllerAttached(): void {
    this.webviewParams.ui.attached = true;
    this.webviewParams.ui.updateSrc(true, this.webviewParams.url, this.webviewParams.html);
  }

  aboutToDisappear(): void {
    this.getController().deleteJavaScriptRegister("LynxJSBridge");
  }

  public onLoad(): void {
    this.webviewParams.ui.sendEvent('load', Object);
  }

  public onMessage(message: string): void {
    this.webviewParams.ui.sendEvent('message', { msg: message } as LynxWebViewMessageEvent);
  }

  public onError(error: LynxWebViewErrorEvent): void {
    this.webviewParams.ui.sendEvent('error', error);
  }
}


export class LynxDefaultWebViewInitParams {
  nodeController: LynxDefaultWebViewNodeController
  javaScriptProxy: JavaScriptProxy
  javaScriptOnDocumentEnd: Array<ScriptItem>

  constructor(nodeController: LynxDefaultWebViewNodeController,
    javaScriptProxy: JavaScriptProxy,
    javaScriptOnDocumentEnd: Array<ScriptItem>) {
    this.nodeController = nodeController
    this.javaScriptProxy = javaScriptProxy
    this.javaScriptOnDocumentEnd = javaScriptOnDocumentEnd
  }
}

@ComponentV2
struct LynxDefaultWebView {
  @BuilderParam initParams: LynxDefaultWebViewInitParams;

  build() {
    Stack() {
      Web({ src: '', controller: this.initParams.nodeController.getController() })
        .backgroundColor(Color.Transparent)
        .onControllerAttached(() => {
          this.initParams.nodeController.onControllerAttached()
        })
        .javaScriptProxy(this.initParams.javaScriptProxy)
        .javaScriptOnDocumentEnd(this.initParams.javaScriptOnDocumentEnd)
        .onPageEnd((event) => {
          this.initParams.nodeController.onLoad();
        })
        .onErrorReceive((event) => {
          this.initParams.nodeController.onError({
            errorCode: event.error.getErrorCode(),
            errorMsg: event.error.getErrorInfo()
          } as LynxWebViewErrorEvent);
        })
    }
    .width('100%')
    .height('100%')
  }
}

@Builder
function lynxDefaultWebViewBuilder(initParams: LynxDefaultWebViewInitParams) {
  LynxDefaultWebView({ initParams });
}

export class LynxDefaultWebViewNodeController extends LynxWebViewNodeController {
  private webviewNode: BuilderNode<[LynxDefaultWebViewInitParams]> | null = null;
  public controller: webview.WebviewController = new webview.WebviewController();

  getController(): webview.WebviewController {
    return this.controller;
  }

  setParams(params: object): void {

  }

  load(url: string): void {
    this.controller.loadUrl(url);
  }

  loadHtmlString(htmlString: string): void {
    this.controller.loadData(encodeURIComponent(htmlString), "text/html", "UTF-8");
  }

  reload(): void {
    this.controller.refresh();
  }

  evaluateJavaScript(script: string): Promise<string> {
    return this.controller.runJavaScript(script);
  }

  makeWebViewNode(context: UIContext, javaScriptProxy: JavaScriptProxy,
    javaScriptOnDocumentEnd: Array<ScriptItem>): FrameNode | null {
    this.webviewNode = new BuilderNode(context);
    this.webviewNode.build(wrapBuilder<[LynxDefaultWebViewInitParams]>(lynxDefaultWebViewBuilder),
      new LynxDefaultWebViewInitParams(this, javaScriptProxy, javaScriptOnDocumentEnd));
    return this.webviewNode.getFrameNode();
  }
}


@Builder
export function buildWebViewNodeContainer(ui: UIBase) {
  if (ui as UIWebView) {
    NodeContainer((ui as UIWebView).webviewNodeController)
      .width('100%')
      .height('100%')
  }
}

export class UIWebView extends UIBase {
  webviewParams: WebViewParams = new WebViewParams(this);
  webviewNodeController?: LynxWebViewNodeController;
  webviewType: string = "default";
  attached: boolean = false;
  readonly builder: WrappedBuilder<[UIBase]> = wrapBuilder<[UIBase]>(buildWebViewNodeContainer);

  layout(x: number, y: number, width: number, height: number, paddingLeft: number, paddingTop: number,
    paddingRight: number, paddingBottom: number, marginLeft: number, marginTop: number, marginRight: number,
    marginBottom: number, sticky?: number[] | undefined): void {
  }

  update(props: Record<string, Object>): void {
    super.update(props);
    let urlChanged: boolean = false;
    let htmlChanged: boolean = false;
    for (const entry of Object.entries(props)) {
      if (entry[0] == 'src') {
        this.webviewParams.url = entry[1] as string;
        urlChanged = true;
      } else if (entry[0] == 'html') {
        this.webviewParams.html = entry[1] as string;
        htmlChanged = true;
      } else if (entry[0] == 'enable-debug') {
        webview.WebviewController.setWebDebuggingAccess(entry[1] as boolean);
      } else if (entry[0] == 'webview-type') {
        this.webviewType = entry[1] as string;
        if (this.webviewType !== 'default' && LynxWebViewService.has(this.webviewType)) {
          this.webviewNodeController =
            LynxWebViewService.get(this.webviewType)!.createLynxWebViewNodeController(this.webviewParams)
          if (this.webviewNodeController) {
            this.builderNode?.update(this);
          }
        }
      }
    }

    if (!this.webviewNodeController) {
      this.webviewNodeController = new LynxDefaultWebViewNodeController(this.webviewParams);
      this.builderNode?.update(this);
    }

    if (this.attached) {
      if (urlChanged || htmlChanged) {
        this.updateSrc(urlChanged, this.webviewParams.url, this.webviewParams.html);
      }
    }
  }

  updateSrc(urlChanged: boolean, url?: string, html?: string): void {
    if (url?.length && urlChanged) {
      this.webviewNodeController?.load(url);
    } else if (html?.length && !url?.length) {
      this.webviewNodeController?.loadHtmlString(html);
    }
    if (!url?.length && !html?.length) {
      this.sendEvent('error',
        { errorCode: -1, errorMsg: 'invalid input: src and html are empty' } as LynxWebViewErrorEvent);
    }
  }

  invokeMethod(method: string, params: Object, callback: (code: number, res: Object) => void): boolean {
    switch (method) {
      case 'reload':
        this.webviewNodeController?.reload();
        callback(LynxUIMethodConstants.SUCCESS, new Object());
        break;
      case 'eval':
        const webviewEvalParams = (params as LynxWebViewEvalParams);
        this.webviewNodeController?.evaluateJavaScript(webviewEvalParams.func);
        callback(LynxUIMethodConstants.SUCCESS, new Object());
        break;
      default:
        return false;

    }
    return super.invokeMethod(method, params, callback);
  }


  sendEvent(name: string, params: Object) {
    super.sendCustomEvent(name, params, 'detail');
  }
}

export let LynxWebViewService = new Map<string, LynxWebViewProvider>();


export abstract class LynxWebViewProvider {
  abstract createLynxWebViewNodeController(webviewParams: WebViewParams): LynxWebViewNodeController;
}
