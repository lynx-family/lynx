// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseMethod, Callback } from '../events';
import { AutoScrollMethod, ListUIMethods } from './list';
import {  ScrollViewUIMethods } from './scroll-view';

export type ListParams = ListUIMethods;

export type ScrollViewParams = ScrollViewUIMethods | AutoScrollMethod;

/**
 * Call this method to get the width, height, and position information of the target element.
 * @Android
 * @iOS
 * @Harmony
 * @PC
 */
interface BoundingClientRectMethod extends BaseMethod {
  method: 'boundingClientRect';
  params: {
    /**
     * The element to which the coordinates are relative. If not specified, the coordinates are relative to the viewport. If the value is `screen`, the coordinates are relative to the screen. If the value is a element's `id`, the coordinates are relative to the element with the specified ID. Else, the coordinates are relative to the viewport of LynxView.
     * @defaultValue null
     * @iOS
     * @Harmony
     * @Android
     */
    relativeTo?: 'screen' | string | null;
    /**
     * Whether to enable transform props on Android, it is recommended to set it to `true` when the element has transform props.
     * @defaultValue false
     * @Android
     */
    androidEnableTransformProps?: boolean;
  };
  success?: Callback<{
    /**
     * The id of the element 
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    id: string;

    /**
     *  Dataset of the element.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    dataset: object;

    /**
     * Left boundary coordinate of the element (in pixels)
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    left: number;

    /**
     *  The right boundary coordinates of the element.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    right: number;

    /**
     *  Upper boundary coordinate of the element.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    top: number;

    /**
     *  The lower boundary coordinates of the element.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    bottom: number;

    /**
     *  Width of the element.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    width: number;

    /**
     * height of the element.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    height: number;
  }>;
}

/**
 * Call this method to take a screenshot of the element.
 * @Android
 * @iOS
 * @Harmony
 * @PC
 */
interface TakeScreenShotMethod extends BaseMethod {
  method: 'takeScreenshot';
  params: {
    /**
     * Specify the image format.
     * @defaultValue 'jpeg'
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    format: 'jpeg' | 'png';
    /**
     * Specify the image quality, within [0, 1], the smaller the value, the blurrier and smaller the size.
     * @defaultValue 1
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    scale?: number;
  };
  success?: Callback<{
    /**
     * The base64-encoded string of the screenshot image.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    data: string;
  }>;
}

/**
 * Set the element to require focus
 * @PC
 */
interface SetFocusMethod extends BaseMethod {
  method: 'setFocus';
  params: {
    /**
     * Set whether the element gains focus, `false` means the element loses focus.
     * @PC
     */
    focus: boolean;
    /**
     * Whether to scroll the element into the visible area at the same time, default is `true`.
     * @PC
     */
    scroll?: boolean;
  };
}

/**
 * Call this method to request accessibility focus for the element.
 * @Android
 * @iOS
 */
interface RequestAccessibilityFocusMethod extends BaseMethod {
  method: 'requestAccessibilityFocus';
}

/**
 * Call this method to check if the element is currently animating.
 * @deprecated Legacy API
 */
interface IsAnimatingMethod extends BaseMethod {
  method: 'isAnimating';
  success?: Callback<{ data: boolean }>;
}

/**
 * Call this method to scroll the element into the visible area.
 * @Android
 * @web
 * @iOS
 * @Harmony
 * @PC
 */
interface ScrollIntoViewMethod extends BaseMethod {
  method: 'scrollIntoView';
  params: {
    /**
     * Specify the scroll options.
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    scrollIntoViewOptions: {
      /**
       * Vertical alignment options: "start" aligns top | "center" centers | "end" aligns bottom
       * @defaultValue 'center'
       * @Android
       * @iOS
       * @Harmony
       * @PC
       */
      block?: 'start' | 'center' | 'end';
      /**
       * Horizontal alignment options: "start" aligns left | "center" centers | "end" aligns right
       * @defaultValue 'start'
       * @Android
       * @iOS
       * @Harmony
       * @PC
       */
      inline?: 'start' | 'center' | 'end';
      /**
       * Specify the scroll behavior.
       * @defaultValue 'smooth'
       * @Android
       * @iOS
       * @Harmony
       * @PC
       */
      behavior?: 'smooth' | 'none';
    };
  };
}

export type InvokeParams = ListParams | ScrollViewParams | BoundingClientRectMethod | SetFocusMethod | IsAnimatingMethod | ScrollIntoViewMethod | TakeScreenShotMethod | RequestAccessibilityFocusMethod;
