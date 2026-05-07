// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';
import { BaseEvent } from '../events';
import type { LoadBundleEntry } from '../../background-thread/lynx-performance-entry';

export interface FrameProps extends StandardProps {
  /**
   * Sets the loading path for the frame resource.
   * @iOS
   * @Android
   * @since 3.4
   */
  src: string;

  /**
   * Passes data to the nested Lynx page within the frame.
   * @iOS
   * @Android
   * @since 3.4
   */
  data?: Record<string, unknown> | undefined;

  /**
   * Bind frame load event callback.
   * @iOS
   * @Android
   * @since 3.6
   */
  bindload?: (e: FrameLoadEvent) => void;

  /**
   * Bind frame load metrics event callback.
   * @iOS
   * @Android
   * @since 3.9
   */
  bindloadmetrics?: (e: FrameLoadMetricsEvent) => void;

  /**
   * Passes `globalProps` to the Lynx page embedded in the frame. The embedded page can read it via `lynx.__globalProps`.
   * @iOS
   * @Android
   * @since 3.6
   */
  'global-props'?: Record<string, unknown>;

  /**
   * Lets the frame width follow the embedded Lynx page’s content width. When enabled, the embedded page can report its content size, and the frame uses that value as its measured width.
   * @defaultValue false
   * @iOS
   * @Android
   * @since 3.8
   */
  'auto-width'?: boolean;

  /**
   * Lets the frame height follow the embedded Lynx page’s content height. When enabled, the embedded page can report its content size, and the frame uses that value as its measured height.
   * @defaultValue false
   * @iOS
   * @Android
   * @since 3.8
   */
  'auto-height'?: boolean;

  /**
   * Sets the preset width used before the embedded Lynx page receives an initialized content rect.
   * @iOS
   * @Android
   * @since 3.9
   */
  'preset-width'?: `${number}px` | `${number}rpx`;

  /**
   * Sets the preset height used before the embedded Lynx page receives an initialized content rect.
   * @iOS
   * @Android
   * @since 3.9
   */
  'preset-height'?: `${number}px` | `${number}rpx`;

  /**
   * Overrides whether the embedded Lynx page uses multiple asynchronous threads. When omitted, the frame inherits the host setting.
   * @defaultValue false
   * @iOS
   * @Android
   * @since 3.9
   */
  'enable-multi-async-thread'?: boolean;
}

export interface BaseFrameLoadInfo {
  /**
   * The loaded url of the frame.
   * @Android
   * @iOS
   * @since 3.6
   */
  url: string;

  /**
   * Frame loaded status code.
   * @Android
   * @iOS
   * @since 3.6
   */
  statusCode: number;

  /**
   * Frame loaded status message.
   * @Android
   * @iOS
   * @since 3.6
   */
  statusMessage: string;
}

export type FrameLoadEvent = BaseEvent<'bindload', BaseFrameLoadInfo>;

export type FrameLoadMetricsEntry = Partial<LoadBundleEntry> & Record<string, unknown>;

export interface BaseFrameLoadMetricsInfo {
  /**
   * The loaded url of the frame.
   * @Android
   * @iOS
   * @since 3.9
   */
  url: string;

  /**
   * The frame loading mode.
   * @Android
   * @iOS
   * @since 3.9
   */
  mode: string;

  /**
   * The raw loadBundle performance entry emitted by the embedded Lynx page.
   * @Android
   * @iOS
   * @since 3.9
   */
  entry: FrameLoadMetricsEntry;
}

export type FrameLoadMetricsEvent = BaseEvent<'bindloadmetrics', BaseFrameLoadMetricsInfo>;
