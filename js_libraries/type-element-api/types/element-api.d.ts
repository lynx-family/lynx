// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface ElementRef {}

export interface ComponentElementRef extends ElementRef {}

export interface PageElementRef extends ComponentElementRef {}

export interface ListElementRef extends ElementRef {}

export interface ViewElementRef extends ElementRef {}

export interface TextElementRef extends ElementRef {}

export interface RawTextElementRef extends ElementRef {}

export interface ImageElementRef extends ElementRef {}

export interface ScrollElementRef extends ElementRef {}

export interface WrapperElementRef extends ElementRef {}

export interface NoneElementRef extends ElementRef {}

export interface IfElementRef extends ElementRef {}

export interface ForElementRef extends ElementRef {}

export interface BlockElementRef extends ElementRef {}

export type ElementInfo = Record<string, any>;

export interface SelectorParams {
  onlyCurrentComponent?: boolean;
}

export interface DynamicComponentResult {
  code: number;
  data: {
    evalResult: (
      url: string
    ) => {
      name: string;
    };
  };
}

declare global {
  function __CreatePage(componentId: string, cssId: number, info?: ElementInfo): PageElementRef;

  function __CreateComponent(
    parentComponentUniId: number,
    componentId: string,
    cssId: number,
    entryName: string,
    name: string,
    path: string,
    config?: Record<string, unknown>
  ): ComponentElementRef;

  function __CreateView(parentComponentUniId: number, info?: ElementInfo): ViewElementRef;

  function __CreateScrollView(parentComponentUniId: number, info?: ElementInfo): ScrollElementRef;

  function __CreateText(parentComponentUniId: number, info?: ElementInfo): TextElementRef;

  function __CreateRawText(text: string, info?: ElementInfo): RawTextElementRef;

  function __CreateImage(parentComponentUniId: number, info?: ElementInfo): ImageElementRef;

  function __CreateWrapperElement(parentComponentUniId: number): WrapperElementRef;

  function __CreateNonElement(parentComponentUniId: number): ElementRef;

  function __CreateIf(parentComponentUniId: number, info?: ElementInfo): IfElementRef;

  function __CreateFor(parentComponentUniId: number, info?: ElementInfo): ForElementRef;

  function __CreateBlock(parentComponentUniId: number, info?: ElementInfo): BlockElementRef;

  function __CreateList(
    parentComponentUniId: number,
    componentAtIndex: (listRef: ListElementRef, listId: number, cellIndex: number, opId: number) => number | undefined,
    enqueueComponent: (listRef: ListElementRef, listId: number, eleId: number) => void,
    info?: ElementInfo
  ): ListElementRef;

  function __UpdateListCallbacks(
    node: ListElementRef,
    componentAtIndex: (listRef: ListElementRef, listId: number, cellIndex: number, opId: number) => number | undefined,
    enqueueComponent: (listRef: ListElementRef, listId: number, eleId: number) => void
  ): void;

  function __CreateElement(tag: string, comParentUniID: number, info?: ElementInfo): ElementRef;

  function __AppendElement(parent: ElementRef, current: ElementRef): ElementRef;

  function __RemoveElement(parent: ElementRef, current: ElementRef): ElementRef;

  function __InsertElementBefore(parent: ElementRef, current: ElementRef, marker: ElementRef): ElementRef;

  function __SwapElement(a: ElementRef, b: ElementRef): void;

  function __ReplaceElement(a: ElementRef, b: ElementRef): void;

  function __ReplaceElements(parent: ElementRef, a: ElementRef | ElementRef[] | undefined, b: ElementRef | ElementRef[] | undefined): void;

  function __GetParent(current: ElementRef): ElementRef;

  function __GetChildren(current: ElementRef): ElementRef[];

  function __FirstElement(current: ElementRef): ElementRef;

  function __LastElement(current: ElementRef): ElementRef;

  function __NextElement(node: ElementRef): ElementRef;

  function __GetTag(node: ElementRef): string;

  function __SetAttribute(current: ElementRef, attrName: string, value: any): void;

  function __AddClass(current: ElementRef, className: string): void;

  function __SetClasses(current: ElementRef, className: string | undefined): void;

  function __GetClasses(current: ElementRef): string[];

  function __SetStaticStyle(node: ElementRef, key: number, value: unknown): void;

  function __SetInlineStyles(node: ElementRef, value: unknown): void;

  function __GetInlineStyle(node: ElementRef, propertyId: number): string;

  function __GetInlineStyles(node: ElementRef): string;

  function __SetID(node: ElementRef, id: string | undefined): void;

  function __GetID(node: ElementRef): string;

  function __SetCSSId(node: ElementRef, cssId: number, entryName?: string): void;

  function __AddEvent(node: ElementRef, type: string, name: string, func: string | Object): void;

  function __SetEvents(node: ElementRef, events: Record<string, unknown>[] | undefined): void;

  function __GetEvent(node: ElementRef, name: string, type: string): Record<string, any>;

  function __GetEvents(node: ElementRef): Record<string, Record<string, any>>;

  function __AddDataset(node: ElementRef, key: string, value: unknown): void;

  function __SetDataset(node: ElementRef, value: Record<string, unknown> | undefined): void;

  function __GetDataset(node: ElementRef): Record<string, unknown>;

  function __GetDataByKey(node: ElementRef, key: string): any;

  function __GetElementUniqueID(node: ElementRef): number;

  function __ElementIsEqual(left: ElementRef, right: ElementRef): boolean;

  function __UpdateComponentID(node: ElementRef, id: string): void;

  function __GetComponentID(node: ElementRef): string;

  function __UpdateComponentInfo(
    node: ElementRef,
    params: {
      componentID?: string;
      name?: string;
      path?: string;
      entry?: string;
      cssID?: number;
    }
  ): void;

  function __FlushElementTree(
    element?: ElementRef,
    options?: {
      triggerLayout?: boolean;
      triggerDataUpdated?: boolean;
      operationID?: number;
      nativeUpdateDataOrder?: number;
      __lynx_timing_flag?: string;
      elementID?: number;
      reloadTemplate?: boolean;
      listID?: number;
      pipelineOptions?: Record<string, any>;
    }
  ): void;

  function __AsyncResolveElement(element: ElementRef): void;

  function __OnLifecycleEvent(args: any[]): void;

  function _ReportError(
    err: Error,
    info: {
      errorCode: number;
    }
  ): void;

  function __ElementFromBinary(elementTemplateKey: string, parentComponentUniId: number): ElementRef[];

  function __GetTemplateParts(ele: ElementRef): Record<string, ElementRef>;

  function __CloneElement(ele: ElementRef, options: Record<string, any>): ElementRef;

  function __IsTemplateElement(ele: ElementRef): boolean;

  function __MarkTemplateElement(ele: ElementRef): void;

  function __MarkPartElement(ele: ElementRef, key: string): void;

  function __QuerySelector(root: ElementRef, cssSelector: string, params: SelectorParams): ElementRef;

  function __QuerySelectorAll(root: ElementRef, cssSelector: string, params: SelectorParams): ElementRef[];

  function __AddConfig(ele: ElementRef, key: string, value: any): void;

  function __SetConfig(ele: ElementRef, config: Record<string, any>): void;

  function __GetElementConfig(ele: ElementRef): Record<string, unknown>;

  function __QueryComponent(source: string, callback: (evalResult: DynamicComponentResult) => void): { evalResult: unknown };

  function __AddInlineStyle(e: ElementRef, key: number | string, value: unknown): void;

  function __GetAttributeByName(e: ElementRef, name: string): any;

  function __GetAttributeNames(e: ElementRef): string[];

  function __GetAttributes(e: ElementRef): Record<string, any>;

  function __GetPageElement(): ElementRef;

  function __InvokeUIMethod(e: ElementRef, method: string, params: Record<string, unknown>, callback: (res: { code: number; data: unknown }) => void): ElementRef[];

  function __LoadLepusChunk(name: string, cfg: { chunkType: number }): void;

  function __CreateGestureDetector(
    node: ElementRef,
    gestureID: number,
    gestureType: number,
    callbackArray: [name: string, script: string, func: Function],
    relationMap: Record<string, number[]>
  ): void;

  function __SetGestureDetector(
    node: ElementRef,
    gestureID: number,
    gestureType: number,
    callbackArray: [name: string, script: string, func: Function],
    relationMap: Record<string, number[]>
  ): void;

  function __RemoveGestureDetector(node: ElementRef, gestureID: number): void;

  function __SetGestureState(node: ElementRef, gestureID: number, state: number): void;

  function __ConsumeGesture(node: ElementRef, gestureID: number, options: Record<string, boolean>): void;

  function __GeneratePipelineOptions(): Record<string, any>;

  function __OnPipelineStart(pipeLineId: string, pipeLineOrigin: string): void;

  function __BindPipelineIDWithTimingFlag(pipeLineId: string, timingFlag: string): void;

  function __MarkTiming(pipeLineId: string, timingFlag: string): void;

  function __AddTimingListener(): void;

  function __SetLepusInitData(initData: Object): void;

  function __GetElementByUniqueID(elementId: number): ElementRef | undefined;

  function __UpdateIfNodeIndex(node: IfElementRef, ifIndex: number): void;

  function __UpdateForChildCount(node: ForElementRef, childCount: number): void;
}
