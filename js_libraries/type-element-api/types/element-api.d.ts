// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * A reference to an Element managed by the Lynx engine.
 *
 * The frontend framework holds this reference to manage the Element's lifecycle.
 * The Element will only be destroyed when the framework no longer holds it
 * and the Element is removed from the tree.
 */
export interface ElementRef extends Record<string, unknown> { }

/**
 * A reference to a ComponentElement, which represents a frontend framework component.
 */
export interface ComponentElementRef extends ElementRef { }

/**
 * A reference to a PageElement, which is a special type of ComponentElement
 * representing the root page of a Lynx application.
 */
export interface PageElementRef extends ComponentElementRef { }

/**
 * A reference to a ListElement for virtualized list rendering.
 * @internal
 */
export interface ListElementRef extends ElementRef { }

/**
 * A reference to a View element.
 * @internal
 */
export interface ViewElementRef extends ElementRef { }

/**
 * A reference to a Text element.
 * @internal
 */
export interface TextElementRef extends ElementRef { }

/**
 * A reference to a RawText element, representing raw text content.
 * @internal
 */
export interface RawTextElementRef extends ElementRef { }

/**
 * A reference to an Image element.
 * @internal
 */
export interface ImageElementRef extends ElementRef { }

/**
 * A reference to a ScrollView element.
 * @internal
 */
export interface ScrollElementRef extends ElementRef { }

/**
 * A reference to a Wrapper element.
 * @internal
 */
export interface WrapperElementRef extends ElementRef { }

/**
 * A reference to a None element (non-visual placeholder).
 * @internal
 */
export interface NoneElementRef extends ElementRef { }

/**
 * A reference to an If element used for conditional rendering.
 * @internal
 */
export interface IfElementRef extends ElementRef { }

/**
 * A reference to a For element used for list rendering.
 * @internal
 */
export interface ForElementRef extends ElementRef { }

/**
 * A reference to a Block element.
 * @internal
 */
export interface BlockElementRef extends ElementRef { }

/**
 * A reference to a style object managed by the Lynx engine.
 * @internal
 */
export interface StyleObjectRef extends ElementRef { }

/**
 * Additional information that can be provided when creating an Element.
 */
export type ElementInfo = Record<string, any>;

/**
 * Options for configuring the rendering pipeline.
 * @internal
 */
export interface PipelineOptions {
  pipelineID: string
  needTimestamps: boolean
}

/**
 * Parameters for element selector queries.
 * @internal
 */
export interface SelectorParams {
  onlyCurrentComponent?: boolean;
}

/**
 * Result returned from a dynamic component query.
 * @internal
 */
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

/**
 * Callback invoked to create a component at a specific index in a list.
 * @internal
 */
export type ComponentAtIndexCallback = (
  listRef: ElementRef,
  listElementId: number,
  cellIndex: number,
  opId: number,
  enableReuseNotification?: boolean,
  enableBatchRender?: boolean,
  asyncFlush?: boolean,
) => number | undefined | Promise<number>

/**
 * Callback invoked to create components at multiple indexes in a list.
 * @internal
 */
export type ComponentAtIndexesCallback = (
  listRef: ElementRef,
  listElementId: number,
  cellIndexes: number[],
  opIds: number[],
  enableReuseNotification: boolean,
  asyncFlush: boolean,
) => void

/**
 * Callback invoked to enqueue a component for a list.
 * @internal
 */
export type EnqueueComponentCallback = (
  listRef: ElementRef,
  listId: number,
  eleId: number,
) => void

/**
 * Animation operation types for the {@link __ElementAnimate} function.
 */
export enum AnimationOperation {
  /** Start a new animation. */
  START = 0,
  /** Play or resume a paused animation. */
  PLAY = 1,
  /** Pause an existing animation. */
  PAUSE = 2,
  /** Cancel an animation. */
  CANCEL = 3,
}

/**
 * Timing and configuration options for an animation.
 */
export interface AnimationTimingOptions {
  /** Animation name (optional, auto-generated if not provided). */
  name?: string;
  /** Animation duration. */
  duration?: number | string;
  /** Animation delay. */
  delay?: number | string;
  /** Number of iterations (can be `'infinite'`). */
  iterationCount?: number | string;
  /** Animation fill mode. */
  fillMode?: string;
  /** Animation timing function. */
  timingFunction?: string;
  /** Animation direction. */
  direction?: string;
}

/**
 * A keyframe definition for animation, mapping CSS property names to values.
 */
export type Keyframe = Record<string, string | number>;

/**
 * Configuration for a gesture detector.
 * @internal
 */
export interface GestureConfig {
  callbacks: {
    name: string;
    callback: unknown;
  }[];
  config?: Record<string, unknown>;
}


declare global {
  // ──────────────────────────────────────────────────────────────────────────
  // Element Creation (Public)
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Creates a PageElement object.
   *
   * The PageElement is a special type of ComponentElement.
   *
   * @param componentId - The componentID assigned to the PageElement by the frontend framework,
   *   which is a case-sensitive string.
   * @param cssId - The CSSFragment ID, which is a number.
   * @param info - Additional information required when creating the PageElement; this parameter
   *   can be omitted.
   * @returns The created PageElement.
   *
   * @example
   * ```js
   * let page = __CreatePage('0', 0, {});
   * ```
   */
  function __CreatePage(componentId: string, cssId: number, info?: ElementInfo): PageElementRef;

  /**
   * Creates a ComponentElement object.
   *
   * @param parentComponentUniId - The Unique Component ID of the ComponentElement that creates
   *   this Element, which is a number.
   * @param componentId - The componentID set by the frontend framework for the ComponentElement,
   *   which is a case-sensitive string.
   * @param cssId - The CSSFragment ID, which is a number.
   * @param entryName - The schema information of the Lazy Bundle where the current
   *   ComponentElement is located, which is a case-sensitive string.
   * @param name - The frontend-defined name of the current ComponentElement, which is a
   *   case-sensitive string.
   * @param path - The frontend path of the current ComponentElement, which is a case-sensitive
   *   string.
   * @param config - The configuration for the current ComponentElement.
   * @returns The created ComponentElement.
   *
   * @example
   * ```js
   * let element = __CreateComponent(0, '1', 1, '', 'name', 'path', {});
   * ```
   */
  function __CreateComponent(
    parentComponentUniId: number,
    componentId: string,
    cssId: number,
    entryName: string,
    name: string,
    path: string,
    config?: Record<string, unknown>
  ): ComponentElementRef;

  // ──────────────────────────────────────────────────────────────────────────
  // Element Creation (Internal)
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
  function __CreateView(parentComponentUniId: number, info?: ElementInfo): ViewElementRef;

  /** @internal */
  function __CreateScrollView(parentComponentUniId: number, info?: ElementInfo): ScrollElementRef;

  /** @internal */
  function __CreateText(parentComponentUniId: number, info?: ElementInfo): TextElementRef;

  /** @internal */
  function __CreateRawText(text: string, info?: ElementInfo): RawTextElementRef;

  /** @internal */
  function __CreateImage(parentComponentUniId: number, info?: ElementInfo): ImageElementRef;

  /** @internal */
  function __CreateWrapperElement(parentComponentUniId: number): WrapperElementRef;

  /** @internal */
  function __CreateNonElement(parentComponentUniId: number): ElementRef;

  /** @internal */
  function __CreateIf(parentComponentUniId: number, info?: ElementInfo): IfElementRef;

  /** @internal */
  function __CreateFor(parentComponentUniId: number, info?: ElementInfo): ForElementRef;

  /** @internal */
  function __CreateBlock(parentComponentUniId: number, info?: ElementInfo): BlockElementRef;

  /** @internal */
  function __CreateList(
    parentComponentUniId: number,
    componentAtIndex: ComponentAtIndexCallback,
    enqueueComponent: EnqueueComponentCallback,
    info?: ElementInfo,
    componentAtIndexes?: ComponentAtIndexesCallback,
  ): ListElementRef;

  /** @internal */
  function __UpdateListCallbacks(
    node: ListElementRef,
    componentAtIndex: ComponentAtIndexCallback,
    enqueueComponent: EnqueueComponentCallback,
    componentAtIndexes: ComponentAtIndexesCallback,
  ): void;

  // ──────────────────────────────────────────────────────────────────────────
  // Element Creation (Public, generic)
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Creates an Element object.
   *
   * The return value is a reference-counted Element managed by `base::scoped_refptr<Element>`.
   * The frontend framework can hold this return value to manage the Element's lifecycle.
   * The Element will only be destroyed when the frontend framework no longer holds it
   * and the Element is removed from the tree.
   *
   * @param tag - A string that specifies the type of Element to create, which corresponds to the
   *   LynxUI tagName, such as `input`. Built-in Elements like `view`, `image`, etc. cannot use
   *   this interface and must use their respective Create APIs.
   * @param comParentUniID - The Unique Component ID of the ComponentElement that creates this
   *   Element, which is a number.
   * @param info - Additional information required when creating the Element; this parameter can
   *   be omitted.
   * @returns The created Element.
   *
   * @example
   * ```js
   * let element = __CreateElement('input', 0, {});
   * ```
   */
  function __CreateElement(tag: string, comParentUniID: number, info?: ElementInfo): ElementRef;

  // ──────────────────────────────────────────────────────────────────────────
  // DOM Operations
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Inserts an Element object after the last child of the current Element.
   *
   * @param parent - The parent Element object.
   * @param current - The child Element to be appended to the parent.
   * @returns The appended child Element.
   *
   * @example
   * ```js
   * let child = __CreateElement('view', 0, {});
   * let parent = __CreateElement('view', 0, {});
   * __AppendElement(parent, child);
   * ```
   */
  function __AppendElement(parent: ElementRef, current: ElementRef): ElementRef;

  /**
   * Removes an Element node from its parent's list of child nodes.
   *
   * @param parent - The parent node object.
   * @param current - The child node to be removed from the parent.
   * @returns The removed child node.
   *
   * @example
   * ```js
   * let child = __CreateElement('view', 0, {});
   * let parent = __CreateElement('view', 0, {});
   * __AppendElement(parent, child);
   * __RemoveElement(parent, child);
   * ```
   */
  function __RemoveElement(parent: ElementRef, current: ElementRef): ElementRef;

  /**
   * Inserts an Element node into the specified parent node, before the reference node.
   *
   * @param parent - The parent node object.
   * @param current - The child node to be inserted into the parent node.
   * @param marker - The reference node before which the child node needs to be inserted. If the
   *   reference node is null or undefined, the child node will be inserted at the end.
   * @returns The inserted child node.
   *
   * @example
   * ```js
   * let first = __CreateElement('view', 0, {});
   * let last = __CreateElement('view', 0, {});
   * let mid = __CreateElement('view', 0, {});
   * let parent = __CreateElement('view', 0, {});
   * __AppendElement(parent, first);
   * __AppendElement(parent, last);
   * __InsertElementBefore(parent, mid, last);
   * ```
   */
  function __InsertElementBefore(parent: ElementRef, current: ElementRef, marker?: ElementRef): ElementRef;

  /**
   * Swaps the positions of two Elements.
   *
   * @param a - The first element to be swapped.
   * @param b - The second element to be swapped.
   *
   * @example
   * ```js
   * let childA = __CreateElement('view', 0, {});
   * let childB = __CreateElement('view', 0, {});
   * let parent = __CreateElement('view', 0, {});
   * __AppendElement(parent, childA);
   * __AppendElement(parent, childB);
   * __SwapElement(childA, childB);
   * ```
   */
  function __SwapElement(a: ElementRef, b: ElementRef): void;

  /**
   * Replaces an Element node with a specified node.
   *
   * @param a - The new node that will replace the old node. If this node already exists in the
   *   Element tree, it will first be removed from its original position.
   * @param b - The original node that is being replaced.
   *
   * @example
   * ```js
   * let newChild = __CreateElement('view', 0, {});
   * let oldChild = __CreateElement('view', 0, {});
   * let parent = __CreateElement('view', 0, {});
   * __AppendElement(parent, oldChild);
   * __ReplaceElement(newChild, oldChild);
   * ```
   */
  function __ReplaceElement(a: ElementRef, b: ElementRef): void;

  /**
   * Replaces a list of Element nodes with a specified list of nodes.
   *
   * @param parent - The parent node whose child nodes will be batch-replaced.
   * @param a - The list of new nodes that will replace the old nodes.
   * @param b - The list of original nodes that are being replaced.
   *
   * @example
   * ```js
   * let newChild = __CreateElement('view', 0, {});
   * let oldChild = __CreateElement('view', 0, {});
   * let parent = __CreateElement('view', 0, {});
   * __AppendElement(parent, oldChild);
   * __ReplaceElements(parent, [newChild], [oldChild]);
   * ```
   */
  function __ReplaceElements(parent: ElementRef, a: ElementRef | ElementRef[] | undefined, b: ElementRef | ElementRef[] | undefined): void;

  // ──────────────────────────────────────────────────────────────────────────
  // Element Traversal
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Returns the parent node of the Element.
   *
   * @param current - Any Element object.
   * @returns The parent node of the current node. If there is no parent node, it returns undefined.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * let parent = __GetParent(element);
   * ```
   */
  function __GetParent(current: ElementRef): ElementRef;

  /**
   * Returns all child nodes of an Element in order, in the form of an array.
   *
   * @param current - Any Element object.
   * @returns All child nodes of the current Element as an array.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * let children = __GetChildren(element);
   * ```
   */
  function __GetChildren(current: ElementRef): ElementRef[];

  /**
   * Returns the first child node of an Element.
   *
   * @param current - Any Element object.
   * @returns The first child Element of the current Element.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * let first = __FirstElement(element);
   * ```
   */
  function __FirstElement(current: ElementRef): ElementRef;

  /**
   * Returns the last child node of the Element.
   *
   * @param current - Any Element object.
   * @returns The last child node of the current Element.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * let last = __LastElement(element);
   * ```
   */
  function __LastElement(current: ElementRef): ElementRef;

  /**
   * Returns the node that immediately follows the specified Element node in its parent's
   * list of child nodes.
   *
   * If the specified node is the last node, it returns undefined.
   *
   * @param node - Any Element object.
   * @returns The node that immediately follows the current Element in its parent's list of
   *   child nodes, or undefined if the current Element is the last node.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * let next = __NextElement(element);
   * ```
   */
  function __NextElement(node: ElementRef): ElementRef;

  // ──────────────────────────────────────────────────────────────────────────
  // Attributes
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Returns the tag selector of the Element node.
   *
   * @param node - Any Element object.
   * @returns The tag selector of the Element object.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * let tag = __GetTag(element);
   * ```
   */
  function __GetTag(node: ElementRef): string;

  /**
   * Sets an attribute for a specific Element node.
   *
   * @param current - Any Element object.
   * @param attrName - A string representing the attribute key to be set.
   * @param value - Any value representing the attribute value to be set.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetAttribute(element, 'src', 'xx');
   * ```
   */
  function __SetAttribute(current: ElementRef, attrName: string, value: any): void;

  /**
   * Adds a class selector to the Element object without affecting existing class selectors.
   *
   * @param current - Any Element object.
   * @param className - The new className, of type string.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __AddClass(element, 'A');
   * __AddClass(element, 'B');
   * __AddClass(element, 'C');
   * ```
   */
  function __AddClass(current: ElementRef, className: string): void;

  /**
   * Sets class selectors for a specific Element node.
   *
   * @param current - Any Element object.
   * @param className - A new list of class selectors, which will overwrite the existing
   *   class selectors, of type string.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetClasses(element, 'A B C');
   * ```
   */
  function __SetClasses(current: ElementRef, className: string | undefined): void;

  /**
   * Returns all class selectors of an Element in order, in the form of an array.
   *
   * @param current - Any Element object.
   * @returns The class selectors of the current Element as an array.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetClasses(element, 'A B C');
   * let classNames = __GetClasses(element);
   * ```
   */
  function __GetClasses(current: ElementRef): string[];

  // ──────────────────────────────────────────────────────────────────────────
  // Styles
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
  function __SetStaticStyle(node: ElementRef, key: number, value: unknown): void;

  /**
   * Sets inline styles for a specific Element node.
   *
   * @param node - Any Element object.
   * @param value - Inline styles that override existing inline styles, of type string or object.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetInlineStyles(element, {
   *   top: '10px',
   *   left: '10px',
   *   right: '10px',
   *   bottom: '10px',
   * });
   * ```
   */
  function __SetInlineStyles(node: ElementRef, value: unknown): void;

  /** @internal */
  function __GetInlineStyle(node: ElementRef, propertyId: number): string;

  /**
   * Returns the inline styles of the Element.
   *
   * @param node - Any Element object.
   * @returns The inline styles of the current Element in the form of a map.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetInlineStyles(element, 'top:10px;left:10px;right:10px;bottom:10px;');
   * let styles = __GetInlineStyles(element);
   * ```
   */
  function __GetInlineStyles(node: ElementRef): string;

  /**
   * Sets an ID selector for a specific Element node.
   *
   * @param node - Any Element object.
   * @param id - The ID selector to be set for the Element object, which should be of type string.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetID(element, 'video');
   * ```
   */
  function __SetID(node: ElementRef, id: string | null): void;

  /**
   * Returns the ID selector of the Element.
   *
   * @param node - Any Element object.
   * @returns The ID selector of the current Element. If it has not been set, an empty string is returned.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetID(element, 'video');
   * let id = __GetID(element);
   * ```
   */
  function __GetID(node: ElementRef): string;

  /**
   * Sets a CSS ID for a specific Element node.
   *
   * @param node - Any Element object, or an array of Element objects.
   * @param cssId - The CSS ID, which is of type number.
   * @param entryName - Optional entry name associated with the CSS ID.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetCSSId(element, 0);
   * ```
   */
  function __SetCSSId(node: ElementRef | ElementRef[], cssId: number, entryName?: string): void;

  // ──────────────────────────────────────────────────────────────────────────
  // Events
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Adds an Event Listener to the Element object.
   *
   * Due to limitations of the Lynx SDK, currently only one listener can be added for the same
   * type and name. Adding a listener with the same type and name again will overwrite the
   * previously added listener.
   *
   * @param node - Any Element object.
   * @param type - A case-sensitive string representing the type of the event to listen for.
   * @param name - A case-sensitive string representing the name of the event to listen for.
   * @param func - The listener can be a case-sensitive string, or it can be null or undefined.
   *   When the listener is a string, upon triggering the listened event, the Lynx SDK will send
   *   the current Element's parentComponentUniqueID, listener, and event to the background thread.
   *   When null or undefined, the Lynx SDK will remove the corresponding listener.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __AddEvent(element, 'bindEvent', 'tap', 'onTap');
   * ```
   */
  function __AddEvent(node: ElementRef, type: string, name: string, func: string | Object | undefined): void;

  /**
   * Sets event listeners for a specific Element node.
   *
   * @param node - Any Element object.
   * @param events - An array of listeners that describes the event listeners to add.
   *   Each element is a map containing `type`, `name`, and `function` keys.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetEvents(element, [{ type: 'bindEvent', name: 'tap', function: 'onTap' }]);
   * ```
   */
  function __SetEvents(node: ElementRef, events: Record<string, unknown>[] | undefined): void;

  /**
   * Returns the event listener specific to the Element.
   *
   * @param node - Any Element object.
   * @param name - A case-sensitive string representing the event name.
   * @param type - A case-sensitive string representing the event type.
   * @returns The event listener for the current Element with the specified name.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetEvents(element, [{ type: 'bindEvent', name: 'tap', function: 'onTap' }]);
   * let listener = __GetEvent(element, 'tap', 'bindEvent');
   * ```
   */
  function __GetEvent(node: ElementRef, name: string, type: string): Record<string, any>;

  /**
   * Returns all event listeners of the Element in the form of an array.
   *
   * @param node - Any Element object.
   * @returns An array of all event listeners for the current Element, where each element
   *   in the array is a map that describes a single event listener.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetEvents(element, [{ type: 'bindEvent', name: 'tap', function: 'onTap' }]);
   * let listeners = __GetEvents(element);
   * ```
   */
  function __GetEvents(node: ElementRef): Record<string, Record<string, any>>;

  // ──────────────────────────────────────────────────────────────────────────
  // Dataset
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Adds a data- attribute to the Element object without affecting existing data- attributes.
   *
   * @param node - Any Element object.
   * @param key - A case-sensitive string representing the dataset key.
   * @param value - The dataset value, which can be of any type.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __AddDataset(element, 'key', 'value');
   * ```
   */
  function __AddDataset(node: ElementRef, key: string, value: unknown): void;

  /**
   * Sets data- attributes for a specific Element node.
   *
   * @param node - Any Element object.
   * @param value - The dataset to be set for the Element object, which is a map object.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetDataset(element, { key: 'value' });
   * ```
   */
  function __SetDataset(node: ElementRef, value: Record<string, unknown> | undefined): void;

  /**
   * Returns all data- attributes of an Element node in the form of a map.
   *
   * @param node - Any Element object.
   * @returns The dataset of the current Element as a map.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetDataset(element, { key: 'value' });
   * let dataset = __GetDataset(element);
   * ```
   */
  function __GetDataset(node: ElementRef): Record<string, unknown>;

  /**
   * Returns the value of a specific data- attribute of an Element.
   *
   * @param node - Any Element object.
   * @param key - A case-sensitive string representing the data- key.
   * @returns The data- value corresponding to the data- key of the current Element.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetDataset(element, { key: 'value' });
   * let value = __GetDataByKey(element, 'key');
   * ```
   */
  function __GetDataByKey(node: ElementRef, key: string): any;

  // ──────────────────────────────────────────────────────────────────────────
  // Element Info
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Returns the unique ID of an Element.
   *
   * @param node - Any Element object.
   * @returns A unique ID of the Element object within the current LynxView, represented as a number.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * let id = __GetElementUniqueID(element);
   * ```
   */
  function __GetElementUniqueID(node: ElementRef): number;

  /**
   * Determines if the two passed Element objects are the same Element.
   *
   * @param left - The left node to be compared.
   * @param right - The right node to be compared.
   * @returns `true` only when the left and right are the same node.
   *
   * @example
   * ```js
   * let childA = __CreateElement('view', 0, {});
   * let childB = __CreateElement('view', 0, {});
   * let isEqual = __ElementIsEqual(childA, childB);
   * ```
   */
  function __ElementIsEqual(left: ElementRef, right: ElementRef): boolean;

  // ──────────────────────────────────────────────────────────────────────────
  // Component
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Updates the componentID of a ComponentElement.
   *
   * @param node - Any ComponentElement.
   * @param id - The componentID assigned to the ComponentElement by the frontend framework,
   *   which is case-sensitive.
   *
   * @example
   * ```js
   * let element = __CreateComponent(0, '1', 1, '', 'name', 'path', {});
   * __UpdateComponentID(element, '2');
   * ```
   */
  function __UpdateComponentID(node: ElementRef, id: string): void;

  /**
   * Returns the componentID of a ComponentElement.
   *
   * @param node - Any ComponentElement.
   * @returns The componentID of the ComponentElement.
   *
   * @example
   * ```js
   * let element = __CreateComponent(0, '1', 1, '', 'name', 'path', {});
   * let id = __GetComponentID(element);
   * ```
   */
  function __GetComponentID(node: ElementRef): string;

  /** @internal */
  function __UpdateComponentInfo(
    node: ElementRef,
    params: {
      componentID?: string;
      name?: string;
      path?: string;
      entry?: string;
      cssID?: number;
      config?: Record<string, unknown>;
    }
  ): void;

  // ──────────────────────────────────────────────────────────────────────────
  // Rendering / Lifecycle (Internal)
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
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
      elementIDs?: number[];
      operationIDs?: number[];
      asyncFlush?: boolean;
      onLayoutReady?: () => void;
    }
  ): void;

  /** @internal */
  function __AsyncResolveElement(element: ElementRef): void;

  /** @internal */
  function __AsyncResolveSubtree(node: ElementRef): void

  /** @internal */
  function __OnLifecycleEvent(args: any[]): void;

  /** @internal */
  function _ReportError(
    err: Error,
    info: {
      errorCode: number;
    }
  ): void;

  // ──────────────────────────────────────────────────────────────────────────
  // Template (Internal)
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
  function __ElementFromBinary(elementTemplateKey: string, parentComponentUniId: number): ElementRef[];

  /** @internal */
  function __GetTemplateParts(ele: ElementRef): Record<string, ElementRef>;

  /** @internal */
  function __CloneElement(ele: ElementRef, options: Record<string, any>): ElementRef;

  /** @internal */
  function __IsTemplateElement(ele: ElementRef): boolean;

  /** @internal */
  function __MarkTemplateElement(ele: ElementRef): void;

  /** @internal */
  function __MarkPartElement(ele: ElementRef, key: string): void;

  // ──────────────────────────────────────────────────────────────────────────
  // Selectors (Internal)
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
  function __QuerySelector(root: ElementRef, cssSelector: string, params: SelectorParams): ElementRef;

  /** @internal */
  function __QuerySelectorAll(root: ElementRef, cssSelector: string, params: SelectorParams): ElementRef[];

  // ──────────────────────────────────────────────────────────────────────────
  // Config
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Adds a config key and config value to the Element object.
   *
   * Developers can set some custom configs for the Element to facilitate
   * extensions by the frontend framework.
   *
   * @param ele - Any Element object.
   * @param key - A case-sensitive string representing the config key.
   * @param value - The config value, which can be of any type.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __AddConfig(element, 'anyConfig', {});
   * ```
   */
  function __AddConfig(ele: ElementRef, key: string, value: any): void;

  /**
   * Sets a configuration for a specific Element node.
   *
   * @param ele - Any Element object.
   * @param config - The configuration to be set for the Element object, which is a map object.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetConfig(element, { anyConfig: {} });
   * ```
   */
  function __SetConfig(ele: ElementRef, config: Record<string, any>): void;

  /**
   * Returns all configurations of an Element node in the form of a map.
   *
   * @param ele - Any Element object.
   * @returns The config of the current Element as a map.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetConfig(element, { anyConfig: {} });
   * let config = __GetElementConfig(element);
   * ```
   */
  function __GetElementConfig(ele: ElementRef): Record<string, unknown>;

  /** @internal */
  function __QueryComponent(source: string, callback?: (evalResult: DynamicComponentResult) => void): { evalResult: unknown };

  /**
   * Adds inline styles to the Element object.
   *
   * @param e - Any Element object.
   * @param key - A number that corresponds to a style key (e.g., 0 corresponds to `top`),
   *   or a string style key.
   * @param value - The style value to be set, which can be a string, null, or undefined.
   *   When a string is provided, the Lynx SDK will set the corresponding key and value for the Element;
   *   if previously set, it will overwrite the previous value. When null or undefined is provided,
   *   the Lynx SDK will remove the key from the inline styles of the Element.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __AddInlineStyle(element, 0, '10px');
   * ```
   */
  function __AddInlineStyle(e: ElementRef, key: number | string, value: unknown): void;

  /** @internal */
  function __GetAttributeByName(e: ElementRef, name: string): any;

  /** @internal */
  function __GetAttributeNames(e: ElementRef): string[];

  /**
   * Returns all attributes of an Element in the form of a map.
   *
   * @param e - Any Element object.
   * @returns The attribute map of the current Element, where the map key is the attribute key
   *   and the map value is the attribute value.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetAttribute(element, 'src', 'xx');
   * let attributeMap = __GetAttributes(element);
   * ```
   */
  function __GetAttributes(e: ElementRef): Record<string, any>;

  /** @internal */
  function __GetPageElement(): ElementRef;

  /** @internal */
  function __InvokeUIMethod(e: ElementRef, method: string, params: Record<string, unknown>, callback: (res: { code: number; data: unknown }) => void): ElementRef[];

  /** @internal */
  function __LoadLepusChunk(name: string, cfg: { chunkType: number }): void;

  // ──────────────────────────────────────────────────────────────────────────
  // Gesture (Internal)
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
  function __CreateGestureDetector(
    node: ElementRef,
    gestureID: number,
    gestureType: number,
    config: GestureConfig,
    relationMap: Record<string, number[]>
  ): void;

  /** @internal */
  function __SetGestureDetector(
    node: ElementRef,
    gestureID: number,
    gestureType: number,
    config: GestureConfig,
    relationMap: Record<string, number[]>
  ): void;

  /** @internal */
  function __RemoveGestureDetector(node: ElementRef, gestureID: number): void;

  /** @internal */
  function __SetGestureState(node: ElementRef, gestureID: number, state: number): void;

  /** @internal */
  function __ConsumeGesture(node: ElementRef, gestureID: number, options: Record<string, boolean>): void;

  // ──────────────────────────────────────────────────────────────────────────
  // Pipeline / Timing (Internal)
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
  function __GeneratePipelineOptions(): Record<string, any>;

  /** @internal */
  function __OnPipelineStart(pipeLineId: string, pipeLineOrigin: string): void;

  /** @internal */
  function __BindPipelineIDWithTimingFlag(pipeLineId: string, timingFlag: string): void;

  /** @internal */
  function __MarkTiming(pipeLineId: string, timingFlag: string): void;

  /** @internal */
  function __AddTimingListener(): void;

  /** @internal */
  function __SetLepusInitData(initData: Object): void;

  /** @internal */
  function __GetElementByUniqueID(elementId: number): ElementRef | undefined;

  // ──────────────────────────────────────────────────────────────────────────
  // If / For (Internal)
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
  function __UpdateIfNodeIndex(node: IfElementRef, ifIndex: number): void;

  /** @internal */
  function __UpdateForChildCount(node: ForElementRef, childCount: number): void;

  // ──────────────────────────────────────────────────────────────────────────
  // StyleObject (Internal)
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
  function __CreateStyleObject(styleObject: Object): StyleObjectRef;

  /** @internal */
  function __SetStyleObject(elementRef: ElementRef, styleObjects: Array<Object>): void;

  /** @internal */
  function __UpdateStyleObject(styleObjectRef: StyleObjectRef, styleObject: Object): void;

  // ──────────────────────────────────────────────────────────────────────────
  // Animation (Internal)
  // ──────────────────────────────────────────────────────────────────────────

  /** @internal */
  function __ElementAnimate(
    element: ElementRef,
    args: [
      operation: AnimationOperation,
      name: string,
      keyframes: Keyframe[],
      options?: AnimationTimingOptions,
    ] | [
      operation: AnimationOperation.PAUSE | AnimationOperation.PLAY | AnimationOperation.CANCEL,
      name: string,
    ],
  ): void;

  /** @internal */
  function __CreateFrame(comParentUniID: number, options?: Record<string, unknown>): ElementRef;

  // ──────────────────────────────────────────────────────────────────────────
  // Computed Style
  // ──────────────────────────────────────────────────────────────────────────

  /**
   * Returns the resolved style value for a CSS property of an element.
   *
   * Unlike browsers, this PAPI won't trigger document reflow.
   *
   * @param element - Any Element object.
   * @param key - A string indicating the CSS property key to be retrieved.
   * @returns Resolved style value for the specified CSS property.
   *
   * @example
   * ```js
   * let element = __CreateElement('view', 0, {});
   * __SetInlineStyles(element, {
   *   top: '10px',
   *   left: '10px',
   *   right: '10px',
   *   bottom: '10px',
   * });
   *
   * let width = __GetComputedStyleByKey(element, 'width');
   * ```
   */
  function __GetComputedStyleByKey(element: ElementRef, key: string): string;
}
