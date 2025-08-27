// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
import { Element } from './element'

/**
 * Animation timing options configuration
 */
export interface AnimationOptions {
  /**
   * The length of time for the animation to run.
   * @default 0
   */
  duration?: number;

  /**
   * The length of time to wait before starting the animation.
   * @default 0
   */
  delay?: number;

  /**
   * The number of times the animation should repeat. You can set this to `Infinity`
   * to make the animation loop indefinitely.
   * @default 1
   */
  iterations?: number | typeof Infinity;

  /**
   * Whether the animation runs forwards (`normal`), backwards (`reverse`),
   * switches direction after each iteration (`alternate`), or runs backwards
   * and switches direction after each iteration (`alternate-reverse`).
   * @default "normal"
   */
  direction?: 'normal' | 'reverse' | 'alternate' | 'alternate-reverse';

  /**
   * The rate of the animation's change over time. Accepts a timing-function,
   * such as `"linear"`, `"ease-in"`, or `"cubic-bezier(0.42, 0, 0.58, 1)"`.
   * @default "linear"
   */
  easing?: string;

  /**
   * Dictates whether the animation's effects should be reflected by the
   * element(s) prior to playing (`"backwards"`), retained after the animation
   * has completed playing (`"forwards"`), or both.
   * @default "none"
   */
  fill?: 'none' | 'forwards' | 'backwards' | 'both';

  /**
   * The name of the animation, which can be used to uniquely identify it.
   * This name appears in the animation events parameters and is typically used
   * to determine if a particular animation event is the one you're interested in.
   * @since 2.12
   * @default "An internal unique ID"
   */
  name?: string;

  /**
   * Animation motion state, which defines whether an animation is running or paused.
   * Accepts an `animation-play-state`.
   * @default "running"
   */
  'play-state'?: 'running' | 'paused';
}

/**
 * Represents a keyframe effect for animations.
 */
export interface KeyframeEffect {
  /**
   * The target element of the animation.
   */
  readonly target: Element;
  /**
   * The keyframes for the animation.
   */
  readonly keyframes: Record<string, string | number>[];
  /**
   * The options for the animation.
   */
  readonly options: AnimationOptions;
}

/**
 * Represents a CSS animation.
 */
export interface Animation {
  /**
   * The keyframe effect associated with the animation.
   */
  readonly effect: KeyframeEffect;
  /**
   * The unique identifier for the animation.
   */
  readonly id: string;

  /**
   * Cancel the animation.
   */
  cancel(): void;

  /**
   * Pause the animation.
   */
  pause(): void;

  /**
   * Play the animation.
   */
  play(): void;
}