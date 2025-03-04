// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

/**
 * Support from Lynx 3.0
 */
export interface TraceOption {
  /**
   * An optional unique identifier for tracing the event flow.
   */
  flowId?: number;
  /**
   * An optional collection of key-value pairs providing additional context for the tracing event.
   */
  args?: Record<string, string>;
}

export interface CommonPerformance {
  /**
   * Support from Lynx 3.0
   *
   * Start a performance tracing event.
   *
   * @param traceName - The name used to identify the tracing event.
   * @param option - Optional parameters providing `flowId` and additional context.
   *
   * @example
   * lynx.performance.profileStart("myEvent", { flowId: 123, args: { key: "value" } });
   */
  profileStart: (traceName: string, option?: TraceOption) => void;

  /**
   * Support from Lynx 3.0
   *
   * End a performance tracing event.
   *
   * @example
   * lynx.performance.profileEnd();
   */
  profileEnd: () => void;

  /**
   * Support from Lynx 3.0
   *
   * Mark a instant trace event. Can connect with `ProfileStart` trace through `flowId`.
   * @param traceName - The name used to identify the tracing mark.
   * @param option - Optional parameters providing `flowId` and additional context.
   *
   * @example
   * lynx.performance.profileMark("myMark", { args: { key: "value" }, flowId:123 });
   */
  profileMark: (traceName: string, option?: TraceOption) => void;

  /**
   * Support from Lynx 3.0
   *
   * Generate a unique flow identifier.
   *
   * @returns A unique numerical identifier for tracing event flows.
   *
   * @example
   * const flowId = lynx.performance.profileFlowId();
   */
  profileFlowId: () => number;

  /**
   * Support from Lynx 2.18
   *
   * Check if the current device is recording trace data.
   *
   * @returns A boolean indicating whether the device is currently recording trace data.
   *
   * @example
   * const isRecording = lynx.performance.isProfileRecording();
   */
  isProfileRecording: () => boolean;
}
