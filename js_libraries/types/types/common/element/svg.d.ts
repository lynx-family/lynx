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
   * Sets the currentColor value used by serval-svg.
   * This value is consumed when the SVG content uses currentColor.
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
