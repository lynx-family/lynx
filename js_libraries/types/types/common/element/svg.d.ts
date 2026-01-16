import { BaseEvent } from '../events';
import { StandardProps } from '../props';
export interface SVGProps extends StandardProps {
  /**
   * SVG resource URL
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  'src'?: string;

  /**
   * SVG XML content
   * @iOS
   * @Android
   * @Harmony
   * @PC
   */
  'content'?: string;

    /**
     * SVG Loaded
     * @Android
     * @iOS
     * @Harmony
     * @PC
     */
    bindload?: (e: BaseEvent) => void;
}
