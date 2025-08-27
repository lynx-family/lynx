// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

export interface FrameProps extends StandardProps {
  /**
   * Sets the loading path for the frame resource.
   * @defaultValue undefined
   * @iOS
   * @Android
   * @since 3.4
   */
  src: string;

  /**
   * Passes data to the nested Lynx page within the frame.
   * @defaultValue undefined
   * @iOS
   * @Android
   * @since 3.4
   */
  data?: Record<string, unknown> | undefined;
}