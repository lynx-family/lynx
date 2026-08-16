// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

/**
 * Dynamic Component
 */
export interface ComponentProps extends StandardProps {
  /**
   * Component name
   * @Android 3.5
   * @iOS 3.8
   * @Harmony 4.1
   * @ClayAndroid 4.0
   * @ClayIOS 2.18
   * @ClayMacOS 4.0
   * @ClayWindows 4.0
   * @ClayHarmony 4.0
   */
  'is'?: string;

  [key: string]: any;
}
