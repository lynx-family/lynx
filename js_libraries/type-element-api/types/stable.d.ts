// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Stable Element APIs for framework integrations.
 *
 * This tier contains the mature element creation, tree, attribute, style,
 * data, selector, component, and gesture primitives.
 */

import type {
  BlockElementRef,
  ComposeElementRef,
  ComponentElementRef,
  ElementInfo,
  ElementRef,
  ForElementRef,
  IfElementRef,
  ImageElementRef,
  ListElementRef,
  PageElementRef,
  RawTextElementRef,
  ScrollElementRef,
  TextElementRef,
  ViewElementRef,
  WrapperElementRef,
} from './shared';

export type {
  BlockElementRef,
  ComponentElementRef,
  ElementInfo,
  ElementRef,
  ForElementRef,
  IfElementRef,
  ImageElementRef,
  ListElementRef,
  NoneElementRef,
  PageElementRef,
  RawTextElementRef,
  ScrollElementRef,
  TextElementRef,
  ViewElementRef,
  WrapperElementRef,
} from './shared';

export interface SelectorParams {
  onlyCurrentComponent?: boolean;
}

export type ComponentAtIndexCallback = (
  listRef: ElementRef,
  listElementId: number,
  cellIndex: number,
  opId: number,
  enableReuseNotification?: boolean,
  enableBatchRender?: boolean,
  asyncFlush?: boolean
) => number | undefined | Promise<number>;

export type ComponentAtIndexesCallback = (
  listRef: ElementRef,
  listElementId: number,
  cellIndexes: number[],
  opIds: number[],
  enableReuseNotification: boolean,
  asyncFlush: boolean
) => void;

export type EnqueueComponentCallback = (listRef: ElementRef, listId: number, eleId: number) => void;

export interface GestureConfig {
  callbacks: {
    name: string;
    callback: unknown;
  }[];
  config?: Record<string, unknown>;
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
    config?: Record<string, unknown>,
    info?: ElementInfo
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
    componentAtIndex: ComponentAtIndexCallback,
    enqueueComponent: EnqueueComponentCallback,
    info?: ElementInfo,
    componentAtIndexes?: ComponentAtIndexesCallback
  ): ListElementRef;

  function __UpdateListCallbacks(
    node: ListElementRef,
    componentAtIndex: ComponentAtIndexCallback,
    enqueueComponent: EnqueueComponentCallback,
    componentAtIndexes: ComponentAtIndexesCallback
  ): void;

  function __CreateElement(tag: string, comParentUniID: number, info?: ElementInfo): ElementRef;

  function __AppendElement(parent: ElementRef, current: ElementRef): ElementRef;

  function __RemoveElement(parent: ElementRef, current: ElementRef): ElementRef;

  function __InsertElementBefore(parent: ElementRef, current: ElementRef, marker?: ElementRef): ElementRef;

  function __SwapElement(left: ElementRef, right: ElementRef): void;

  function __ReplaceElement(newElement: ElementRef, oldElement: ElementRef): void;

  function __ReplaceElements(parent: ElementRef, insertedChildren: ElementRef | ElementRef[] | undefined, removedChildren: ElementRef | ElementRef[] | undefined): void;

  function __GetParent(current: ElementRef): ElementRef;

  function __GetChildren(current: ElementRef): ElementRef[];

  function __FirstElement(current: ElementRef): ElementRef;

  function __LastElement(current: ElementRef): ElementRef;

  function __NextElement(node: ElementRef): ElementRef;

  function __GetTag(node: ElementRef): string;

  function __SetAttribute(current: ElementRef | ComposeElementRef, attrName: string, value: any): void;

  function __AddClass(current: ElementRef, className: string): void;

  function __SetClasses(current: ElementRef, className: string | undefined): void;

  function __GetClasses(current: ElementRef): string[];

  function __SetStaticStyle(node: ElementRef, key: number, value: unknown): void;

  function __SetInlineStyles(node: ElementRef, value: unknown): void;

  function __GetInlineStyle(node: ElementRef, propertyId: number): string;

  function __GetInlineStyles(node: ElementRef): string;

  function __SetID(node: ElementRef, id: string | null): void;

  function __GetID(node: ElementRef): string;

  function __SetCSSId(node: ElementRef | ElementRef[], cssId: number, entryName?: string): void;

  function __AddEvent(node: ElementRef, type: string, name: string, func: string | Object | undefined): void;

  function __SetEvents(node: ElementRef, events: Record<string, unknown>[] | undefined): void;

  function __GetEvent(node: ElementRef, name: string, type: string): Record<string, any>;

  function __GetEvents(node: ElementRef): Record<string, Record<string, any>>;

  function __AddDataset(node: ElementRef, key: string, value: unknown): void;

  function __SetDataset(node: ElementRef, value: Record<string, unknown> | undefined): void;

  function __GetDataset(node: ElementRef): Record<string, unknown>;

  function __GetDataByKey(node: ElementRef, key: string): any;

  function __GetElementUniqueID(node: ElementRef): number;

  function __ElementIsEqual(left: ElementRef, right: ElementRef): boolean;

  function __GetComponentID(node: ElementRef): string;

  function __QuerySelector(root: ElementRef, cssSelector: string, params: SelectorParams): ElementRef;

  function __QuerySelectorAll(root: ElementRef, cssSelector: string, params: SelectorParams): ElementRef[];

  function __AddInlineStyle(e: ElementRef, key: number | string, value: unknown): void;

  function __GetAttributeByName(e: ElementRef, name: string): any;

  function __GetAttributeNames(e: ElementRef): string[];

  function __GetAttributes(e: ElementRef): Record<string, any>;

  function __GetPageElement(): ElementRef;

  function __CreateGestureDetector(node: ElementRef, gestureID: number, gestureType: number, config: GestureConfig, relationMap: Record<string, number[]>): void;

  function __SetGestureDetector(node: ElementRef, gestureID: number, gestureType: number, config: GestureConfig, relationMap: Record<string, number[]>): void;

  function __RemoveGestureDetector(node: ElementRef, gestureID: number): void;

  function __SetGestureState(node: ElementRef, gestureID: number, state: number): void;

  function __ConsumeGesture(node: ElementRef, gestureID: number, options: Record<string, boolean>): void;

  function __GetElementByUniqueID(elementId: number): ElementRef | undefined;
}
