// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';
import { BaseEvent } from '../events';

export enum OverlayTouchState {
  OverlayTouchStateDown = 0,
  OverlayTouchStateMove = 1,
  OverlayTouchStateUp = 2,
  OverlayTouchStateCancel = 3,
}

export interface OverlayError {
  /**
   * Error code: 0 - Display normally; non-0 - Unable to display, you can try to solve it by using the preloading scheme mentioned below for adapting containers.
   * @Android
   */
  errorCode: string;
  /**
   * Error message
   * @Android
   */
  errorMsg: string;
}

export interface OverlayTouch {
  /**
   * x position relative to the window, in px
   * @Android
   * @iOS
   * @PC
   */
  x: number;
  /**
   * y position relative to the window, in px
   * @Android
   * @iOS
   * @PC
   */
  y: number;
  /**
   * Touch state
   * @Android
   * @iOS
   * @PC
   */
  state: OverlayTouchState;
}

export interface OverlayProps extends Omit<StandardProps, 'binderror'> {
  
  /**
   * Control whether the overlay is displayed
   * @Android 3.5
   * @iOS 3.5
   * @Harmony 3.5
   * @ClayAndroid 3.5
   * @ClayIOS 3.5
   * @ClayMacOS 3.5
   * @ClayWindows 3.5
   * @ClayHarmony 3.6
   * @defaultValue false
   */
  visible?: boolean;

  /**
   * Sets the Android navigation bar background and contrasting system-bar appearance. `auto` inherits the host Activity navigation bar.
   * When set to `transparent`, `android-adapt-edge-to-edge` must also be set to `true`.
   * @Android 4.1
   * @defaultValue 'auto'
   */
  'android-navigation-bar-style'?: 'auto' | 'light' | 'dark' | 'transparent';

  /**
   * Scopes Android overlay visibility to the Fragment that owns its LynxView.
   * @Android 4.3
   * @defaultValue 'global'
   */
  'android-overlay-scope'?: 'global' | 'fragment';

  /**
   * Introduces the concept of layers, which are divided into four levels. The larger the layer, the closer it is to the bottom. By default, it is the first level. The layers are arranged in order from 1 to 4. The displayed layer is specified and is not affected by the order of display. Within each layer, the arrangement is based on the 'last in, first out' logic. The layer cannot be dynamically adjusted when the overlay is displayed, and can only be adjusted when it is hidden.     * @Android
   * @Android 3.5
   * @iOS 3.5
   * @Harmony 3.5
   * @ClayAndroid 3.5
   * @ClayIOS 3.5
   * @ClayMacOS 3.5
   * @ClayWindows 3.5
   * @defaultValue 1
   */
  level?: 1 | 2 | 3 | 4;

  /**
   * When overlay is displayed, 'true' allows swiping right to close the current page, 'false' does not allow it
   * @iOS 3.5
   * @ClayIOS 3.8
   * @defaultValue false
   */
  'ios-enable-swipe-back'?: boolean;

  /**
   * Specifies the level at which overlay content resides. On iOS, window mounts on the app window, top mounts on the topViewController, page mounts on UINavigationController, and other strings customize the client class name. On Harmony native overlay, page embeds in the current page; window, top, missing, and other strings use the default window-like overlay level.
   * @iOS 3.5
   * @Harmony 4.0
   * @ClayIOS 3.8
   * @ClayHarmony 4.0
   * @defaultValue 'window'
   */
  mode?: 'window' | 'top' | 'page' | string;
  
  /**
   * Callback when the overlay is displayed
   * @Android 3.5
   * @iOS 3.5
   * @Harmony 3.5
   * @ClayAndroid 3.5
   * @ClayIOS 3.5
   * @ClayMacOS 3.5
   * @ClayWindows 3.5
   * @ClayHarmony 3.6
   */
  bindshowoverlay?: (e: BaseEvent) => void;

  /**
   * Callback when the overlay is hidden.
   * @Android 3.5
   * @iOS 3.5
   * @Harmony 3.5
   * @ClayAndroid 3.5
   * @ClayIOS 3.5
   * @ClayMacOS 3.5
   * @ClayWindows 3.5
   * @ClayHarmony 3.6
   */
  binddismissoverlay?: (e: BaseEvent) => void;

  /**
   * Callback when the back button is clicked.
   * @Android 3.5
   * @Harmony 3.5
   * @ClayAndroid 3.5
   * @ClayMacOS 3.5
   * @ClayWindows 3.5
   */
  bindrequestclose?: (e: BaseEvent) => void;

  /**
   * Callback when touch on the overlay
   * @Android 3.5
   * @iOS 3.5
   * @Harmony 4.0
   * @ClayAndroid 3.5
   * @ClayIOS 3.5
   * @ClayMacOS 3.5
   * @ClayWindows 3.5
   * @ClayHarmony 4.0
   */
  bindoverlaytouch?: (e: OverlayTouchEvent) => void;

  /**
   * Callback when touch on the overlay
   */
  binderror?: (e: OverlayErrorEvent) => void;
}

export type OverlayTouchEvent = BaseEvent<'bindoverlaytouch', OverlayTouch>;
export type OverlayErrorEvent = BaseEvent<'binderror', OverlayError>;
