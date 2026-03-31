// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

import { StandardProps } from '../props';
import { BaseEvent, BaseEventOrig, Target } from '../events';

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
