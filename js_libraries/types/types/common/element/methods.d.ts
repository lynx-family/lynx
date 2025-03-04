// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseMethod, Callback } from '../events';
import { AutoScrollMethod, ScrollToPositionMethod } from './list';
import { ScrollToMethod } from './scroll-view';

export type ListParams = ScrollToPositionMethod;

export type ScrollViewParams = ScrollToMethod | AutoScrollMethod;

interface BoundingClientRectMethod extends BaseMethod {
  method: 'boundingClientRect';
  success?: Callback<{
    /**
     *  Node ID.
     */
    id: string;

    /**
     *  Dataset of nodes.
     */
    dataset: object;

    /**
     * Left boundary coordinate of the node (in pixels)
     */
    left: number;

    /**
     *  The right boundary coordinates of the node.
     */
    right: number;

    /**
     *  Upper boundary coordinate of the node.
     */
    top: number;

    /**
     *  The lower boundary coordinates of the node
     */
    bottom: number;

    /**
     *  Width of the node.
     */
    width: number;

    /**
     * height of the node.
     */
    height: number;
  }>;
}

interface SetFocusMethod extends BaseMethod {
  method: 'setFocus';
  params: {
    /**  Set whether the element gains focus, `false` means the element loses focus.*/
    focus: boolean;
    /**   Whether to scroll the element into the visible area at the same time, default is `true`.*/
    scroll?: boolean;
  };
}

interface IsAnimatingMethod extends BaseMethod {
  method: 'isAnimating';
  success?: Callback<{ data: boolean }>;
}

interface ScrollIntoViewMethod extends BaseMethod {
  method: 'scrollIntoView';
  params: {
    scrollIntoViewOptions: {
      block: 'start' | 'center' | 'end';
      inline: 'start' | 'center' | 'end';
      behavior?: 'smooth';
    };
  };
}

export type InvokeParams = ListParams | ScrollViewParams | BoundingClientRectMethod | SetFocusMethod | IsAnimatingMethod | ScrollIntoViewMethod;
