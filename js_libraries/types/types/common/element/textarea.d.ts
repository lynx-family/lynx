import { BaseEvent, BaseMethod, Callback } from '../events';
import { StandardProps } from '../props';


export interface TextAreaInputEvent {
  /**
   * Input content
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   */
  value: string;
  /**
   * The start position of the selection
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   */
  selectionStart: number;
  /**
   * The end position of the selection
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   */
  selectionEnd: number;
  /**
   * Is composing or not
   * @iOS 3.4
   * @Web
   * @Android 3.4
   * @Harmony 3.4
   */
  isComposing?: boolean;
  /**
   * The type of input action, "paste" when pasting, "normal" otherwise
   * @Android 4.3
   * @iOS 4.3
   * @Harmony 4.3
   */
  inputType?: 'paste' | 'normal';
}

export interface TextAreaFocusEvent {
  /**
   * Input content
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   */
  value: string;
}

export interface TextAreaBlurEvent {
  /**
   * Input content
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   */
  value: string;
}

export interface TextAreaConfirmEvent {
  /**
   * Input content
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   */
  value: string;
}

export interface TextAreaSelectionChangeEvent {
  /**
   * The start position of the selection
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
     * @Web
     */
  selectionStart: number;
  /**
   * The end position of the selection
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
     * @Web
     */
  selectionEnd: number;
}

export interface TextAreaProps extends Omit<StandardProps, 'bindfocus' | 'bindblur'> {
  /**
   * Placeholder
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 3.4
   * @ClayIOS 3.4
   * @ClayMacOS 3.4
   * @ClayWindows 3.4
   * @ClayHarmony 2.14
   */
  placeholder?: string
  /**
   * Initial textarea content. Only applies on the first render and does not trigger `bindinput`.
   * @Android 4.0
   * @iOS 2.17
   * @Harmony 4.0
   */
  'default-value'?: string;
  /**
   * The type of confirm button
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 3.4
   * @ClayIOS 3.4
   * @ClayMacOS 3.4
   * @ClayWindows 3.4
   * @defaultValue 'done'
   */
  'confirm-type'?: 'send' | 'search' | 'go' | 'done' | 'next';
  /**
   * Max input length
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 3.4
   * @ClayIOS 3.4
   * @ClayMacOS 3.4
   * @ClayWindows 3.4
   * @ClayHarmony 2.16
   * @defaultValue 140
   */
  maxlength?: number;
  /**
   * Max input lines
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @ClayAndroid 3.4
   * @ClayIOS 3.4
   * @ClayMacOS 3.4
   * @ClayWindows 3.4
   * @ClayHarmony 2.16
   * @defaultValue undefined
   */
  maxlines?: number;
  /**
   * Bounce effect for iOS
   * @iOS 2.16
   * @defaultValue true
   */
  bounces?: boolean;
  /**
   * Line spacing
   * @Android 2.16
   * @iOS 2.16
   * @Harmony 2.17
   * @ClayAndroid 2.16
   * @ClayIOS 2.16
   * @defaultValue undefined
   */
  'line-spacing'?: number | `${number}px` | `${number}rpx`;
  /**
   * Readonly
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 3.4
   * @ClayIOS 3.4
   * @ClayMacOS 3.4
   * @ClayWindows 3.4
   * @ClayHarmony 2.16
   * @defaultValue false
   */
  readonly?: boolean;

  /**
   * Interaction enabled
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 3.0
   * @Web
   * @ClayAndroid 1.5
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   * @defaultValue false
   */
  disabled?: boolean;

  /**
   * Show soft input keyboard while focused
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @ClayAndroid 3.4
   * @ClayIOS 3.4
   * @defaultValue true
   */
  'show-soft-input-on-focus'?: boolean;

  /**
   * Filter the input content and process it in the form of regular expressions
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayIOS 3.4
   * @defaultValue undefined
   */
  'input-filter'?: string;

  /**
   * Whether to show scroll bar, on HarmonyOS, the scroll bar will always be shown
   * @Android 3.6
   * @iOS 3.5
   * @defaultValue false
   */
  'enable-scroll-bar'?: boolean;

  /**
   * Input content type
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 3.4
   * @ClayIOS 3.4
   * @defaultValue "text"
   */
  type?: 'text' | 'number' | 'digit' | 'tel' | 'email';

  /**
   * Auto correct input content on iOS
   * @iOS 2.16
   * @defaultValue true
   */
  'ios-auto-correct'?: boolean;

  /**
   * Check spelling issue on iOS
   * @iOS 2.16
   * @Web
   * @ClayIOS 2.16
   * @defaultValue true
   */
  'ios-spell-check'?: boolean;

  /**
   * Whether to enter the full-screen input mode when in landscape screen, in which the keyboard and input box will take up the entire screen
   * @Android 2.16
   * @defaultValue true
   */
  'android-fullscreen-mode'?: boolean;

  /**
   * Focused
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 2.16
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   */
  bindfocus?: (e: BaseEvent<'bindfocus', TextAreaFocusEvent>) => void;

  /**
   * Blurred
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 2.16
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.8
   * @ClayMacOS 2.14
   * @ClayWindows 4.0
   * @ClayHarmony 2.14
   */
  bindblur?: (e: BaseEvent<'bindblur', TextAreaBlurEvent>) => void;

  /**
   * Input content changed
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 2.8
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.8
   * @ClayMacOS 2.8
   * @ClayWindows 2.8
   * @ClayHarmony 2.8
   */
  bindinput?: (e: BaseEvent<'bindinput', TextAreaInputEvent>) => void;

  /**
   * Input selection changed
   * @Android 1.5
   * @iOS 2.16
   * @Harmony 2.17
   * @Web
   * @ClayIOS 2.16
   */
  bindselection?: (e: BaseEvent<'bindselection', TextAreaSelectionChangeEvent>) => void;

  /**
   * Confirm button clicked, only work when confirm-type is defined
   * @Android 1.5
   * @iOS 1.5
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.8
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 2.17
   */
  bindconfirm?: (e: BaseEvent<'bindconfirm', TextAreaConfirmEvent>) => void;
}

/**
 * Require focus
 * @Android 1.5
 * @iOS 1.5
 * @Harmony 2.16
 * @Web
 * @ClayAndroid 1.5
 * @ClayIOS 2.8
 * @ClayMacOS 4.0
 * @ClayWindows 2.14
 * @ClayHarmony 2.14
 */
export interface TextAreaFocusMethod extends BaseMethod {
  method: 'focus';
}

/**
 * Release focus
 * @Android 1.5
 * @iOS 1.5
 * @Harmony 2.16
 * @Web
 * @ClayAndroid 1.5
 * @ClayIOS 2.8
 * @ClayMacOS 4.0
 * @ClayWindows 4.0
 * @ClayHarmony 2.14
 */
export interface TextAreaBlurMethod extends BaseMethod {
  method: 'blur';
}

/**
 * Get input content
 * @Android 2.14
 * @iOS 2.16
 * @Harmony 2.16
 * @Web
 * @ClayAndroid 2.14
 * @ClayIOS 2.16
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
 * @ClayHarmony 2.14
 */
export interface TextAreaGetValueMethod extends BaseMethod {
  method: 'getValue';
  success?: Callback<{
    /**
     * Input content
     * @Android 3.4
     * @iOS 3.4
     * @Harmony 3.4
     * @Web
     */
    value: string;
    /**
     * Begin position of the cursor
     * @Android 3.4
     * @iOS 3.4
     * @Harmony 3.4
     * @Web
     */
    selectionStart: number;
    /**
     * End position of the cursor
     * @Android 3.4
     * @iOS 3.4
     * @Harmony 3.4
     * @Web
     */
    selectionEnd: number;
    /**
     * Is composing or not, iOS only
     * @iOS 3.4
     * @Android 3.4
     * @Harmony 3.4
     * @Web
     */
    isComposing: boolean;
  }>;
}

/**
 * Set input content
 * @Android 1.5
 * @iOS 1.5
 * @Harmony 2.16
 * @Web
 * @ClayAndroid 1.5
 * @ClayIOS 2.8
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
 * @ClayHarmony 2.14
 */
export interface TextAreaSetValueMethod extends BaseMethod {
  method: 'setValue';
  params: {
    /**
     * Input content
     * @Android 3.4
     * @iOS 3.4
     * @Harmony 3.4
     * @Web
     */
    value: string;
  };
}

/**
 * Set selection range
 * @Android 2.1
 * @iOS 2.0
 * @Harmony 2.16
 * @Web
 * @ClayAndroid 2.1
 * @ClayIOS 2.8
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
 * @ClayHarmony 2.14
 */
export interface TextAreaSetSelectionRangeMethod extends BaseMethod {
  method: 'setSelectionRange';
  params: {
    /**
     * Start position of the selection
     * @Android 3.4
     * @iOS 3.4
     * @Harmony 3.4
     * @Web
     */
    selectionStart: number;
    /**
     * End position of the selection
     * @Android 3.4
     * @iOS 3.4
     * @Harmony 3.4
     * @Web
     */
    selectionEnd: number;
  };
}

export type TextAreaUIMethods = TextAreaFocusMethod | TextAreaBlurMethod | TextAreaGetValueMethod | TextAreaSetValueMethod | TextAreaSetSelectionRangeMethod;
