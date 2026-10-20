// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';
import { BaseEvent } from '../events';
import type { LoadBundleEntry } from '../../background-thread/lynx-performance-entry';

export interface FrameProps extends StandardProps {
  /**
   * Sets the loading path for the frame resource.
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 4.2
   * @Web
   */
  src: string;

  /**
   * Passes data to the nested Lynx page within the frame.
   * @Android 3.4
   * @iOS 3.4
   * @Harmony 4.2
   * @Web
   */
  data?: Record<string, unknown> | undefined;

  /**
   * Bind frame load event callback.
   * @Android 3.6
   * @iOS 3.5
   * @Harmony 4.2
   * @Web
   * @Clay 3.3
   */
  bindload?: (e: FrameLoadEvent) => void;

  /**
   * Bind frame load metrics event callback.
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 4.2
   */
  bindloadmetrics?: (e: FrameLoadMetricsEvent) => void;

  /**
   * Passes `globalProps` to the Lynx page embedded in the frame. The embedded page can read it via `lynx.__globalProps`.
   * @Android 3.6
   * @iOS 3.5
   * @Harmony 4.2
   * @Web
   */
  'global-props'?: Record<string, unknown>;

  /**
   * Lets the frame width follow the embedded Lynx page’s content width. When enabled, the embedded page can report its content size, and the frame uses that value as its measured width.
   * @Android 3.8
   * @iOS 3.8
   * @Harmony 4.2
   * @defaultValue false
   */
  'auto-width'?: boolean;

  /**
   * Lets the frame height follow the embedded Lynx page’s content height. When enabled, the embedded page can report its content size, and the frame uses that value as its measured height.
   * @Android 3.8
   * @iOS 3.8
   * @Harmony 4.2
   * @Web
   * @defaultValue false
   */
  'auto-height'?: boolean;

  /**
   * Sets the preset width used before the embedded Lynx page receives an initialized content rect.
   * @Android 4.0
   * @iOS 3.7
   * @Harmony 4.2
   */
  'preset-width'?: `${number}px` | `${number}rpx`;

  /**
   * Sets the preset height used before the embedded Lynx page receives an initialized content rect.
   * @Android 4.0
   * @iOS 3.7
   * @Harmony 4.2
   */
  'preset-height'?: `${number}px` | `${number}rpx`;

  /**
   * Overrides whether the embedded Lynx page uses multiple asynchronous threads. When omitted, the frame inherits the host setting.
   * @Android 3.9
   * @iOS 3.9
   * @Harmony 4.2
   * @defaultValue false
   */
  'enable-multi-async-thread'?: boolean;
}

export interface BaseFrameLoadInfo {
  /**
   * The loaded url of the frame.
   * @Android 3.6
   * @iOS 3.6
   */
  url: string;

  /**
   * Frame loaded status code.
   * @Android 3.6
   * @iOS 3.6
   */
  statusCode: number;

  /**
   * Frame loaded status message.
   * @Android 3.6
   * @iOS 3.6
   */
  statusMessage: string;
}

export type FrameLoadEvent = BaseEvent<'bindload', BaseFrameLoadInfo>;

export type FrameLoadMetricsEntry = Partial<LoadBundleEntry> & Record<string, unknown>;

export interface BaseFrameLoadMetricsInfo {
  /**
   * The loaded url of the frame.
   * @Android 3.9
   * @iOS 3.9
   */
  url: string;

  /**
   * The frame loading mode.
   * @Android 3.9
   * @iOS 3.9
   */
  mode: string;

  /**
   * The raw loadBundle performance entry emitted by the embedded Lynx page.
   * @Android 3.9
   * @iOS 3.9
   */
  entry: FrameLoadMetricsEntry;
}

export type FrameLoadMetricsEvent = BaseEvent<'bindloadmetrics', BaseFrameLoadMetricsInfo>;
