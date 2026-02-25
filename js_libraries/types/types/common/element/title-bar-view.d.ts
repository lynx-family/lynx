// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

/**
 * The title-bar-view element is similar to the app-region style in the web,
 * allowing developers to customize the window dragging area.
 * @ClayWindows
 * @ClayMacOS
 */
export interface TitleBarViewProps extends StandardProps {
  /**
   * When set to true, the title bar view is able to move the window when dragged.
   * @defaultValue false
   * @ClayWindows
   * @ClayMacOS
   */
  'moveable'?: boolean;
}
