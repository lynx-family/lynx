// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, Callback, BaseMethod, TextLayoutEventDetail, TextSelectionChangeEventDetail } from '../events';
import { StandardProps } from '../props';

/**
 * Text Component
 */
export interface TextProps extends StandardProps {
  /**
   * Maximum number of lines for text display
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 2.18
   */
  'text-maxline'?: string;

  /**
   * Maximum number of characters for text display
   * @Android 1.4
   * @iOS 2.18
   * @Web
   * @ClayAndroid 3.1
   * @ClayIOS 2.8
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   * @ClayHarmony 2.18
   * @deprecated Suggest preprocessing the text content length.
   */
  'text-maxlength'?: string;

  /**
   * Whether font-size is affected by system font scaling
   * @Android 0.1
   * @iOS 1.5
   * @defaultValue false
   * @deprecated
   */
  'enable-font-scaling'?: boolean;

  /**
   * Baseline adjustment strategy in vertical direction; note: setting this value does not guarantee text centering
   * @Android 1.4
   * @iOS 2.18
   * @defaultValue ""
   * @deprecated Use the text-single-line-vertical-align attribute instead.
   */
  'text-vertical-align'?: 'bottom' | 'center' | 'top';

  /**
   * By default, if text truncation occurs, the color of the inserted ... will be specified by the style on the nearest inline-text. If this attribute is enabled, the color of ... will be specified by the style on the outermost text tag.
   * @Android 1.5
   * @iOS 1.5
   * @Web
   * @defaultValue false
   */
  'tail-color-convert'?: boolean;

  /**
   * Set single-line plain text to be centered and aligned within the line. Inline text settings are not supported. Recommended only when the default font doesn't meet center alignment needs, as it increases text measurement time.
   * @Android 1.5
   * @iOS 1.5
   * @ClayAndroid 1.5
   * @ClayIOS 1.5
   * @defaultValue 'normal'
   */
  'text-single-line-vertical-align'?: 'normal' | 'bottom' | 'center' | 'top';

  /**
   * Enable additional spacing above and below the text on Android; recommended only in high language scenarios to avoid text truncation.
   * @Android 1.5
   * @defaultValue false
   */
  'include-font-padding'?: boolean;

  /**
   * Enable support for Emoji2 adaptation; requires androidx.emoji2 dependency.
   * @Android 2.10
   * @defaultValue false
   */
  'android-emoji-compat'?: boolean;

  /**
   * Enables fake bold for fonts when the default bold is not found.
   * @Android 2.12
   * @iOS 2.18
   * @defaultValue false
   */
  'text-fake-bold'?: boolean;

  /**
   * Sets whether to enable text selection.
   * @Android 3.2
   * @iOS 3.2
   * @Harmony 4.0
   * @Web
   * @ClayAndroid 3.7
   * @ClayIOS 3.7
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   * @defaultValue false
   */
  'text-selection'?: boolean;

  /**
   * Used to set whether to turn on the custom pop-up context menu after selection and copying. It takes effect after enabling text-selection.
   * @Android 3.2
   * @iOS 3.2
   * @Harmony 4.0
   * @Clay 4.0
   * @defaultValue false
   */
  'custom-context-menu'?: boolean;

  /**
   * Used to set whether to enable the custom text selection function. When it is enabled, the element will no longer handle the gesture logic related to selection and copying. It takes effect after enabling text-selection.
   * @Android 3.2
   * @iOS 3.2
   * @Harmony 4.0
   * @Clay 4.0
   * @defaultValue false
   */
  'custom-text-selection'?: boolean;

  /**
   * Text layout event
   * @Android 0.1
   * @iOS 2.18
   * @Harmony 2.16
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.8
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   */
  bindlayout?: (e: LayoutEvent) => void;

  /**
   * Text selection change event
   * @Android 2.17
   * @iOS 3.1
   * @Harmony 4.0
   * @ClayAndroid 3.1
   * @ClayIOS 3.1
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 4.0
   */
  bindselectionchange?: (e: SelectionChangeEvent) => void;
}

export type LayoutEvent = BaseEvent<'layout', TextLayoutEventDetail>;

export type SelectionChangeEvent = BaseEvent<
  'selectionchange',
  TextSelectionChangeEventDetail
>;

interface Rect {
  left: number;
  right: number;
  top: number;
  bottom: number;
  width: number;
  height: number;
}

interface Handle {
  /**
   * Center X of handle
   */
  x: number;
  /**
   * Center Y of handle
   */
  y: number;
  /**
   * Touch radius of the handle
   */
  radius: number;
}

/**
 * Sets the text selection.
 * @Android 3.2
 * @iOS 3.2
 * @Harmony 4.0
 * @Clay 4.0
 */
interface SetTextSelectionMethod extends BaseMethod {
  method: 'setTextSelection';
  params: {
    /**
     *  X-coordinate of the selection start relative to the element
     */
    startX: number;
    /**
     *  Y-coordinate of the selection start relative to the element
     */
    startY: number;
    /**
     *  X-coordinate of the selection end relative to the element
     */
    endX: number;
    /**
     *  Y-coordinate of the selection end relative to the element
     */
    endY: number;
    /**
     * Whether to show the start handle, default is true
     */
    showStartHandle?: boolean;
    /**
     * Whether to show the end handle, default is true
     */
    showEndHandle?: boolean;
  };
  success?: Callback<{
    /**
     * Bounding rectangle of the selected text
     */
    boundingRect: Rect;
    /**
     * Rectangles of the selected text
     */
    boxes: Rect[];
    /**
     * Handles of the selected text
     */
    handles: Handle[]
  }>;
}

/**
 * Gets the bounding rectangle of the text.
 * @Android 3.2
 * @iOS 3.2
 * @Harmony 4.0
 * @ClayAndroid 1.5
 * @ClayIOS 1.5
 * @ClayMacOS 1.5
 * @ClayWindows 1.5
 * @ClayHarmony 2.17
 */
interface GetTextBoundingRectMethod extends BaseMethod {
  method: 'getTextBoundingRect';
  params: {
    /**
     * Start index of the text
     */
    start: number;
    /**
     * End index of the text
     */
    end: number;
  };
  success?: Callback<{
    /**
     * Bounding rectangle of the text
     */
    boundingRect: Rect;
    /**
     * Rectangles of the text
     */
    boxes: Rect[];
  }>;
}

/**
 * Gets the selected text.
 * @Android 3.2
 * @iOS 3.2
 * @Harmony 4.0
 * @Clay 4.0
 */
interface GetSelectedTextMethod extends BaseMethod {
  method: 'getSelectedText';
  success?: Callback<{
    selectedText: string;
  }>;
}

export type TextUIMethods = SetTextSelectionMethod | GetTextBoundingRectMethod | GetSelectedTextMethod;
