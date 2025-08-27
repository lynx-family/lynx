// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { CSSProperties } from '../common';

export interface Element {
  styles: CSSProperties;
  attributes: Record<string, any>;
  scrollBy: (
    width: number,
    height: number
  ) => {
    consumedX: number;
    consumedY: number;
    unconsumedX: number;
    unconsumedY: number;
  };
  getBoundingClientRect: () => {
    width: number;
    height: number;
    top: number;
    bottom: number;
    left: number;
    right: number;
  };

  /**
   * Set an attribute.
   * @param attributeName The name of the attribute.
   * @since Lynx 2.14
   */
  getAttribute(attributeName: string): any;

  /**
   * Get all attribute names as an array.
   * @since Lynx 2.14
   */
  getAttributeNames(): string[];

  /**
   * Set an attribute.
   * @param name The name of the attribute to set.
   * @param value The value to set.
   * @since Lynx 2.14
   */
  setAttribute(name: string, value: any): void;

  /**
   * Set a style property.
   * @param name The name of the style property, in kebab-case.
   * @param value The value of the property.
   * @since Lynx 2.14
   */
  setStyleProperty(name: string, value: string): void;

  /**
   * Set a list of style properties.
   * @param styles The object containing key-value pair of the style properties to set, name of the property in kebab-case.
   * @since Lynx 2.14
   */
  setStyleProperties(styles: Record<string, string>): void;

  /**
   * Select the first element matching the given CSS selector in this element's children.
   * @param selector CSS Selector string.
   * @since Lynx 2.14
   */
  querySelector(selector: string): Element | null;

  /**
   * Select all the elements matching the given CSS selector in this element's children.
   * @param selector CSS Selector string.
   * @since Lynx 2.14
   */
  querySelectorAll(selector: string): Element[];

  /**
   * Invoke a UI method.
   * @param methodName The UI method to invoke.
   * @param params Params of the UI method.
   * @since Lynx 2.14
   */
  invoke(methodName: string, params?: Record<string, any>): Promise<any>;
}
