import { assertType, describe, expectTypeOf, it } from 'vitest';
import {
  GetElementByIdFunc,
  AnimationElement,
  GlobalProps,
  SelectorQuery,
  BeforePublishEvent,
  Performance,
  ResourcePrefetchData,
  ResourcePrefetchResult,
  RequestInit,
  Response,
  CreateIntersectionObserverFunc,
  LoadDynamicComponentFunc,
  TextMetrics,
  TextInfo,
  CommonPerformance,
  NativeModules as INativeModules,
  SystemInfo,
  PlatformType,
  ContextProxy,
  MessageEvent,
  DispatchEventResult,
} from '../../types/index';

describe('Global Variable Type Test ', () => {
  it('lynx Properties Types Test', () => {
    assertType<GlobalProps>(lynx.__globalProps);
    assertType<Record<string, unknown>>(lynx.__presetData);
    assertType<GetElementByIdFunc>(lynx.getElementById);
    assertType<CreateIntersectionObserverFunc>(lynx.createIntersectionObserver);
    assertType<LoadDynamicComponentFunc>(lynx.loadDynamicComponent);
    assertType<string | undefined>(lynx.targetSdkVersion);
    assertType<BeforePublishEvent>(lynx.beforePublishEvent);
    assertType<Performance | CommonPerformance>(lynx.performance);
  });

  it('lynx Method Types Test', () => {
    //Prefetch
    expectTypeOf(lynx.requestResourcePrefetch).parameter(0).toEqualTypeOf<ResourcePrefetchData>();
    expectTypeOf(lynx.requestResourcePrefetch).parameter(1).toEqualTypeOf<(res: ResourcePrefetchResult) => void>();
    expectTypeOf(lynx.requestResourcePrefetch).returns.toBeVoid();
    expectTypeOf(lynx.cancelResourcePrefetch).toEqualTypeOf<(data: ResourcePrefetchData, callback: (res: ResourcePrefetchResult) => void) => void>();

    assertType<ResourcePrefetchData>({
      data: [
        {
          uri: 'http://example.com/font.ttf',
          type: 'font',
        },
      ],
    });
    //SelectorQuery
    expectTypeOf(lynx.createSelectorQuery).toEqualTypeOf<() => SelectorQuery>();
    //JS Module Method
    expectTypeOf(lynx.registerModule).parameter(0).toEqualTypeOf<string>();
    expectTypeOf(lynx.registerModule).parameter(1).toBeObject();
    expectTypeOf(lynx.registerModule).returns.toBeVoid();
    expectTypeOf(lynx.getJSModule).parameter(0).toEqualTypeOf<string>();
    expectTypeOf(lynx.getJSModule).returns.toBeUnknown();
    //Require Module
    expectTypeOf(lynx.requireModuleAsync).parameter(0).toEqualTypeOf<string>();
    expectTypeOf(lynx.requireModuleAsync).parameter(1).toEqualTypeOf<(error: Error | null, exports?: unknown) => void>();
    expectTypeOf(lynx.requireModuleAsync).returns.toBeVoid();
    expectTypeOf(lynx.requireModule).parameter(0).toEqualTypeOf<string>();
    expectTypeOf(lynx.requireModule).parameter(1).toEqualTypeOf<string | undefined>();
    expectTypeOf(lynx.requireModule).parameter(2).toEqualTypeOf<{ timeout: number } | undefined>();
    expectTypeOf(lynx.requireModule).returns.toBeUnknown();
    //SharedData
    expectTypeOf(lynx.setSharedData).parameter(0).toEqualTypeOf<string>();
    expectTypeOf(lynx.setSharedData).parameter(1).toBeUnknown();
    expectTypeOf(lynx.setSharedData).returns.toBeVoid();
    expectTypeOf(lynx.getSharedData).parameter(0).toEqualTypeOf<string>();
    expectTypeOf(lynx.getSharedData).returns.toBeUnknown();
    expectTypeOf(lynx.registerSharedDataObserver).parameter(0).toBeFunction();
    expectTypeOf(lynx.registerSharedDataObserver).returns.toBeVoid();
    expectTypeOf(lynx.registerSharedDataObserver).parameter(0).toBeFunction();
    expectTypeOf(lynx.registerSharedDataObserver).returns.toBeVoid();
    //Fetch
    expectTypeOf(lynx.fetch).parameter(0).toEqualTypeOf<RequestInfo>();
    expectTypeOf(lynx.fetch).parameter(1).toMatchTypeOf<RequestInit | undefined>();
    expectTypeOf(lynx.fetch).returns.toEqualTypeOf<Promise<Response>>();
    //Exposure
    expectTypeOf(lynx.resumeExposure).returns.toBeVoid();
    expectTypeOf(lynx.stopExposure).parameter(0).toEqualTypeOf<{ sendEvent: boolean } | undefined>();
    expectTypeOf(lynx.stopExposure).returns.toBeVoid();
    //ObserverFrameRate
    expectTypeOf(lynx.setObserverFrameRate).parameter(0).toEqualTypeOf<{ forPageRect?: number; forExposureCheck?: number } | undefined>();
    expectTypeOf(lynx.setObserverFrameRate).returns.toBeVoid();
    //QueueMicrotask
    expectTypeOf(lynx.queueMicrotask).parameter(0).toEqualTypeOf<() => void>();
    expectTypeOf(lynx.queueMicrotask).returns.toBeVoid();
    //Reload
    expectTypeOf(lynx.reload).parameter(0).toEqualTypeOf<object>();
    expectTypeOf(lynx.reload).parameter(1).toEqualTypeOf<() => void>();
    expectTypeOf(lynx.reload).returns.toBeVoid();
    //OnError
    expectTypeOf(lynx.onError).toEqualTypeOf<((error: Error) => void) | undefined>();
    //GetTextInfo
    expectTypeOf(lynx.getTextInfo).parameter(0).toBeString();
    expectTypeOf(lynx.getTextInfo).parameter(1).toEqualTypeOf<TextInfo>();
    expectTypeOf(lynx.getTextInfo).returns.toEqualTypeOf<TextMetrics>();
    //ReportError
    expectTypeOf(lynx.reportError).parameter(0).toEqualTypeOf<string | Error>();
    expectTypeOf(lynx.reportError).parameter(1).toEqualTypeOf<{ level?: 'error' | 'warning' } | undefined>();
    expectTypeOf(lynx.reportError).returns.toBeVoid();
    //Runtime
    expectTypeOf(lynx.getCoreContext).returns.toEqualTypeOf<ContextProxy>();
    expectTypeOf(lynx.getJSContext).returns.toEqualTypeOf<ContextProxy>();
    expectTypeOf(lynx.getDevtool).returns.toEqualTypeOf<ContextProxy>();
    expectTypeOf(lynx.getNative).returns.toEqualTypeOf<ContextProxy>();
    expectTypeOf(lynx.getEngine).returns.toEqualTypeOf<ContextProxy>();
  });

  it('ContextProxy Method Types Check', () => {
    const contextProxy = {} as ContextProxy;
    expectTypeOf(contextProxy.onTriggerEvent).parameter(0).toEqualTypeOf<MessageEvent>();
    expectTypeOf(contextProxy.onTriggerEvent).returns.toBeVoid();
    expectTypeOf(contextProxy.postMessage).parameter(0).toEqualTypeOf<any>();
    expectTypeOf(contextProxy.postMessage).returns.toBeVoid();
    expectTypeOf(contextProxy.dispatchEvent).parameter(0).toEqualTypeOf<MessageEvent>();
    expectTypeOf(contextProxy.dispatchEvent).returns.toEqualTypeOf<DispatchEventResult>();
    expectTypeOf(contextProxy.addEventListener).parameter(0).toEqualTypeOf<string>();
    expectTypeOf(contextProxy.addEventListener).parameter(1).toEqualTypeOf<(event: MessageEvent) => void>();
    expectTypeOf(contextProxy.addEventListener).returns.toBeVoid();
    expectTypeOf(contextProxy.removeEventListener).parameter(0).toEqualTypeOf<string>();
    expectTypeOf(contextProxy.removeEventListener).parameter(1).toEqualTypeOf<(event: MessageEvent) => void>();
    expectTypeOf(contextProxy.removeEventListener).returns.toBeVoid();
  });

  it('GetElementById Types Check', () => {
    const func: GetElementByIdFunc = (id: string): AnimationElement => {
      return {} as AnimationElement;
    };
    expectTypeOf(func).parameter(0).toBeString();
    expectTypeOf(func).returns.toEqualTypeOf<AnimationElement>();

    assertType<GetElementByIdFunc>(getElementById);
    assertType<AnimationElement>(getElementById('id'));
  });

  it('NativeModules Type Check', () => {
    assertType<INativeModules>(NativeModules);

    expectTypeOf(NativeModules.NetworkingModule).toEqualTypeOf<any | undefined>();
    expectTypeOf(NativeModules.LynxTestModule).toEqualTypeOf<any | undefined>();
    expectTypeOf(NativeModules['bridge']['call']).toEqualTypeOf<(name: string, params: Record<string, unknown>, cb: (...args: unknown[]) => void) => void>();
    expectTypeOf(NativeModules['bridge']['on']).toEqualTypeOf<(name: string, cb: (...args: unknown[]) => void) => void>();
  });

  it('AnimationFrame Type Check', () => {
    //requestAnimationFrame
    const callbackWithTimestamp: (timeStamp?: number) => void = (timeStamp) => {
      if (timeStamp) {
        assertType<number>(timeStamp);
      }
    };
    requestAnimationFrame(callbackWithTimestamp);
    const callbackWithoutTimestamp: () => void = () => {};
    requestAnimationFrame(callbackWithoutTimestamp);
    requestAnimationFrame(undefined);
    expectTypeOf(requestAnimationFrame).returns.toBeNumber();
    //cancelAnimationFrame
    expectTypeOf(cancelAnimationFrame).returns.toBeVoid();
    expectTypeOf(cancelAnimationFrame).parameter(0).toEqualTypeOf<number | undefined>();
  });

  it('SystemInfo Type Check', () => {
    assertType<SystemInfo>(SystemInfo);
    assertType<PlatformType>(SystemInfo.platform);
    assertType<string>(SystemInfo.osVersion);
    assertType<string>(SystemInfo.engineVersion);
    assertType<object | undefined>(SystemInfo.theme);
    assertType<string>(SystemInfo.lynxSdkVersion);
    assertType<number>(SystemInfo.pixelHeight);
    assertType<number>(SystemInfo.pixelWidth);
    assertType<number>(SystemInfo.pixelRatio);
    assertType<'v8' | 'jsc' | 'quickjs'>(SystemInfo.runtimeType);
  });
});
