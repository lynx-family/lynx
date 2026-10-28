// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';

/**
 * A desktop Clay container that renders above embedded platform views.
 * @ClayMacOS 4.1
 * @ClayWindows 4.1
 */
export interface CoverViewProps extends StandardProps {
  /**
   * Whether pointer events pass through the platform overlay surface.
   * Unlike other container elements, cover-view ignores event-through.
   * @ClayMacOS 4.1
   * @ClayWindows 4.1
   * @defaultValue false
   */
  'events-pass-through'?: boolean;
}
