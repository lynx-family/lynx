// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, TextLayoutEventDetail } from '../events';
import { StandardProps } from '../props';

/**
 * Text Component
 */
export interface TextProps extends StandardProps {
  /**
   * Maximum number of lines for text display
   * @defaultValue "-1"
   * @since 1.0
   */
  'text-maxline'?: string;

  /**
   * Maximum number of characters for text display
   * @defaultValue ""
   * @since 1.0
   * @deprecated Suggest preprocessing the text content length.
   */
  'text-maxlength'?: string;

  /**
   * Whether font-size is affected by system font scaling
   * @defaultValue false
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
   * @since 2.0
   */
  'tail-color-convert'?: boolean;

  /**
   * Enable long press to select text
   * @defaultValue false
   * @since 2.5
   */
  'text-selection'?: boolean;

  /**
   * Set single-line plain text to be centered and aligned within the line. Inline text settings are not supported. Recommended only when the default font doesn't meet center alignment needs, as it increases text measurement time.
   * @defaultValue normal
   * @iOS
   * @Android
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
   * Text layout event
   * @since 2.7
   */
  bindlayout?: (e: LayoutEvent) => void;
}

export type LayoutEvent = BaseEvent<'layout', TextLayoutEventDetail>;
