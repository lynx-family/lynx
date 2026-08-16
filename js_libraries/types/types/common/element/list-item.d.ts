// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

export interface ListItemProps extends StandardProps {
  /**
   * The unique key of list child node, and it's a mandatory property.
   * @Android 1.4
   * @iOS 2.18
   * @Harmony 1.4
   * @Web
   * @ClayAndroid 1.5
   * @ClayIOS 2.18
   * @ClayMacOS 1.5
   * @ClayWindows 1.5
   * @ClayHarmony 1.5
   */
  'item-key': string;

  /**
   * sticky top effect. Not compatible with flatten.
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @Clay
   * @since 1.6
   * @defaultValue false
   */
  'sticky-top'?: boolean;

  /**
   * sticky bottom effect. Not compatible with flatten.
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @Clay
   * @since 1.6
   * @defaultValue false
   */
  'sticky-bottom'?: boolean;

  /**
   * Adding the `full-span` attribute to `<list-item/>` will make it occupy a single line. You need to configure {@link ListProps."list-type" | list-type} correctly to make the list enter a multi-column layout for this to work.
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @Clay
   * @since 1.6
   * @defaultValue false
   */
  'full-span'?: boolean;

  /**
   * Preset size in main scroll axis to control the placeholder size of the view while the list component has not finished rendering. The more accurately it is set, the less flickering the list will have. If not set, we will use list size in main axis as the estimated size of list-item.
   * @Android 3.1
   * @iOS 3.1
   * @Harmony 3.1
   * @Web
   * @ClayAndroid 3.1
   * @ClayIOS 3.1
   * @defaultValue -1
   */
  'estimated-main-axis-size-px'?: number;

  /**
   * Control whether the list-item can be recycled. If set to false, the list-item will not be recycled after being scrolled off the screen, and do not need to be re-rendered when they come back on the screen. The default value is true.
   * @Android
   * @iOS
   * @Harmony
   * @Web
   * @Clay
   * @since 3.3
   * @defaultValue true
   */
  recyclable?: boolean;
}
