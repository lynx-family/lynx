// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Experimental Element APIs for framework integrations.
 *
 * This tier includes every stable API and adds newer capabilities whose
 * signatures may continue to evolve.
 */

import type { ComposeElementRef, ElementRef } from './shared';

export * from './stable';
export type { ComposeElementRef } from './shared';

export enum ComposeElementKind {
  View = 1,
  Text = 2,
  Image = 3,
}

export interface StyleObjectRef extends ElementRef {}

/**
 * Animation operation types for ElementAnimate function
 */
export enum AnimationOperation {
  START = 0, // Start a new animation
  PLAY = 1, // Play/resume a paused animation
  PAUSE = 2, // Pause an existing animation
  CANCEL = 3, // Cancel an animation
}

/**
 * Animation timing options configuration
 */
export interface AnimationTimingOptions {
  name?: string; // Animation name (optional, auto-generated if not provided)
  duration?: number | string; // Animation duration
  delay?: number | string; // Animation delay
  iterationCount?: number | string; // Number of iterations (can be 'infinite')
  fillMode?: string; // Animation fill mode
  timingFunction?: string; // Animation timing function
  direction?: string; // Animation direction
}

/**
 * Keyframe definition for animation
 */
export type Keyframe = Record<string, string | number>;

export enum ElementEventClosureType {
  NONE = 0,
  JS = 1,
  CORE = 2,
  CLIENT = 3,
}

export enum ElementEventBindType {
  NONE = 0,
  BUBBLE = 1,
  CAPTURE = 2,
  CAPTURE_CATCH = 3,
  BUBBLE_CATCH = 4,
  GLOBAL_BIND = 5,
}

export enum ElementEventType {
  NONE = 0,
  TOUCH = 1,
  KEYBOARD = 2,
  WHEEL = 3,
  POINTER = 4,
  UI = 5,
  MOUSE = 6,
  MESSAGE = 7,
  CUSTOM = 8,
}

export interface ElementEventRef extends Record<string, unknown> {}

export interface ElementEvent extends Record<string, unknown> {
  ref?: ElementEventRef;
}

export type ElementEventCallback = (event: ElementEvent) => void;

export interface ElementEventListenerOptions {
  capture?: boolean;
  once?: boolean;
  passive?: boolean;
  signal?: boolean;
  closure_type?: ElementEventClosureType.NONE | ElementEventClosureType.CORE | ElementEventClosureType.CLIENT;
  bind_type?: ElementEventBindType;
}

export interface ElementEventOptions {
  capture?: boolean;
  bubbles?: boolean;
  cancelable?: boolean;
  composed?: boolean;
}

export type SerializableValue = any;

export type SerializedTemplateInstance = SerializedCompiledTemplateInstance | SerializedTypedTemplateInstance;

export interface SerializedCompiledTemplateInstance {
  templateKey: string;
  bundleUrl?: string;
  attributeSlots?: SerializableValue[] | null;
  elementSlots?: SerializedTemplateInstance[][] | null;
  options?: Record<string, any> | null;
  uid: number | string;
}

export interface SerializedTypedTemplateInstance {
  tag: string;
  attributes?: Record<string, SerializableValue> | null;
  elementSlots?: SerializedTemplateInstance[][] | null;
  options?: Record<string, any> | null;
  uid: number | string;
}

declare global {
  function __CreateCompose(parentComponentUniId: number, kind: ComposeElementKind): ComposeElementRef;

  function __InsertElementAt(parent: ElementRef | ComposeElementRef, current: ElementRef | ComposeElementRef, index: number): void;

  function __RemoveElementsAt(parent: ElementRef | ComposeElementRef, index: number, count: number): void;

  function __MoveElements(parent: ElementRef | ComposeElementRef, from: number, to: number, count: number): void;

  function __SetComposeModifier(owner: ComposeElementRef, modifier: object | null | undefined): void;

  function __AddEventListener(node: ElementRef, name: string, callback: ElementEventCallback | string, options: ElementEventListenerOptions): void;

  function __RemoveEventListener(node: ElementRef, name: string, callback: ElementEventCallback | string, options: ElementEventListenerOptions): void;

  function __RemoveEventListeners(node: ElementRef): void;

  function __CreateEvent(type: ElementEventType, name: string, options: ElementEventOptions, detail: Record<string, unknown>): ElementEventRef;

  function __DispatchEvent(node: ElementRef, event: ElementEventRef): boolean;

  function __StopPropagation(event: ElementEventRef): void;

  function __StopImmediatePropagation(event: ElementEventRef): void;

  /**
   * Create a template instance from the complete initial slot state.
   * `attributeSlots[i]` maps to `attrSlotIndex = i`.
   * `elementSlots[i]` maps to `elementSlotIndex = i`.
   */
  function __CreateElementTemplate(
    templateKey: string,
    bundleUrl: string | null | undefined,
    attributeSlots: any[] | null | undefined,
    elementSlots: ElementRef[][] | null | undefined,
    uid: number | string,
    options?: Record<string, any> | null | undefined
  ): ElementRef;

  /**
   * Create a typed template instance. `elementSlots[0]` is mounted as the root element's children.
   */
  function __CreateTypedElementTemplate(
    tag: string,
    attributes: Record<string, SerializableValue> | null | undefined,
    elementSlots: ElementRef[][] | null | undefined,
    uid: number | string,
    options?: Record<string, any> | null | undefined
  ): ElementRef;

  /**
   * Update one dynamic attribute slot on an existing compiled template instance.
   *
   * For typed template instances, `attrSlotIndex === 0` applies `value` as
   * the root spread attributes. Passing `null` or `undefined` clears the
   * previously applied root spread attributes.
   */
  function __SetAttributeOfElementTemplate(templateInstance: ElementRef, attrSlotIndex: number, value: any): void;

  /**
   * Insert or move a child into one element slot.
   * If `referenceChild` is omitted or `null`, append to the slot tail.
   */
  function __InsertNodeToElementTemplate(templateInstance: ElementRef, elementSlotIndex: number, child: ElementRef, referenceChild?: ElementRef | null): void;

  /**
   * Remove a child from one element slot.
   */
  function __RemoveNodeFromElementTemplate(templateInstance: ElementRef, elementSlotIndex: number, child: ElementRef): void;

  /**
   * Serialize a template instance into a machine-consumable structure used by
   * hydration and cross-thread transfer.
   */
  function __SerializeElementTemplate(templateInstance: ElementRef): SerializedTemplateInstance;

  function __CreateStyleObject(styleObject: Object): StyleObjectRef;

  function __SetStyleObject(elementRef: ElementRef, styleObjects: Array<Object>): void;

  function __UpdateStyleObject(styleObjectRef: StyleObjectRef, styleObject: Object): void;

  /**
   * ElementAnimate function - controls animations on DOM elements
   * @param element - The DOM element to animate (FiberElement reference)
   * @param args - Animation configuration array
   * @returns undefined
   */
  function __ElementAnimate(
    element: ElementRef,
    args:
      | [
          operation: AnimationOperation, // Animation operation type
          name: string, // Animation name
          keyframes: Keyframe[], // Array of keyframes
          options?: AnimationTimingOptions // Timing and configuration options
        ]
      | [
          operation: AnimationOperation.PAUSE | AnimationOperation.PLAY | AnimationOperation.CANCEL,
          name: string // Animation name to pause/play
        ]
  ): void;

  function __CreateFrame(comParentUniID: number, options?: Record<string, unknown>): ElementRef;
}
