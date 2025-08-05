import { BaseEvent, BaseMethod, Callback } from '../events';
import { StandardProps } from '../props';


export interface TextAreaInputEvent {
  /**
   * Input content
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  value: string;
  /**
   * The start position of the selection
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  selectionStart: number;
  /**
   * The end position of the selection
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  selectionEnd: number;
  /**
   * Is composing or not
   * @iOS
   * @Web
   * @Android
   * @Harmony
   * @since 3.4
   */
  isComposing?: boolean;
}

export interface TextAreaFocusEvent {
  /**
   * Input content
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  value: string;
}

export interface TextAreaBlurEvent {
  /**
   * Input content
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  value: string;
}

export interface TextAreaConfirmEvent {
  /**
   * Input content
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  value: string;
}

export interface TextAreaSelectionChangeEvent {
  /**
   * The start position of the selection
   * @Android
   * @iOS
   * @Harmony
     * @Web
   * @since 3.4
   */
  selectionStart: number;
  /**
   * The end position of the selection
   * @Android
   * @iOS
   * @Harmony
     * @Web
   * @since 3.4
   */
  selectionEnd: number;
}

export interface TextAreaProps extends Omit<StandardProps, 'bindfocus' | 'bindblur'> {
  /**
   * Placeholder
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  placeholder?: string
  /**
   * The type of confirm button
   * @defaultValue 'done'
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  'confirm-type'?: 'send' | 'search' | 'go' | 'done' | 'next';
  /**
   * Max input length
   * @defaultValue 140
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  maxlength?: number;
  /**
   * Max input lines
   * @defaultValue undefined
   * @Android
   * @iOS
   * @Harmony
   * @since 3.4
   */
  maxlines?: number;
  /**
   * Bounce effect for iOS
   * @defaultValue true
   * @iOS
   * @since 3.4
   */
  bounces?: boolean;
  /**
   * Line spacing
   * @defaultValue undefined
   * @iOS
   * @Android
   * @since 3.4
   */
  'line-spacing'?: number | `${number}px` | `${number}rpx`;
  /**
   * Readonly
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  readonly?: boolean;

  /**
   * Interaction enabled
   * @defaultValue false
   * @Android
   * @iOS
   * @Harmony
   * @since 3.5
   */
  disabled?: boolean;

  /**
   * Show soft input keyboard while focused
   * @defaultValue true
   * @Android
   * @iOS
   * @since 3.4
   */
  'show-soft-input-on-focus'?: boolean;

  /**
   * Filter the input content and process it in the form of regular expressions
   * @defaultValue undefined
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  'input-filter'?: string;

  /**
   * Input content type
   * @defaultValue "text"
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  type?: 'text' | 'number' | 'digit' | 'tel' | 'email';

  /**
   * Auto correct input content on iOS
   * @defaultValue true
   * @iOS
   * @since 3.4
   */
  'ios-auto-correct'?: boolean;

  /**
   * Check spelling issue on iOS
   * @defaultValue true
   * @iOS
   * @Web
   * @since 3.4
   */
  'ios-spell-check'?: boolean;

  /**
   * Whether to enter the full-screen input mode when in landscape screen, in which the keyboard and input box will take up the entire screen
   * @defaultValue true
   * @Android
   * @since 3.4
   */
  'android-fullscreen-mode'?: boolean;

  /**
   * Focused
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  bindfocus?: (e: BaseEvent<'bindfocus', TextAreaFocusEvent>) => void;

  /**
   * Blurred
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  bindblur?: (e: BaseEvent<'bindblur', TextAreaBlurEvent>) => void;

  /**
   * Input content changed
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  bindinput?: (e: BaseEvent<'bindinput', TextAreaInputEvent>) => void;

  /**
   * Input selection changed
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  bindselection?: (e: BaseEvent<'bindselection', TextAreaSelectionChangeEvent>) => void;

  /**
   * Confirm button clicked, only work when confirm-type is defined
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @since 3.4
   */
  bindconfirm?: (e: BaseEvent<'bindconfirm', TextAreaConfirmEvent>) => void;
}

/**
 * Require focus
 * @Android
 * @iOS
 * @Harmony
 * @Web
 * @since 3.4
 */
export interface TextAreaFocusMethod extends BaseMethod {
  method: 'focus';
}

/**
 * Release focus
 * @Android
 * @iOS
 * @Harmony
 * @Web
 * @since 3.4
 */
export interface TextAreaBlurMethod extends BaseMethod {
  method: 'blur';
}

/**
 * Get input content
 * @Android
 * @iOS
 * @Harmony
 * @Web
 * @since 3.4
 */
export interface TextAreaGetValueMethod extends BaseMethod {
  method: 'getValue';
  success?: Callback<{
    /**
     * Input content
     * @Android
     * @iOS
     * @Harmony
     * @Web
     * @since 3.4
     */
    value: string;
    /**
     * Begin position of the cursor
     * @Android
     * @iOS
     * @Harmony
     * @Web
     * @since 3.4
     */
    selectionStart: number;
    /**
     * End position of the cursor
     * @Android
     * @iOS
     * @Harmony
     * @Web
     * @since 3.4
     */
    selectionEnd: number;
    /**
     * Is composing or not, iOS only
     * @iOS
     * @Android
     * @Harmony
     * @Web
     * @since 3.4
     */
    isComposing: boolean;
  }>;
}

/**
 * Set input content
 * @Android
 * @iOS
 * @Harmony
 * @Web
 * @since 3.4
 */
export interface TextAreaSetValueMethod extends BaseMethod {
  method: 'setValue';
  params: {
    /**
     * Input content
     * @Android
     * @iOS
     * @Harmony
     * @Web
     * @since 3.4
     */
    value: string;
  };
}

/**
 * Set selection range
 * @Android
 * @iOS
 * @Harmony
 * @Web
 * @since 3.4
 */
export interface TextAreaSetSelectionRangeMethod extends BaseMethod {
  method: 'setSelectionRange';
  params: {
    /**
     * Start position of the selection
     * @Android
     * @iOS
     * @Harmony
     * @Web
     * @since 3.4
     */
    selectionStart: number;
    /**
     * End position of the selection
     * @Android
     * @iOS
     * @Harmony
     * @Web
     * @since 3.4
     */
    selectionEnd: number;
  };
}

export type TextAreaUIMethods = TextAreaFocusMethod | TextAreaBlurMethod | TextAreaGetValueMethod | TextAreaSetValueMethod | TextAreaSetSelectionRangeMethod;
