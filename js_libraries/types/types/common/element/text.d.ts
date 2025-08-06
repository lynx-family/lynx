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
   * @Android
   * @iOS
   * @Harmony
   * @PC
   * @since 1.0
   */
  'text-maxline'?: string;

  /**
   * Maximum number of characters for text display
   * @since 1.0
   * @deprecated Suggest preprocessing the text content length.
   */
  'text-maxlength'?: string;

  /**
   * Whether font-size is affected by system font scaling
   * @defaultValue false
   * @deprecated
   * @since 1.6
   */
  'enable-font-scaling'?: boolean;

  /**
   * Baseline adjustment strategy in vertical direction; note: setting this value does not guarantee text centering
   * @defaultValue ""
   * @since 1.4
   * @deprecated Use the text-single-line-vertical-align attribute instead.
   */
  'text-vertical-align'?: 'bottom' | 'center' | 'top';

  /**
   * By default, if text truncation occurs, the color of the inserted ... will be specified by the style on the nearest inline-text. If this attribute is enabled, the color of ... will be specified by the style on the outermost text tag.
   * @defaultValue false
   * @Android
   * @iOS
   * @since 2.0
   */
  'tail-color-convert'?: boolean;

  /**
   * Set single-line plain text to be centered and aligned within the line. Inline text settings are not supported. Recommended only when the default font doesn't meet center alignment needs, as it increases text measurement time.
   * @defaultValue 'normal'
   * @iOS
   * @Android
   * @PC
   * @since 2.12
   */
  'text-single-line-vertical-align'?: 'normal' | 'bottom' | 'center' | 'top';

  /**
   * Enable additional spacing above and below the text on Android; recommended only in high language scenarios to avoid text truncation.
   * @defaultValue false
   * @Android
   * @since 1.0
   */
  'include-font-padding'?: boolean;

  /**
   * Enable support for Emoji2 adaptation; requires androidx.emoji2 dependency.
   * @defaultValue false
   * @Android
   * @since 2.9
   */
  'android-emoji-compat'?: boolean;

  /**
   * Enables fake bold for fonts when the default bold is not found.
   * @defaultValue false
   * @Android
   * @iOS
   * @since 2.13
   */
  'text-fake-bold'?: boolean;

  /**
   * Sets whether to enable text selection.
   * @defaultValue false
   * @Android
   * @iOS
   * @since 2.18
   */
  'text-selection'?: boolean;

  /**
   * Used to set whether to turn on the custom pop-up context menu after selection and copying. It takes effect after enabling text-selection.
   * @defaultValue false
   * @Android
   * @iOS
   * @since 2.18
   */
  'custom-context-menu'?: boolean;

  /**
   * Used to set whether to enable the custom text selection function. When it is enabled, the element will no longer handle the gesture logic related to selection and copying. It takes effect after enabling text-selection.
   * @defaultValue false
   * @Android
   * @iOS
   * @since 2.18
   */
  'custom-text-selection'?: boolean;

  /**
   * Text layout event
   * @since 2.7
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  bindlayout?: (e: LayoutEvent) => void;

  /**
   * Text selection change event
   * @Android
   * @iOS
   * @since 2.18
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
 * @Android
 * @iOS
 * @since 2.18
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
 * @Android
 * @iOS
 * @PC
 * @since 2.18
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
 * @Android
 * @iOS
 * @since 2.18
 */
interface GetSelectedTextMethod extends BaseMethod {
  method: 'getSelectedText';
  success?: Callback<{
    selectedText: string;
  }>;
}

export type TextUIMethods = SetTextSelectionMethod | GetTextBoundingRectMethod | GetSelectedTextMethod;
