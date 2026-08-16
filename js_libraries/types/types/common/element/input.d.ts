import { BaseEvent, BaseMethod, Callback } from '../events';
import { StandardProps } from '../props';

export interface InputFocusEvent {
  /**
   * Input content
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   */
  value: string;
}

export interface InputBlurEvent {
  /**
   * Input content
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   */
  value: string;
}

export interface InputConfirmEvent {
  /**
   * Input content
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   */
  value: string;
}

export interface InputInputEvent {
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
   * @Android 3.4
   * @Harmony 3.4
   * @Web
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

export interface InputSelectionEvent {
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

export interface InputProps extends Omit<StandardProps, 'bindfocus' | 'bindblur'> {
  /**
   * Placeholder
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 3.9
   * @ClayIOS 3.9
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.17
   */
  placeholder?: string
  /**
   * Initial input content. Only applies on the first render and does not trigger `bindinput`.
   * @Android 4.0
   * @iOS 2.17
   * @Harmony 4.0
   * @ClayAndroid 2.14
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
   * @defaultValue 'send'
   */
  'confirm-type'?: 'send' | 'search' | 'go' | 'done' | 'next';
  /**
   * Max input length
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 2.18
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.17
   * @defaultValue 140
   */
  maxlength?: number;
  /**
   * Readonly
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 2.14
   * @ClayIOS 2.14
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.17
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
   * @defaultValue true
   */
  'show-soft-input-on-focus'?: boolean;

  /**
   * Filter the input content and process it in the form of regular expressions
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayAndroid 2.17
   * @ClayHarmony 2.17
   * @defaultValue undefined
   */
  'input-filter'?: string;

  /**
   * Input content type
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 3.4
   * @Web
   * @ClayMacOS 3.4
   * @ClayWindows 3.4
   * @ClayHarmony 2.16
   * @defaultValue "text"
   */
  type?: 'text' | 'number' | 'digit' | 'password' | 'tel' | 'email';

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
   * @defaultValue true
   */
  'ios-spell-check'?: boolean;

  /**
   * Whether to enter the full-screen input mode when in landscape screen, in which the keyboard and input box will take up the entire screen
   * @Android 2.16
   * @ClayAndroid 2.16
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
   * @ClayMacOS 2.14
   * @ClayWindows 2.14
   * @ClayHarmony 2.14
   */
  bindfocus?: (e: BaseEvent<'bindfocus', InputFocusEvent>) => void;

  /**
   * Blurred
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
  bindblur?: (e: BaseEvent<'bindblur', InputBlurEvent>) => void;

  /**
   * Confirm button clicked
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
  bindconfirm?: (e: BaseEvent<'bindconfirm', InputConfirmEvent>) => void;

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
  bindinput?: (e: BaseEvent<'bindinput', InputInputEvent>) => void;

  /**
   * Input selection changed
   * @Android 1.5
   * @iOS 2.16
   * @Harmony 2.17
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.16
   */
  bindselection?: (e: BaseEvent<'bindselection', InputSelectionEvent>) => void;
}

/**
 * Require focus
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
export interface InputFocusMethod extends BaseMethod {
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
export interface InputBlurMethod extends BaseMethod {
  method: 'blur';
}

/**
 * Get input content
 * @Android 2.14
 * @iOS 2.16
 * @Harmony 2.16
 * @Web
 * @ClayAndroid 2.14
 * @ClayIOS 2.14
 * @ClayMacOS 2.14
 * @ClayWindows 2.14
 * @ClayHarmony 2.17
 */
export interface InputGetValueMethod extends BaseMethod {
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
     * Begin position of the selection
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
    /**
     * Is composing or not, iOS only
     * @Android 3.4
     * @iOS 3.4
     * @Harmony 3.4
     * @Web
     */
    isComposing: boolean;
  }>;
}

/**
 *  Set input content
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
export interface InputSetValueMethod extends BaseMethod {
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
export interface InputSetSelectionRangeMethod extends BaseMethod {
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

export type InputUIMethods = InputFocusMethod | InputBlurMethod | InputGetValueMethod | InputSetValueMethod | InputSetSelectionRangeMethod;
