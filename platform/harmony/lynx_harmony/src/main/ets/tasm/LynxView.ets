// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BusinessError } from '@ohos.base';
import { LLog } from '../base/LLog';
import { LynxTemplateRenderer } from './LynxTemplateRenderer';
import { LynxModuleManager, SendableLynxModuleManager } from '../jsbridge/LynxModuleManager';
import { HarmonyUIRenderer } from './behavior/HarmonyUIRenderer';
import { LynxContext } from './LynxContext';
import { LynxResourceRequest, LynxResourceType } from '../provider/LynxResourceRequest';
import { LynxEnv } from './LynxEnv';
import { MetaData } from './MetaData';
import { BehaviorRegistryMap } from './behavior/Behavior';
import { ModuleClassWrapper, SendableModuleClassWrapper } from '../jsbridge/LynxModule';
import { LynxPipelineInfo, LynxPipelineOrigin, LynxViewClient } from './LynxViewClient';
import { CAPIView } from './behavior/CAPIView';
import { LynxError } from './LynxError';
import { LynxSubErrorCode } from './gen/LynxSubErrorCode';
import { BuilderNode, FrameNode, NodeController, Size, UIContext, window } from '@kit.ArkUI';
import { TraceCategory, TraceEvent } from '../base/TraceEvent';
import { TraceEventDef } from '../base/TraceEventDef';
import { PerformanceController } from './performance/PerformanceController';
import { TimingOption } from './performance/timing/TimingOption';
import { TimingConstants } from './performance/timing/TimingConstants';
import { LynxUIRenderer } from './LynxUIRenderer';
import { LynxGenericResourceFetcher } from '../provider/LynxGenericResourceFetcher';
import { LynxMediaResourceFetcher } from '../provider/LynxMediaResourceFetcher';
import { LynxTemplateResourceFetcher, TemplateProviderResult } from '../provider/LynxTemplateResourceFetcher';
import { LynxEventReporter } from './eventreport/LynxEventReporter';
import { LynxFetchModule } from '../jsbridge/network/LynxFetchModule';
import { LynxResourceModule } from '../jsbridge/LynxResourceModule';
import { LynxAccessibilityModule } from '../jsbridge/LynxAccessibilityModule';
import { LynxBackgroundRuntimeOptions } from '../jsbridge/LynxBackgroundRuntimeOptions';
import { LynxInfoReporterHelper, LynxTemplateType } from './eventreport/LynxInfoReporterHelper';
import { TemplateBundle } from './TemplateBundle';
import { LynxLoadOption } from './LynxLoadOption';
import { LynxServiceCenter, LynxServiceType } from './service/LynxServiceCenter';
import { ILynxSecurityService, LynxTasmType, SecurityResult } from './service/ILynxSecurityService';
import { LynxSetModule } from '../jsbridge/LynxSetModule';
import { ILynxExtensionService } from './service/ILynxExtensionService';
import { LynxUIRendererCreator } from './LynxUIRendererCreator';
import { url } from '@kit.ArkTS';
import { ILynxTrailService, LynxBackgroundRuntime, LynxResourceLoadInfo } from '../Index';
import { ILogBox } from '../devtool_wrapper/ILogBox';
import { LynxDevTool } from '../devtool_wrapper/LynxDevTool';
import { ILynxDevToolService } from './service/ILynxDevToolService';
import { LynxImageConfig } from './image/LynxImageConfig';

export enum MeasureMode {
  INDEFINITE = 0,
  DEFINITE = 1,
  AT_MOST = 2
}

export enum ThreadStrategyForRendering {
  ALL_ON_UI = 0,
  MOST_ON_TASM = 1,
}

const TAG = 'LynxView';

export interface LynxViewBuilder {
  genericResourceFetcher?: LynxGenericResourceFetcher;
  mediaResourceFetcher?: LynxMediaResourceFetcher;
  templateResourceFetcher?: LynxTemplateResourceFetcher;
  modules?: Map<string, ModuleClassWrapper>;
  sendableModules?: Map<string, SendableModuleClassWrapper>;
  behaviors?: BehaviorRegistryMap;
  screenSize?: Size;
  uiRendererCreator?: LynxUIRendererCreator;
  backgroundRuntime?: LynxBackgroundRuntime;
  platformConfig?: string;
  clients?: LynxViewClient[];
  fitContentWidth?: boolean;
  fitContentHeight?: boolean;
  experimentalFixFitContent?: boolean;
  beforeLoadTemplate?: (context: LynxContext) => void;
  threadMode?: ThreadStrategyForRendering;
  backgroundRuntimeOptions?: LynxBackgroundRuntimeOptions;
  enableAirStrictMode?: boolean;
  lynxImageConfig?: LynxImageConfig;
  fontScale?: number;
}

@Builder
function buildLynxView(params: ESObject) {
  CAPIView({
    renderer: params.renderer as HarmonyUIRenderer
  })
    .width('100%')
    .height(params.fitContentHeight ? 'auto' : '100%')
    .id((params.renderer as HarmonyUIRenderer).uiOwner.getId())
}

export abstract class LynxNodeController extends NodeController {
  renderer?: LynxUIRenderer;
  fitContentHeight?: boolean;
  builderNode: BuilderNode<[ESObject]> | undefined | null;

  updateNode(renderer: LynxUIRenderer, fitContentHeight: boolean) {
    this.dispose();
    this.renderer = renderer;
    this.fitContentHeight = fitContentHeight;
    this.rebuild();
  }

  dispose() {
    this.renderer = undefined;
    this.builderNode?.dispose();
    this.builderNode = undefined;
  }
}

/**
 * CAPINodeController is used to build CAPIView for LynxView.
 * Use BuilderNode to create dynamically.
 */
class CAPINodeController extends LynxNodeController {
  override makeNode(uiContext: UIContext): FrameNode | null {
    if (!this.renderer) {
      return null;
    }
    if (!this.builderNode) {
      this.builderNode = new BuilderNode<[ESObject]>(uiContext);
      this.builderNode.build(wrapBuilder<[ESObject]>(buildLynxView), {
        fitContentHeight: this.fitContentHeight,
        renderer: this.renderer,
      });
    }
    return this.builderNode.getFrameNode();
  }
}

@Component
export struct LynxView {
  genericResourceFetcher?: LynxGenericResourceFetcher;
  mediaResourceFetcher?: LynxMediaResourceFetcher;
  templateResourceFetcher?: LynxTemplateResourceFetcher;
  modules: Map<string, ModuleClassWrapper> = new Map();
  sendableModules: Map<string, SendableModuleClassWrapper> = new Map();
  metaData: MetaData = new MetaData();
  behaviors: BehaviorRegistryMap = new Map();
  url: string = '';
  screenSize?: Size;
  buffer?: ArrayBuffer;
  templateBundle?: TemplateBundle;
  loadOption?: LynxLoadOption;
  uiRendererCreator?: LynxUIRendererCreator;
  backgroundRuntime?: LynxBackgroundRuntime;
  @StorageLink('lynxWindowStage') windowStage: window.WindowStage | undefined = undefined;
  /**
   * URL for LynxView, which will be parsed by ILynxTrailService in building LynxView.
   */
  uri?: url.URL;
  /**
   * Platform config to change page config on ets
   */
  platformConfig?: string;
  logBox: ILogBox | null = null;
  private nodeController?: LynxNodeController;
  clients: LynxViewClient[] = [];
  @Prop fitContentWidth: boolean = false;
  @Prop fitContentHeight: boolean = false;
  //TODO(chengjunnan): This is an issue with OpenHarmony.
  // We have introduced an experimental switch as a temporary workaround for the abnormal layout problem in the waterfall flow scenario.
  // This switch will be removed once OpenHarmony is fixed. Please avoid using it unnecessarily.
  experimentalFixFitContent?: boolean = false;
  private perfController?: PerformanceController;
  private uiRenderer?: LynxUIRenderer;
  private templateRenderer?: LynxTemplateRenderer;
  private context: LynxContext = new LynxContext();
  private moduleManager: LynxModuleManager = new LynxModuleManager(this.context);
  private sendableModuleManager: SendableLynxModuleManager = new SendableLynxModuleManager();
  private currentWindow?: window.Window;
  private enableLogBox: boolean = false;
  private keyboardHeightChangeCallback?: (height: number) => void;
  private loadingID: number = 0;
  private fontScale: number = 1.0;
  onCreate: (context: LynxContext) => void = ((context: LynxContext) => {
  });
  onReuse: (context: LynxContext) => boolean = ((context: LynxContext) => {
    return true;
  });
  beforeLoadTemplate: (context: LynxContext) => void = ((context: LynxContext) => {
  });
  @State rootSize: Size = { width: 0, height: 0 };
  @State translateY: number = 0;
  templateLoaded: boolean = false;
  private measuredSize: Size = { width: 0, height: 0 };
  private hasAppeared: boolean = false;
  threadMode: ThreadStrategyForRendering = ThreadStrategyForRendering.ALL_ON_UI;
  backgroundRuntimeOptions: LynxBackgroundRuntimeOptions = new LynxBackgroundRuntimeOptions();
  enableAirStrictMode? = false;
  lynxImageConfig?: LynxImageConfig;

  initWithBuilder(builder: LynxViewBuilder) {
    this.genericResourceFetcher = builder.genericResourceFetcher;
    this.mediaResourceFetcher = builder.mediaResourceFetcher;
    this.templateResourceFetcher = builder.templateResourceFetcher;
    this.modules = builder.modules ?? new Map();
    this.sendableModules = builder.sendableModules ?? new Map();
    this.behaviors = builder.behaviors ?? new Map();
    this.screenSize = builder.screenSize;
    this.uiRendererCreator = builder.uiRendererCreator;
    this.backgroundRuntime = builder.backgroundRuntime;
    this.platformConfig = builder.platformConfig;
    this.clients = builder.clients ?? [];
    this.fitContentWidth = builder.fitContentWidth ?? false;
    this.fitContentHeight = builder.fitContentHeight ?? false;
    this.experimentalFixFitContent = builder.experimentalFixFitContent ?? false;
    this.beforeLoadTemplate = builder.beforeLoadTemplate ?? ((context: LynxContext) => {
    });
    this.threadMode = builder.threadMode ?? ThreadStrategyForRendering.ALL_ON_UI;
    this.backgroundRuntimeOptions = new LynxBackgroundRuntimeOptions(builder.backgroundRuntimeOptions);
    this.enableAirStrictMode = builder.enableAirStrictMode ?? false;
    this.lynxImageConfig = builder.lynxImageConfig;
    this.fontScale = builder.fontScale ?? 1.0;
    if (this.hasAppeared) {
      LynxServiceCenter.getService<ILynxTrailService>(LynxServiceType.Trail)?.parseLynxView(this);
      this.reInitTemplateRenderer(this.hasLoadSource());
    }
  }

  private hasLoadSource(): boolean {
    return !!this.url || !!this.buffer || !!this.templateBundle;
  }

  ensureTemplateRendererAndLoadTemplate() {
    if (!!this.templateRenderer && !this.templateLoaded) {
      this.loadPreparedTemplate();
    } else {
      this.reInitTemplateRenderer();
    }
  }

  private loadPreparedTemplate() {
    this.loadingID++;
    this.beforeLoadTemplate(this.context);
    this.loadTemplate();
  }

  reInitTemplateRenderer(shouldLoadTemplate: boolean = true) {
    LynxInfoReporterHelper.reportLynxTemplateEvent(LynxTemplateType.PAGE, this.url);
    if (this.perfController) {
      LLog.d(TAG, 'Destroy perfController before re-init template renderer');
      this.perfController.destroy()
      this.perfController = undefined;
    }
    if (this.uiRenderer) {
      LLog.d(TAG, 'Destroy uiRenderer before re-init template renderer');
      this.uiRenderer.aboutToDisappear();
      this.uiRenderer = undefined;
    }
    if (this.templateRenderer) {
      LLog.d(TAG, 'Destroy templateRender before re-init template renderer, url: ' + this.url);
      this.templateRenderer.aboutToDisappear();
    }
    const initStartTiming = new Date().getTime();
    if (this.backgroundRuntime) {
      this.backgroundRuntime.moduleFactory.setContext(this.context);
      this.moduleManager = this.backgroundRuntime.moduleFactory;
      this.backgroundRuntimeOptions.merge(this.backgroundRuntime.runtimeOptions);
      this.genericResourceFetcher = this.genericResourceFetcher ?? this.backgroundRuntime.genericResourceFetcher;
      this.mediaResourceFetcher = this.mediaResourceFetcher ?? this.backgroundRuntime.mediaResourceFetcher;
      this.templateResourceFetcher = this.templateResourceFetcher ?? this.backgroundRuntime.templateResourceFetcher;
    }

    this.modules.forEach((v, k) => {
      this.moduleManager.registerModule(k, v);
    });
    this.sendableModules.forEach((v, k) => {
      this.sendableModuleManager.registerModule(k, v);
    });
    const devtoolService = LynxServiceCenter.getService<ILynxDevToolService>(LynxServiceType.DevTool)
    devtoolService?.registerWebSocketModule(this.moduleManager);
    this.moduleManager.registerModule('LynxFetchModule', { moduleClass: LynxFetchModule });
    this.moduleManager.registerModule('LynxSetModule', { moduleClass: LynxSetModule });
    this.moduleManager.registerModule('LynxResourceModule', { moduleClass: LynxResourceModule });
    this.moduleManager.registerModule('LynxAccessibilityModule', { moduleClass: LynxAccessibilityModule });

    this.context.setGenericResourceFetcher(this.genericResourceFetcher);
    this.context.setMediaResourceFetcher(this.mediaResourceFetcher);

    let providers = [this.genericResourceFetcher, this.mediaResourceFetcher, this.templateResourceFetcher];

    this.templateRenderer =
      new LynxTemplateRenderer(this.clients, providers, !this.uiRendererCreator,
        this.uiRendererCreator ? ThreadStrategyForRendering.ALL_ON_UI : this.threadMode,
        this.backgroundRuntimeOptions, this, !this.enableAirStrictMode,
        this.moduleManager, this.sendableModuleManager, this.screenSize);
    if (this.platformConfig) {
      this.templateRenderer.setPlatformConfig(this.platformConfig);
    }

    this.templateLoaded = false;
    if (shouldLoadTemplate) {
      this.beforeLoadTemplate(this.context);
    }
    this.loadingID++;

    this.prepareForLoadTemplate();
    const initEndTiming = new Date().getTime();
    this.templateRenderer.setTiming(initStartTiming * 1000, TimingConstants.CREATE_LYNX_START);
    this.templateRenderer.setTiming(initEndTiming * 1000, TimingConstants.CREATE_LYNX_END);
    if (shouldLoadTemplate) {
      this.loadTemplate();
    }
  }

  onEnterForeground() {
    this.uiRenderer?.onEnterForeground();
    this.templateRenderer?.onEnterForeground();
  }

  onEnterBackground() {
    this.uiRenderer?.onEnterBackground();
    this.templateRenderer?.onEnterBackground();
  }

  prepareForLoadTemplate() {
    if (this.perfController) {
      this.perfController.destroy()
    }
    if (this.uiRenderer) {
      this.uiRenderer.aboutToDisappear();
    }

    let providers = [this.genericResourceFetcher, this.mediaResourceFetcher, this.templateResourceFetcher];
    this.perfController = new PerformanceController();
    if (this.uiRendererCreator) {
      this.uiRenderer =
        this.uiRendererCreator.createUIRenderer(this.context,
          this.rootSize, providers, this.behaviors) as LynxUIRenderer;
    } else {
      this.uiRenderer =
        new HarmonyUIRenderer(this.context, this.rootSize,
          this.behaviors, this.getUIContext(), this.perfController);
    }
    let extensionService: ILynxExtensionService | undefined =
      LynxServiceCenter.getService<ILynxExtensionService>(LynxServiceType.Extension);
    let uiDelegate = this.uiRenderer.getUIDelegate();
    if (!this.enableAirStrictMode && extensionService != undefined) {
      extensionService.onLynxViewSetup(this.context, uiDelegate, !this.uiRendererCreator);
    }
    this.perfController.setPerformanceObserver(this.clients);

    if (!this.templateRenderer) {
      return;
    }
    this.templateRenderer.reset(uiDelegate, this.perfController);

    this.uiRenderer.updateRefreshRate(LynxEnv.getDisplayInfo().refreshRate);
    this.context.setRenderer(this.templateRenderer);
    if (this.fontScale != 1.0) {
      this.context.updateFontScale(this.fontScale);
    }

    this.nodeController?.updateNode(this.uiRenderer, this.fitContentHeight && !!this.experimentalFixFitContent);
    if (!this.enableAirStrictMode) {
      this.setupExtensionModules();
    }
    this.templateRenderer.initScreenMetrics();
  }

  aboutToAppear() {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.LYNX_VIEW_CREATE);
    this.hasAppeared = true;
    LynxServiceCenter.getService<ILynxTrailService>(LynxServiceType.Trail)?.parseLynxView(this);
    this.nodeController =
      this.uiRendererCreator ? this.uiRendererCreator.createNodeController() : new CAPINodeController()
    this.setupLogBox();
    this.context.setLynxView(this);
    if (this.hasLoadSource()) {
      this.reInitTemplateRenderer(true);
    }
    if (this.onCreate) {
      this.onCreate(this.context)
    }
    (this.windowStage ? this.windowStage.getMainWindow() : window.getLastWindow(getContext(this))).then((win) => {
      this.currentWindow = win;
      this.keyboardHeightChangeCallback = (height: number) => {
        const keyBoardHeight = Math.floor(px2vp(height));
        this.sendGlobalEvent('keyboardstatuschanged', [height !== 0 ? 'on' : 'off', keyBoardHeight, keyBoardHeight]);
        this.uiRenderer?.keyboardStatusChanged(height !== 0 ? keyBoardHeight + 1 : 0)
      };
      this.currentWindow.on('keyboardHeightChange', this.keyboardHeightChangeCallback);
      this.uiRenderer?.setAvoidKeyboardListener((translateY: number) => {
        this.getUIContext()?.animateTo({
          duration: 300,
          curve: Curve.EaseOut,
        }, () => {
          this.translateY += translateY;
        })
      })
    }).catch((err: BusinessError) => {
      LLog.e(TAG, `Failed to getLastWindow. Code: ${err.code}, message: ${err.message}`);
    });

    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.LYNX_VIEW_CREATE);
  }

  aboutToDisappear() {
    LLog.d(TAG, 'aboutToDisappear.')
    this.hasAppeared = false;
    this.perfController?.destroy();
    this.perfController = undefined;
    this.uiRenderer?.aboutToDisappear();
    this.uiRenderer = undefined;
    this.templateRenderer?.aboutToDisappear();
    this.templateRenderer = undefined;
    this.nodeController?.dispose();
    this.nodeController = undefined;
    this.disposeLogBox();
    try {
      this.currentWindow?.off('keyboardHeightChange', this.keyboardHeightChangeCallback);
    } catch (e) {
      LLog.e(TAG, `Failed to remove the keyboardHeightChange listener. Code: ${e.code}, message: ${e.message}`);
    }
    this.keyboardHeightChangeCallback = undefined;
    let extensionService: ILynxExtensionService | undefined =
      LynxServiceCenter.getService<ILynxExtensionService>(LynxServiceType.Extension);
    if (!this.enableAirStrictMode && extensionService != undefined) {
      extensionService.onLynxViewDestroy(this.context);
    }
    this.context.aboutToDisappear();
  }

  private setupLogBox() {
    this.enableLogBox = LynxEnv.isLynxDebugEnabled() && LynxEnv.isLogBoxEnabled();
    if (!this.enableLogBox) {
      return;
    }
    this.logBox = LynxDevTool.createLogBox((_msg: string) => {
    }, (): Record<string, string> => {
      return this.templateRenderer?.getAllJsSource() ?? {};
    });
    if (this.logBox) {
      LynxDevTool.registerLogBox(this.logBox, this.getUIContext());
    }
  }

  private disposeLogBox() {
    if (this.logBox) {
      LynxDevTool.unregisterLogBox(this.logBox);
    }
    this.logBox = null;
    this.enableLogBox = false;
  }

  aboutToReuse(params: Record<string, ESObject>): void {
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.LYNX_VIEW_REUSE);
    LLog.d(TAG, 'aboutToReuse');
    let couldReuse = true;
    if (params.url !== undefined && this.url != params.url) {
      LLog.d(TAG, `can not reuse due to different url, old(${this.url}) new(${params.url})`);
      this.url = params.url;
      this.buffer = params.buffer;
      this.templateBundle = params.templateBundle;
      couldReuse = false;
    }
    if (params.buffer !== undefined && this.buffer != params.buffer) {
      LLog.d(TAG, 'can not reuse due to different buffer.');
      this.url = params.url;
      this.buffer = params.buffer;
      this.templateBundle = params.templateBundle;
      couldReuse = false;
    }
    if (params.templateBundle !== undefined && this.templateBundle != params.templateBundle) {
      LLog.d(TAG, 'can not reuse due to different templateBundle.');
      this.url = params.url;
      this.buffer = params.buffer;
      this.templateBundle = params.templateBundle;
      couldReuse = false;
    }
    if (params.uiRendererCreator !== undefined && this.uiRendererCreator != params.uiRendererCreator) {
      LLog.d(TAG, `can not reuse due to different renderer mode`);
      this.uiRendererCreator = params.uiRendererCreator;
      couldReuse = false;
    }
    if (params.enableAirStrictMode !== undefined && this.enableAirStrictMode !== params.enableAirStrictMode) {
      LLog.d(TAG,
        `can not reuse due to different enableAirStrictMode, old(${this.enableAirStrictMode}) new(${params.enableAirStrictMode})`);
      this.enableAirStrictMode = params.enableAirStrictMode;
      couldReuse = false;
    }
    if (params.genericResourceFetcher !== undefined && this.genericResourceFetcher != params.genericResourceFetcher) {
      this.genericResourceFetcher = params.genericResourceFetcher;
    }
    if (params.mediaResourceFetcher !== undefined && this.mediaResourceFetcher != params.mediaResourceFetcher) {
      this.mediaResourceFetcher = params.mediaResourceFetcher;
    }
    if (params.templateResourceFetcher !== undefined &&
      this.templateResourceFetcher != params.templateResourceFetcher) {
      this.templateResourceFetcher = params.templateResourceFetcher;
    }
    if (params.modules !== undefined && this.modules != params.modules) {
      this.modules = params.modules;
    }
    if (params.behaviors !== undefined && this.behaviors != params.behaviors) {
      this.behaviors = params.behaviors;
    }
    if (params.clients !== undefined && this.clients != params.clients) {
      this.clients = params.clients;
    }
    if (params.fitContentWidth !== undefined && this.fitContentWidth != params.fitContentWidth) {
      this.fitContentWidth = params.fitContentWidth;
    }
    if (params.fitContentHeight !== undefined && this.fitContentHeight != params.fitContentHeight) {
      this.fitContentHeight = params.fitContentHeight;
    }
    if (params.onCreate !== undefined && this.onCreate != params.onCreate) {
      this.onCreate = params.onCreate;
    }
    if (params.backgroundRuntimeOptions !== undefined &&
      this.backgroundRuntimeOptions != params.backgroundRuntimeOptions) {
      this.backgroundRuntimeOptions = params.backgroundRuntimeOptions;
    }
    if (params.onReuse !== undefined && this.onReuse != params.onReuse) {
      this.onReuse = params.onReuse;
    }
    if (params.metaData !== undefined && this.metaData != params.metaData) {
      this.metaData = params.metaData;
      if (couldReuse && this.templateLoaded) {
        // metaData changed, try to reload.
        LLog.d(TAG, 'reloadTemplate');
        if (this.onReuse(this.context)) {
          this.reload();
        }
      }
    }
    if (params.fontScale !== undefined && this.fontScale != params.fontScale) {
      this.fontScale = params.fontScale;
      couldReuse = false;
    }
    if (!couldReuse) {
      LLog.d(TAG, 'can not reuse, so we recreate LynxTemplateRenderer');
      this.reInitTemplateRenderer(this.hasLoadSource());
    }
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.LYNX_VIEW_REUSE);
  }

  aboutToRecycle(): void {
    LLog.d(TAG, 'aboutToRecycle');
  }

  sendGlobalEvent(name: string, params: Object[]) {
    if (!this.templateRenderer) {
      return;
    }
    if (this.enableAirStrictMode || this.templateRenderer.shouldSendEventToMainThread()) {
      this.templateRenderer.triggerEventBus(name, params);
    } else {
      this.templateRenderer.callJSFunction('GlobalEventEmitter', 'emit', [name, params]);
    }
  }

  private reportLynxError(error: LynxError) {
    this.clients.forEach(client => client.onReceivedError(error));
    this.logBox?.reportError(error);
  }

  private updateViewport(width: number, height: number) {
    LLog.d(TAG, `update view port width: ${width}, height: ${height}`);
    this.templateRenderer?.updateViewport(width,
      this.fitContentWidth ? MeasureMode.INDEFINITE : MeasureMode.DEFINITE,
      height, this.fitContentHeight ? MeasureMode.INDEFINITE : MeasureMode.DEFINITE);
    this.uiRenderer?.updateViewport(width, height);
  }

  override onMeasureSize(selfLayoutInfo: GeometryInfo, children: Measurable[], constraint: ConstraintSizeOptions) {
    if (!this.templateRenderer) {
      return selfLayoutInfo;
    }

    let childResult = children[0]?.measure(constraint);
    let result = selfLayoutInfo;

    if (childResult && result.height !== childResult.height && childResult.height !== 0) {
      result.height = childResult.height;
    }
    if (childResult && result.width !== childResult.width && childResult.width !== 0) {
      result.width = childResult.width;
    }
    // trigger native layout here, and rootSize will updated.
    this.updateViewport(result.width, result.height);
    if (this.fitContentWidth) {
      result.width = this.rootSize.width;
    }
    if (this.fitContentHeight) {
      result.height = this.rootSize.height;
    }
    return result;
  }

  override onPlaceChildren(selfLayoutInfo: GeometryInfo, children: Layoutable[],
    constraint: ConstraintSizeOptions): void {
    if (children.length > 0) {
      children[0].layout({ x: 0, y: 0 })
    }
  }

  reload() {
    if (!this.templateRenderer) {
      LLog.e(TAG, 'reload failed, templateRenderer does not exist.');
      return;
    }
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.LYNX_VIEW_RELOAD);
    let timingOption: TimingOption = new TimingOption(TimingConstants.RELOAD_BUNDLE_FROM_NATIVE);
    let nowStartTimestamp = Date.now();
    timingOption.setTiming(TimingConstants.PIPELINE_START, nowStartTimestamp);
    timingOption.setTiming(TimingConstants.RELOAD_BUNDLE_START, nowStartTimestamp);
    this.onPageStarted(LynxPipelineOrigin.LynxReload);
    timingOption.setTiming(TimingConstants.FFI_START, Date.now());
    this.templateRenderer.reloadTemplate(this.metaData.templateData?.processor, this.metaData.templateData?.data,
      this.metaData.templateData?.readOnly,
      this.metaData.globalProps?.data, timingOption.toRecord());
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.LYNX_VIEW_RELOAD);
  }

  private onPageStarted(pipelineOrigin: LynxPipelineOrigin) {
    if (!this.templateRenderer) {
      return;
    }
    const instanceId = this.templateRenderer.getInstanceId();
    LynxEventReporter.updateGenericInfo(LynxEventReporter.PROP_NAME_URL, this.url, instanceId);
    LynxEventReporter.updateGenericInfo(LynxEventReporter.PROP_NAME_RELATIVE_PATH, this.url, instanceId);
    let pipelineInfo = new LynxPipelineInfo(this.url);
    pipelineInfo.addPipelineOrigin(pipelineOrigin);
    this.clients.forEach(client => client.onPageStarted(this.context, pipelineInfo));
    LynxEventReporter.onEvent(LynxEventReporter.LYNX_SDK_OPEN_PAGE_EVENT, instanceId);
  }

  loadTemplate() {
    if (!this.templateRenderer) {
      LLog.e(TAG, 'loadTemplate skipped, templateRenderer does not exist.');
      return;
    }
    let timingOption: TimingOption = new TimingOption(TimingConstants.LOAD_BUNDLE);
    let loadBundleStartTimestamp = Date.now();
    timingOption.setTiming(TimingConstants.PIPELINE_START, loadBundleStartTimestamp);
    timingOption.setTiming(TimingConstants.LOAD_BUNDLE_START, loadBundleStartTimestamp);
    if (this.templateBundle) {
      LLog.i(TAG, `start loadTemplate(capi=${!this.uiRendererCreator}) with bundle`);
      this.loadTemplateInternal(timingOption, this.templateBundle, undefined);
      return;
    }

    if (this.buffer) {
      LLog.i(TAG, `start loadTemplate(capi=${!this.uiRendererCreator}}) with buffer`);
      this.verifyAndLoad(timingOption, this.buffer);
      return;
    }

    let templateProvider = this.templateResourceFetcher;
    if (!templateProvider) {
      LLog.e(TAG, 'loadTemplate without template provider: ' + this.url);
      let error = LynxError.createLynxError(
        LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE,
        "LoadTemplate without template provider");
      error.setTemplateUrl(this.url);
      this.reportLynxError(error);
      return;
    }

    let requestUrl = this.url;
    let req = new LynxResourceRequest(requestUrl, LynxResourceType.LYNX_RESOURCE_TYPE_TEMPLATE);
    LLog.i(TAG, `start loadTemplate(capi=${!this.uiRendererCreator}}): ${requestUrl}`);
    let blockLoadingID = this.loadingID;
    templateProvider.fetchTemplate(req, (err: BusinessError, res: TemplateProviderResult) => {
      // If the url has been changed, ignore this response.
      // It will happen in case of reused LynxView.
      if (blockLoadingID !== this.loadingID) {
        LLog.e(TAG,
          'template fetcher callback will not be executed because the loading ID is not consistent with LynxView instance.')
        return;
      }
      if (requestUrl != this.url) {
        LLog.w(TAG, 'url dismissed, maybe changed, ignore this response.');
        return;
      }
      if (err) {
        const errorString = 'loadTemplate error: ' + JSON.stringify(err);
        LLog.e(TAG, errorString);
        let lynxError = LynxError.createLynxError(
          LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE,
          errorString);
        lynxError.setTemplateUrl(requestUrl);
        this.reportLynxError(lynxError);
        return;
      }
      if (res.binary == undefined) {
        const errorMsg = 'loadTemplate error: empty binary.';
        LLog.e(TAG, errorMsg);
        let lynxError = LynxError.createLynxError(
          LynxSubErrorCode.E_APP_BUNDLE_LOAD_BAD_RESPONSE,
          errorMsg);
        lynxError.setTemplateUrl(this.url);
        this.reportLynxError(lynxError);
        return;
      }
      this.verifyAndLoad(timingOption, res.binary);
    });
  }

  private verifyAndLoad(timingOption: TimingOption, buffer: ArrayBuffer) {
    timingOption.setTiming(TimingConstants.VERIFY_TASM_START, Date.now())
    let securityService = LynxServiceCenter.getService<ILynxSecurityService>(LynxServiceType.Security);
    let result: SecurityResult = { verified: true };
    if (securityService != undefined) {
      result = securityService.verifyTASM(buffer, this.url, LynxTasmType.TYPE_TEMPLATE, this.context);
    }
    timingOption.setTiming(TimingConstants.VERIFY_TASM_END, Date.now())

    if (result.verified) {
      this.loadTemplateInternal(timingOption, undefined, buffer);
    } else {
      LLog.e(TAG, `verify template failed, errMsg: ${result?.errMsg}`);
      let error = LynxError.createLynxError(
        LynxSubErrorCode.E_APP_BUNDLE_VERIFY_INVALID_SIGNATURE,
        "Signature check failed for template binary");
      error.setTemplateUrl(this.url);
      this.reportLynxError(error);
    }
  }

  private loadTemplateInternal(timingOption: TimingOption, bundle?: TemplateBundle, buffer?: ArrayBuffer) {
    if (!this.templateRenderer) {
      return;
    }
    TraceEvent.beginSection(TraceCategory.Lynx, TraceEventDef.LYNX_VIEW_LOAD);
    this.onPageStarted(LynxPipelineOrigin.LynxFirstScreen);
    if (this.metaData.globalProps !== undefined) {
      this.templateRenderer.updateGlobalProps(this.metaData.globalProps?.data);
    }

    if (bundle) {
      timingOption.setTiming(TimingConstants.FFI_START, Date.now())
      this.templateRenderer.loadTemplateBundle(this.url, bundle,
        this.metaData.templateData?.processor,
        this.metaData.templateData?.data,
        this.metaData.templateData?.readOnly, this.loadOption?.isEnableDumpElement(),
        timingOption.toRecord())
    } else if (buffer) {
      timingOption.setTiming(TimingConstants.FFI_START, Date.now())
      this.templateRenderer.loadTemplate(this.url, buffer,
        this.metaData.templateData?.processor,
        this.metaData.templateData?.data,
        this.metaData.templateData?.readOnly, this.loadOption?.isEnableRecycleTemplateBundle(),
        timingOption.toRecord());
    }
    TraceEvent.endSection(TraceCategory.Lynx, TraceEventDef.LYNX_VIEW_LOAD);
    this.templateLoaded = true;

    if (this.measuredSize.width > 0) {
      // The onMeasureSize may be ignored in reuse case if fitContentHeight and rootSize not changed,
      // so we try to call updateViewport here.

      this.updateViewport(this.measuredSize.width, this.measuredSize.height);
    }
  }

  build() {
    Stack() {
      NodeContainer(this.nodeController)
    }
    .width(this.fitContentWidth ? this.rootSize.width : '100%')
    .height(this.fitContentHeight
      ? (!this.uiRendererCreator && this.experimentalFixFitContent ? 'auto' : this.rootSize.height)
      : '100%')
    // Add 'key' property to locate LynxView during UI automation.
    .key('lynxview')
    .translate({ y: this.translateY })
    .onAreaChange((oldValue: Area, newValue: Area) => {
      const width = newValue.width as number;
      const height = newValue.height as number;
      if (this.measuredSize.width === width && this.measuredSize.height === height) {
        // onAreaChange can be triggered by both size and position changes.
        // If the measured size hasn't changed, skip the update to avoid unnecessary layout calculations.
        return;
      }
      this.measuredSize.width = width;
      this.measuredSize.height = height;
      this.uiRenderer?.requestLayout();
    })
  }

  setEnableBytecode(enable: boolean, sourceUrl: string) {
    if (!this.templateRenderer) {
      return;
    }
    this.templateRenderer.setEnableBytecode(enable, sourceUrl);
  }

  setupExtensionModules() {
    let module = this.context.getExtensionModule();
    if (module == undefined || !this.templateRenderer) {
      return;
    }
    this.templateRenderer.setupExtensionDelegate(module.getExtensionDelegatePtr());
    module.setup();
    module.onTemplateLoad(this.url); // no other timing to set, may various with other platform
  }
}
