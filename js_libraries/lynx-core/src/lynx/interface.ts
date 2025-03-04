// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Lynx is the interface that native lynx implements implementations are in lynx.cc
 */
import { TextInfo, TextMetrics } from '../modules/nativeModules';
import { NativeElementProxy } from '../modules';
import { GlobalProps, Lynx as BackgroundLynx, Response } from '@lynx-js/types';

export interface NativeLynxProxy extends BackgroundLynx {
  __globalProps: GlobalProps;
  __presetData: Record<string, unknown>;

  getComponentContext(
    id: string,
    key: string,
    callback: (data: unknown) => void
  ): void;
  createElement(rootId: string, id: string): NativeElementProxy;
  fetchDynamicComponent(
    url: string,
    options: Record<string, any>,
    callback: (res: { code: number }) => void,
    id?: string[]
  ): void;
  QueryComponent(source: string, callback: (result: any) => void): void;
  addFont(
    font: { src: string; 'font-family': string },
    callback: (err?: Error) => void
  ): void;
  getTextInfo(text: string, info: TextInfo): TextMetrics;
  getDevtool(): ContextProxy;
  getCoreContext(): ContextProxy;
  getJSContext(): ContextProxy;
  getUIContext(): ContextProxy;

  getCustomSectionSync<T = any>(key: string): T;
  reload(value: object, callback: () => void): void;

  fetch: (input: RequestInfo, init?: RequestInit) => Promise<Response>;
}

export enum DispatchEventResult {
  // Event was not canceled by event handler or default event handler.
  NotCanceled = 0,
  // Event was canceled by event handler; i.e. a script handler calling
  // preventDefault.
  CanceledByEventHandler,
  // Event was canceled by the default event handler; i.e. executing the default
  // action.  This result should be used sparingly as it deviates from the DOM
  // Event Dispatch model. Default event handlers really shouldn't be invoked
  // inside of dispatch.
  CanceledByDefaultEventHandler,
  // Event was canceled but suppressed before dispatched to event handler.  This
  // result should be used sparingly; and its usage likely indicates there is
  // potential for a bug. Trusted events may return this code; but untrusted
  // events likely should always execute the event handler the developer intends
  // to execute.
  CanceledBeforeDispatch,
}

export const enum ContextProxyType {
  CoreContext,
  DevTool,
  JSContext,
  UIContext,
}

export const enum MessageEventType {
  ON_NATIVE_APP_READY = '__OnNativeAppReady',
  NOTIFY_GLOBAL_PROPS_UPDATED = '__NotifyGlobalPropsUpdated',
  ON_LIFECYCLE_EVENT = '__OnLifecycleEvent',
  ON_REACT_COMPONENT_RENDER = '__OnReactComponentRender',
  ON_REACT_CARD_DID_UPDATE = '__OnReactCardDidUpdate',
  ON_REACT_COMPONENT_CREATED = '__OnReactComponentCreated',
  ON_REACT_CARD_RENDER = '__OnReactCardRender',
  ON_REACT_COMPONENT_UNMOUNT = '__OnReactComponentUnmount',
  ON_APP_FIRST_SCREEN = '__OnAppFirstScreen',
  ON_DYNAMIC_JS_SOURCE_PREPARED = '__OnDynamicJSSourcePrepared',
  ON_APP_ENTER_FOREGROUND = '__OnAppEnterForeground',
  ON_APP_ENTER_BACKGROUND = '__OnAppEnterBackground',
  ON_COMPONENT_ACTIVITY = '__OnComponentActivity',
  FORCE_GC_JSI_OBJECT_WRAPPER = '__ForceGcJSIObjectWrapper',
  ON_REACT_COMPONENT_DID_UPDATE = '__OnReactComponentDidUpdate',
  ON_REACT_COMPONENT_DID_CATCH = '__OnReactComponentDidCatch',
  ON_COMPONENT_SELECTOR_CHANGED = '__OnComponentSelectorChanged',
  ON_COMPONENT_PROPERTIES_CHANGED = '__OnComponentPropertiesChanged',
  ON_COMPONENT_DATA_SET_CHANGED = '__OnComponentDataSetChanged',

  // storage
  EVENT_SET_SESSION_STORAGE = '__SetSessionStorageItem',
  EVENT_GET_SESSION_STORAGE = '__GetSessionStorageItem',
  EVENT_SUBSCRIBE_SESSION_STORAGE = '__SubscribeSessionStorage',
  EVENT_UNSUBSCRIBE_SESSION_STORAGE = '__UnSubscribeSessionStorage',
}

export interface MessageEvent {
  type: string;
  data: any;
  origin?: string;
}

export interface ContextProxy {
  onTriggerEvent?: (event: MessageEvent) => void;

  postMessage(message: any): void;
  dispatchEvent(event: MessageEvent): DispatchEventResult;
  addEventListener(type: string, listener: (event: MessageEvent) => void): void;
  removeEventListener(
    type: string,
    listener: (event: MessageEvent) => void
  ): void;
}

export interface RequireModuleCache {
  cache: Record<string, unknown>;
}

export interface RequireModule extends RequireModuleCache {
  <T = unknown>(path: string, entryName?: string): T;
}

export interface RequireModuleAsync extends RequireModuleCache {
  <T>(path: string, callback?: (error?: Error, ret?: T) => void): void;
}
