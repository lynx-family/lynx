import { BaseEvent } from '../events';
import { StandardProps } from '../props';
export interface SVGProps extends StandardProps {
  /**
   * SVG resource URL
   * @iOS
   * @Android
   * @web
   * @Harmony
   * @PC
   */
  'src'?: string;

  /**
   * SVG XML content
   * @iOS
   * @Android
   * @web
   * @Harmony
   * @PC
   */
  'content'?: string;

  /**
   * Host-injected default color used to resolve `currentColor` in SVG content.
   * This does not override explicit `fill` or `stroke` values.
   * @iOS
   * @Android
   * @Harmony
   */
  'current-color'?: string;

  /**
   * SVG Loaded
   * @Android
   * @iOS
   * @Harmony
   * @PC
   */
  bindload?: (e: BaseEvent) => void;
}
