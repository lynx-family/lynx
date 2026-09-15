// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { BaseEvent, BaseMethod, Callback } from '../events';
import { StandardProps } from '../props';

export interface MarkdownAnimationStepEvent {
  /**
   * Current step of animation.
   */
  animationStep: number;

  /**
   * Maximum step of animation.
   */
  maxAnimationStep: number;
}

export interface MarkdownOverflowEvent {
  /**
   * Type of overflow.
   * ellipsis: overflow text will be replaced by ... or truncation view.
   * clip: overflow text will be clipped.
   */
  type: 'ellipsis' | 'clip';
}

export interface MarkdownLinkEvent {
  /**
   * Link url.
   */
  url: string;

  /**
   * Link text.
   */
  content: string;
}

export interface MarkdownImageTapEvent {
  /**
   * Image url.
   */
  url: string;
}

export interface MarkdownTextClickEvent {
  /**
   * ID of the clicked text attachment.
   */
  id: string;
}

export interface MarkdownParseEndEvent {
  /**
   * The id of content that parsed.
   */
  id: string;
}

export interface MarkdownSelectionChangeEvent {
  /**
   * Start char pos of selection range.
   */
  start: number;

  /**
   * End char pos of selection range.
   */
  end: number;

  /**
   * Direction of selection.
   */
  direction: 'forward' | 'backward';
}

/** Styling for one border of a text attachment. */
export interface MarkdownTextAttachmentBorder {
  /**
   * Border line style accepted by the Markdown parser.
   * Attachment borders currently render solid and dashed lines.
   * @defaultValue 'none'
   */
  lineType?: 'none' | 'solid' | 'double' | 'dotted' | 'dashed' | 'wavy';
  /**
   * Border width, as a native length number or string such as '2px'.
   * @defaultValue 0
   */
  width?: number | string;
  /**
   * Hexadecimal color or a supported linear-gradient()/radial-gradient() string.
   * @defaultValue '#00000000' (transparent)
   */
  color?: string;
  /**
   * Length of each dash in a dashed border.
   * @defaultValue 2.5 density-independent pixels
   */
  elementSize?: number | string;
  /**
   * Requested gap between dashes; spacing may be adjusted to fit the border.
   * @defaultValue 1.5 density-independent pixels
   */
  emptySize?: number | string;
}
/** Background and border decoration for the rectangle covering a text range. */
export interface MarkdownTextAttachmentStyle {
  /**
   * Left edge offset from the text range's left edge.
   * @defaultValue 0
   */
  left?: number | string;
  /**
   * Top edge offset from the text range's top edge.
   * @defaultValue 0
   */
  top?: number | string;
  /**
   * Right edge offset from the text range's left edge, not a right inset.
   * @defaultValue '100%' (the text range's width)
   */
  right?: number | string;
  /**
   * Bottom edge offset from the text range's top edge, not a bottom inset.
   * @defaultValue '100%' (the text range's height)
   */
  bottom?: number | string;
  /**
   * Corner radius of the background rectangle.
   * @defaultValue 0
   */
  radius?: number | string;
  /**
   * Hexadecimal fill color or a supported linear-gradient()/radial-gradient() string.
   * @defaultValue '#00000000' (transparent)
   */
  color?: string;
  /**
   * Left border decoration.
   * @defaultValue {} (no border)
   */
  borderLeft?: MarkdownTextAttachmentBorder;
  /**
   * Top border decoration.
   * @defaultValue {} (no border)
   */
  borderTop?: MarkdownTextAttachmentBorder;
  /**
   * Right border decoration.
   * @defaultValue {} (no border)
   */
  borderRight?: MarkdownTextAttachmentBorder;
  /**
   * Bottom border decoration.
   * @defaultValue {} (no border)
   */
  borderBottom?: MarkdownTextAttachmentBorder;
}
/** Decoration and optional click target for the text range [startIndex, endIndex). */
export interface MarkdownTextAttachment {
  /**
   * Inclusive start index in the text selected by indexType.
   * @defaultValue 0
   */
  startIndex: number;
  /**
   * Exclusive end index in the text selected by indexType.
   * @defaultValue 0
   */
  endIndex: number;
  /**
   * 'char' indexes parsed text; 'source' indexes the original Markdown source.
   * @defaultValue 'char'
   */
  indexType?: 'char' | 'source';
  /**
   * Whether the decoration is drawn behind or in front of the text.
   * @defaultValue 'background'
   */
  layer?: 'background' | 'foreground';
  /**
   * Identifier returned by the textClick event when this attachment is clicked.
   * @defaultValue ''
   */
  id?: string;
  /**
   * Whether the text range can emit a textClick event.
   * @defaultValue false
   */
  clickable?: boolean;
  /**
   * Background and border decoration for the text range.
   * @defaultValue {} (no decoration)
   */
  style?: MarkdownTextAttachmentStyle;
}
export interface MarkdownProps extends StandardProps {
  /**
   * Decorations and optional click targets attached to text ranges.
   * Clay mobile support in 4.3 requires the Skity rendering backend.
   * @Android 4.0
   * @iOS 4.0
   * @ClayAndroid 4.3
   * @ClayIOS 3.4
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @defaultValue []
   */
  'text-mark-attachments'?: MarkdownTextAttachment[];
  /**
   * Markdown tags whose visibility should be tracked, such as 'link'.
   * @Android 4.0
   * @iOS 4.0
   * @defaultValue [] (no exposure tracking)
   */
  'exposure-tags'?: string[];
  /**
   * Maximum layout height of the markdown content, in platform logical units.
   * @Android 4.0
   * @iOS 4.0
   * @defaultValue 0 (no additional height limit)
   */
  'markdown-max-height'?: number;
  /**
   * Text selection highlight color, encoded as a 32-bit ARGB number.
   * Clay Android and iOS require the Skity rendering backend in 4.3 prereleases.
   * Desktop versions refer to the earliest verified prerelease builds.
   * @Android 4.0
   * @iOS 4.0
   * @ClayAndroid 4.3
   * @ClayIOS 4.3
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  'selection-background-color'?: number;
  /**
   * Text selection handle color, encoded as a 32-bit ARGB number.
   * Clay Android and iOS require the Skity rendering backend in 4.3 prereleases.
   * Desktop versions refer to the earliest verified prerelease builds.
   * @Android 4.0
   * @iOS 4.0
   * @ClayAndroid 4.3
   * @ClayIOS 4.3
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  'selection-handle-color'?: number;
  /**
   * Text selection handle size. Android uses physical pixels; iOS uses points.
   * Clay Android and iOS require the Skity rendering backend in 4.3 prereleases.
   * Desktop versions refer to the earliest verified prerelease builds.
   * @Android 4.0
   * @iOS 4.0
   * @ClayAndroid 4.3
   * @ClayIOS 4.3
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  'selection-handle-size'?: number;
  /**
   * Prefetches the next content height for typewriter height transitions.
   * Removing this attribute resets it to false on Android and iOS.
   * @Android 4.0
   * @iOS 4.0
   * @defaultValue true
   */
  'typewriter-height-transition-prefetch'?: boolean;
  /**
   * Markdown source content.
   * @Android 2.15
   * @iOS 2.16
   * @Harmony 3.1
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   */
  content: string;

  /**
   * Set the style of markdown.
   * @Android 2.15
   * @iOS 2.16
   * @Harmony 3.1
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   */
  'markdown-style'?: object;

  /**
   * Which animation to use.
   * none: no animation.
   * typewriter: typewriter animation.
   * line-expand: line expand animation.
   * @Android 2.15
   * @iOS 2.16
   * @Harmony 3.1
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   * @defaultValue 'none'
   */
  'animation-type'?: 'none' | 'typewriter' | 'line-expand';

  /**
   * Velocity of animation.
   * When animation-type is typewriter, it is the number of characters typed per second.
   * @Android 2.15
   * @iOS 2.16
   * @Harmony 3.1
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   * @defaultValue 1
   */
  'animation-velocity'?: number;

  /**
   * Maximum number of text lines.
   * A table row is counted as a line.
   * @Android 2.15
   * @iOS 2.16
   * @Harmony 3.1
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   * @defaultValue -1 (no limit)
   */
  'text-maxline'?: number;

  /**
   * Animation will start at the given step.
   * @Android 2.17
   * @iOS 2.18
   * @Harmony 3.1
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue 1
   */
  'initial-animation-step'?: number;

  /**
   * true: typewriter cursor will be hidden and drawEnd event will be triggered after the animation is complete.
   * false: typewriter cursor will be shown and drawEnd event will not be triggered.
   * @Android 2.17
   * @iOS 3.1
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue true
   */
  'content-complete'?: boolean;

  /**
   * Whether markdown height is equal to the content height typed by typewriter.
   * @Android 2.17
   * @iOS 2.18
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue false
   */
  'typewriter-dynamic-height'?: boolean;

  /**
   * Whether markdown content is selectable.
   * @Android 2.17
   * @iOS 2.18
   * @Harmony 3.1
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue false
   */
  'text-selection'?: boolean;

  /**
   * Alias of text-selection.
   * @Harmony 4.0
   * @defaultValue false
   */
  'enable-selection'?: boolean;

  /**
   * Whether text can break around punctuation.
   * @Android 3.2
   * @iOS 3.2
   * @Harmony 4.0
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue false
   */
  'allow-break-around-punctuation'?: boolean;

  /**
   * Alias of allow-break-around-punctuation.
   * @Harmony 4.0
   * @defaultValue false
   */
  'enable-break-around-punctuation'?: boolean;

  /**
   * The range of content to be parsed.
   * [start, end]
   * @Android 2.18
   * @iOS 3.1
   * @Harmony 3.1
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue [0, int32_max]
   */
  'content-range'?: number[];

  /**
   * Additional effects.
   * @Android 3.1
   * @iOS 3.2
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  'markdown-effect'?: object;

  /**
   * Mark the content when the event or UI method is triggered.
   * @Android 3.1
   * @iOS 3.2
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  'content-id'?: string;

  /**
   * Height transition duration. Disable height transition if value less than or equal to 0.
   * @Android 3.2
   * @iOS 3.2
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue 0
   */
  'typewriter-height-transition-duration'?: number;

  /**
   * Control animation frame rate to improve performance.
   * @Android 3.2
   * @iOS 3.2
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @defaultValue 15
   */
  'animation-frame-rate'?: number;

  /**
   * Draw start.
   * @Android 2.15
   * @iOS 2.16
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   */
  binddrawStart?: (e: BaseEvent) => void;

  /**
   * Draw end.
   * @Android 2.15
   * @iOS 2.16
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   */
  binddrawEnd?: (e: BaseEvent) => void;

  /**
   * Callback after each animation frame. Some animation steps may be skipped if animation velocity is too fast.
   * @Android 2.17
   * @iOS 2.18
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  bindanimationStep?: (e: BaseEvent<'bindanimationStep', MarkdownAnimationStepEvent>) => void;

  /**
   * Callback when text overflows.
   * @Android 2.16
   * @iOS 4.0
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   */
  bindoverflow?: (e: BaseEvent<'bindoverflow', MarkdownOverflowEvent>) => void;

  /**
   * Callback when link is clicked.
   * @Android 2.15
   * @iOS 2.16
   * @ClayMacOS 2.18
   * @ClayWindows 2.18
   */
  bindlink?: (e: BaseEvent<'bindlink', MarkdownLinkEvent>) => void;

  /**
   * Callback when selection changes.
   * @Android 2.17
   * @iOS 3.1
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  bindselectionchange?: (e: BaseEvent<'bindselectionchange', MarkdownSelectionChangeEvent>) => void;

  /**
   * Callback when image is clicked.
   * @Android 3.1
   * @iOS 3.2
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  bindimageTap?: (e: BaseEvent<'bindimageTap', MarkdownImageTapEvent>) => void;

  /**
   * Callback when a text attachment is clicked.
   */
  bindtextClick?: (e: BaseEvent<'bindtextClick', MarkdownTextClickEvent>) => void;

  /**
   * Callback when parse ends.
   * @Android 3.1
   * @iOS 3.2
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   */
  bindparseEnd?: (e: BaseEvent<'bindparseEnd', MarkdownParseEndEvent>) => void;
}

/**
 * @Android 2.17
 * @iOS 3.1
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 */
export interface MarkdownPauseAnimationMethod extends BaseMethod {
  method: 'pauseAnimation';
  success?: Callback<{
    /**
     * Current step of animation.
     */
    animationStep: number;
  }>;
}

/**
 * @Android 2.17
 * @iOS 3.1
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 */
export interface MarkdownResumeAnimationMethod extends BaseMethod {
  method: 'resumeAnimation';
  params?: {
    /**
     * Animation will start at the given step.
     */
    animationStep?: number;
  };
}

/**
 * Get parsed content.
 * @Android 2.17
 * @iOS 2.18
 * @Harmony 3.1
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 */
export interface MarkdownGetContentMethod extends BaseMethod {
  method: 'getContent';
  params: {
    /**
     * Start index of parsed content.
     * @defaultValue 0
     */
    start?: number;

    /**
     * End index of parsed content.
     * @defaultValue int32_max
     */
    end?: number;
  };
  success?: Callback<{
    /**
     * Parsed content of the given range.
     */
    content: string;
  }>;
}

/**
 * @Android 2.17
 * @iOS 2.18
 * @Harmony 3.1
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 */
export interface MarkdownGetSelectedTextMethod extends BaseMethod {
  method: 'getSelectedText';
  success?: Callback<{
    /**
     * Selected text.
     */
    selectedText: string;
  }>;
}

/**
 * Control text selection highlighting.
 * @Android 2.17
 * @iOS 2.18
 * @Harmony 3.1
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 */
export interface MarkdownSetTextSelectionMethod extends BaseMethod {
  method: 'setTextSelection';
  params: {
    /**
     * The x-coordinate of the start of the selected text relative to the text component.
     */
    startX: number;

    /**
     * The y-coordinate of the start of the selected text relative to the text component.
     */
    startY: number;

    /**
     * The x-coordinate of the end of the selected text relative to the text component.
     */
    endX: number;

    /**
     * The y-coordinate of the end of the selected text relative to the text component.
     */
    endY: number;
  };
  success?: Callback<{
    /**
     * The bounding box of the selected text.
     */
    boundingRect: {
      left: number;
      top: number;
      right: number;
      bottom: number;
      width: number;
      height: number;
    };

    /**
     * The bounding boxes of each line.
     */
    boxes: {
      left: number;
      top: number;
      right: number;
      bottom: number;
      width: number;
      height: number;
    }[];

    /**
     * The cursor positions and the default radius.
     */
    handles: {
      x: number;
      y: number;
      radius: number;
    }[];
  }>;
}

/**
 * Get index of the character at the given position.
 * @Android 4.0
 * @iOS 4.0
 */
export interface MarkdownGetCharIndexByPointMethod extends BaseMethod {
  method: 'getCharIndexByPoint';
  params: {
    /**
     * x-coordinate of the point.
     */
    x: number;

    /**
     * y-coordinate of the point.
     */
    y: number;

    /**
     * Index type of the point.
     * @defaultValue null, set to source to get index of source content.
     */
    indexType?: string;
  };
  success?: Callback<{
    /**
     * Index of the character at the point.
     */
    index: number;
  }>;
}

/**
 * Get all images in the markdown.
 * @Android 3.1
 * @iOS 3.2
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 */
export interface MarkdownGetImagesMethod extends BaseMethod {
  method: 'getImages';
  success?: Callback<{
    /**
     * All images in the markdown.
     */
    images: string[];
  }>;
}

/**
 * Get source ranges of the given tags.
 * @Android 3.1
 * @iOS 3.2
 * @Harmony 4.0
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 */
export interface MarkdownGetParseResultMethod extends BaseMethod {
  method: 'getParseResult';
  params: {
    /**
     * Tags whose source ranges should be returned.
     */
    tags: string[];
  };
  success?: Callback<{
    /**
     * Source ranges of the given tags.
     */
    result: Map<
      string,
      {
        start: number;
        end: number;
      }[]
    >;
  }>;
}

export type MarkdownUIMethods =
  | MarkdownPauseAnimationMethod
  | MarkdownResumeAnimationMethod
  | MarkdownGetContentMethod
  | MarkdownGetSelectedTextMethod
  | MarkdownSetTextSelectionMethod
  | MarkdownGetCharIndexByPointMethod
  | MarkdownGetImagesMethod
  | MarkdownGetParseResultMethod;
