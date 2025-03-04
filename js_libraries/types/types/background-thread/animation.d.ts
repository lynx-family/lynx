// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

export interface AnimationElement {
  // keyframes: see https://developer.mozilla.org/en-US/docs/Web/API/Web_Animations_API/Keyframe_Formats
  //  Either an array of keyframe objects, or a keyframe object whose property are arrays of values to iterate over. See Keyframe Formats for more details.
  //
  // timingOptions: see https://developer.mozilla.org/en-US/docs/Web/API/Element/animate
  //  id Optional: A property unique to animate(): a DOMString with which to reference the animation.
  //  delay Optional: The number of milliseconds to delay the start of the animation. Defaults to 0.
  //  direction Optional: Whether the animation runs forwards (normal), backwards (reverse), switches direction after each iteration (alternate), or runs backwards and switches direction after each iteration (alternate-reverse). Defaults to "normal".
  //  duration Optional: The number of milliseconds each iteration of the animation takes to complete. Defaults to 0. Although this is technically optional, keep in mind that your animation will not run if this value is 0.
  //  easing Optional: The rate of the animation's change over time. Accepts the pre-defined values "linear", "ease", "ease-in", "ease-out", and "ease-in-out", or a custom "cubic-bezier" value like "cubic-bezier(0.42, 0, 0.58, 1)". Defaults to "linear".
  //  endDelay Optional: The number of milliseconds to delay after the end of an animation. This is primarily of use when sequencing animations based on the end time of another animation. Defaults to 0.
  //  fill Optional: Dictates whether the animation's effects should be reflected by the element(s) prior to playing ("backwards"), retained after the animation has completed playing ("forwards"), or both. Defaults to "none".
  //  iterationStart Optional: Describes at what point in the iteration the animation should start. 0.5 would indicate starting halfway through the first iteration for example, and with this value set, an animation with 2 iterations would end halfway through a third iteration. Defaults to 0.0.
  // iterations Optional: The number of times the animation should repeat. Defaults to 1, and can also take a value of Infinity to make it repeat for as long as the element exists.
  animate(keyframes: Array<Record<string, any>>, timingOptions: Record<string, any>): Animation;

  playAnimate(ani: Animation): void;

  pauseAnimate(ani: Animation): void;

  cancelAnimate(ani: Animation): void;

  finishAnimate(ani: Animation): void;

  setProperty(propsObj: string | Record<string, string>, propsVal?: string): void;
}

export interface KeyframeEffect {
  readonly target: AnimationElement;
  readonly keyframes: Array<Record<string, any>>;
  readonly options: Record<string, any>;
}

export interface Animation {
  readonly effect: KeyframeEffect;
  readonly id: string;

  cancel(): void;

  pause(): void;

  play(): void;
}
