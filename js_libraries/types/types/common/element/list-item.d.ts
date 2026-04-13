// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

export interface ListItemProps extends StandardProps {
  /**
   * The unique key of list child node, and it's a mandatory property.
   * @Android
   * @web
   * @iOS
   * @Harmony
   * @PC
   */
  'item-key': string;

  /**
   * sticky top effect. Not compatible with flatten.
   * @defaultValue false
   * @Android
   * @web
   * @iOS
   * @Harmony
   * @PC
   */
  'sticky-top'?: boolean;

  /**
   * sticky bottom effect. Not compatible with flatten.
   * @defaultValue false
   * @Android
   * @web
   * @iOS
   * @Harmony
   * @PC
   */
  'sticky-bottom'?: boolean;

  /**
   * Adding the `full-span` attribute to `<list-item/>` will make it occupy a single line. You need to configure {@link ListProps."list-type" | list-type} correctly to make the list enter a multi-column layout for this to work.
   * @defaultValue false
   * @Android
   * @web
   * @iOS
   * @Harmony
   * @PC
   */
  'full-span'?: boolean;

  /**
   * Preset size in main scroll axis to control the placeholder size of the view while the list component has not finished rendering. The more accurately it is set, the less flickering the list will have. If not set, we will use list size in main axis as the estimated size of list-item.
   * @defaultValue -1
   * @Android
   * @web
   * @iOS
   * @Harmony
   * @PC
   */
  'estimated-main-axis-size-px'?: number;

  /**
   * Control whether the list-item can be recycled. If set to false, the list-item will not be recycled after being scrolled off the screen, and do not need to be re-rendered when they come back on the screen. The default value is true.
   * @since 3.4
   * @defaultValue true
   * @Android
   * @web
   * @iOS
   * @Harmony
   * @PC
   */
  recyclable?: boolean;
}
