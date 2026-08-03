// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { ViewProps } from './view';

/**
 * A desktop-only container whose children can cover native views rendered
 * above Clay's main rendering surface.
 *
 * Use it when a desktop custom element is backed by a native platform view or
 * child window that a regular Lynx element cannot cover.
 * @ClayWindows
 * @ClayMacOS
 */
export interface CoverViewProps extends ViewProps {}
