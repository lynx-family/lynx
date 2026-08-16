import { BaseEvent } from '../events';
import { StandardProps } from '../props';
export interface SVGProps extends StandardProps {
  /**
   * SVG resource URL
   * @Android 3.7
   * @iOS 3.7
   * @Harmony 3.7
   * @Web
   * @ClayAndroid 3.7
   * @ClayIOS 3.7
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  'src'?: string;

  /**
   * SVG XML content
   * @Android 3.7
   * @iOS 3.7
   * @Harmony 3.7
   * @Web
   * @ClayAndroid 3.7
   * @ClayIOS 3.7
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.14
   */
  'content'?: string;

  /**
   * Host-injected default color used to resolve `currentColor` in SVG content.
   * This does not override explicit `fill` or `stroke` values.
   * @Android 1.5
   * @iOS 4.1
   * @Harmony 4.0
   */
  'current-color'?: string;

  /**
   * SVG Loaded
   * @Android 3.7
   * @iOS 3.7
   * @Harmony 3.7
   * @Web
   * @ClayAndroid 3.7
   * @ClayIOS 3.7
   * @ClayMacOS 3.7
   * @ClayWindows 3.7
   * @ClayHarmony 2.17
   */
  bindload?: (e: BaseEvent) => void;
}
