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
   * Pass global props to the nested Lynx page within the frame.
   * @iOS
   * @Android
   * @since 3.6
   */
  'global-props'?: Record<string, unknown>;
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